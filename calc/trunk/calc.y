%{
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "calc.h"

extern struct value Tracecode;
int offset;				/* frame offsets */
struct symbol *funcp = NULL;		/* current function */
struct symbol *Show = NULL;		/* symbol table entry for show */
struct symbol *Table = NULL;		/* symbol table entry for table */
struct symbol *globals = NULL;		/* list of globals */
struct symbol *locals = NULL;		/* list of all locals */
struct dnode *tlist;			/* sorted list of globals */
int changed = 0;			/* if data has changed */
struct symbol *constant();
#define emit1(x) emit(x)
#define emit2(x,y) {emit(x); emit(y);}
#define emit3(x,y,z) {emit(x); emit(y); emit(z); }

extern GlobalAddress(), LocalAddress(), Push(), Add(), Sub(), Mul(), Div(),
	Negate(), Jump(), JumpIfVoid(), JumpNotVoid(), Not(), LessThan(),
	LessOrEqual(), GtrThan(), GtrOrEqual(), Equal(), NotEqual(), Store(),
	Concatenate(), End(), Call(), Lindex(), Rindex(), Return(), Swap(),
	Dup(), Pop(), Print(), Pow();

struct anode {			/* compound return types */
	struct dnode *dlist;
	union inst *ip;
	int argn;
};
%}
%union {
	char *str;		/* string */
	struct symbol *sym;	/* symbol table pointer */
	union inst *inst;	/* machine instruction */
	struct dnode *dnode;	/* dependency node */
	struct anode *ap;	/* compound values */
	int argn;		/* number of arguments */
}
%token	HELP CONTINUE BREAK FOR WHILE IN IF ELSE FUNCTION RETURN END
%token	<sym>	FCON SCON
%token	<str>	ID
%type	<dnode>	expr
%type	<inst>	else cond m and or ocond test
%type	<sym>	procname parm id lhs asgn
%type	<argn>	parms cmdargs
%type	<ap>	arglist elist cexprs
%right	'=' IS
%right	'?' ':'
%left	CAT
%left	'|'
%left	'&'
%left	'>' GE '<' LE EQ NE
%left	'+' '-'
%left	'*' '/'
%left	UNARYMINUS '~' 
%right	'^'
%%

list	:	/* nothing */
	| list '\n'
	| list defn '\n'
	| list asgn '\n'	{ emit1(End);
				if ($2->class == NULL)
					if (globals) {
						$2->class = globals->class;
						globals->class = $2;
						globals = $2;
						}
					else {
						globals = $2;
						globals->class = $2;
						}
				changed++; return (1); }
	| list expr '\n'	{ release($2); emit2(Print, End);
				if (tlist == NULL)
					tlist = tsort(DEFERRED);
				if (changed) {
					compute(tlist);
					changed = 0;
					}
				return (1); }
	| list cmd '\n'  	{ emit1(End); return (1); }
	| list error '\n' 	{ yyerrok; }
	;

cmd	: '?' cmdargs		{ emit3(Call, Show, $2); }
	| HELP cmdargs		{ emit3(Call, Show, $2); }
	;

cmdarg	: ID			{ emit2(Push, constant(T_STRING,$1)); }
	| FCON			{ emit2(Push, $1); }
	;

cmdargs	: /* nothing */		{ $$ = 0; }
	| cmdarg		{ $$ = 1; }
	| cmdargs cmdarg	{ $$ = $1 + 1; }
	;

lhs	: id
	| id '[' expr ']'	{ emit1(Lindex); release($3); }
	;

asgn	: lhs '=' expr	{ emit2(Store, Pop);
			release($1->depen); $1->depen = NULL;
			bfree($1->code); $1->code = NULL;
			if ($1->flags&DEFERRED) {
				release(tlist);
				tlist = NULL;
				}
			$1->flags &= ~DEFERRED; }
	| lhs IS expr	{ struct dnode *dp;
			emit3(Store, Pop, End);
			$1->flags |= DEFERRED;
			release($1->depen); $1->depen = NULL;
			if (dp = $3)
				do {
					dp = dp->link;
					$1->depen = add(dp->sym, $1->depen);
					} while (dp != $3);
			release($3); release(tlist); tlist = NULL;
			bfree($1->code); $1->code = savecode(pc); pc = code; }
	;

m	: /* nothing */			{ $$ = pc; }
	;

else	: ELSE				{ emit1(Jump); $$ = pc; emit1(0); }
	;

stmt	: exp				{ emit1(Pop); }
	| RETURN exp			{ emit1(Return); }
	| WHILE m '(' cond ')' stmt	{ emit2(Jump, $2 - pc);
					backpatch($4, pc); }
	| FOR '(' oexp ';' m ocond ';' m iexp ')' m stmt
					{ union inst buf[100];
					if ($11 > $8) {
						copycode($11 - $8, $8, buf);
						copycode(pc - $11, $11, $8);
						copycode($11 - $8, buf,
							$8 + (pc - $11));
						}
					emit2(Jump, $5 - pc);
					backpatch($6, pc); }
	| IF '(' cond ')' stmt		{ backpatch($3, pc); }
	| IF '(' cond ')' stmt else m stmt
					{ backpatch($3, $7); backpatch($6, pc); }
	| '{' stmtlist '}'
	;

exp	: expr			{ release($1); }
	| lhs '=' expr		{ emit1(Store); release($3); }
	;

oexp	:
	| exp			{ emit1(Pop); }
	;

iexp	:
	| exp			{ emit1(Pop); }
	;

ocond	:			{ $$ = NULL; }
	| cond
	;

cond	: exp			{ emit1(JumpIfVoid); $$ = pc; emit1(0); }
	;

stmtlist: /* nothing */
	| stmtlist '\n'
	| stmtlist stmt
	;

expr	: FCON			{ emit2(Push, $1); $$ = NULL; }
	| SCON			{ emit2(Push, $1); $$ = NULL; }
	| id			{ $$ = dlist($1); }
	| expr '+' expr		{ emit1(Add); $$ = append($1, $3); }
	| expr '-' expr		{ emit1(Sub); $$ = append($1, $3); }
	| expr '*' expr		{ emit1(Mul); $$ = append($1, $3); }
	| expr '/' expr		{ emit1(Div); $$ = append($1, $3); }
	| expr '^' expr		{ emit1(Pow); $$ = append($1, $3); }
	| '-' expr %prec UNARYMINUS	{ emit1(Negate); $$ = $2; }
	| expr '<' expr		{ emit1(LessThan); $$ = append($1, $3); }
	| expr GE expr		{ emit1(GtrOrEqual); $$ = append($1, $3); }
	| expr '>' expr		{ emit1(GtrThan); $$ = append($1, $3); }
	| expr LE expr		{ emit1(LessOrEqual); $$ = append($1, $3); }
	| expr EQ expr		{ emit1(Equal); $$ = append($1, $3); }
	| expr NE expr		{ emit1(NotEqual); $$ = append($1, $3); }
	| expr and expr	%prec '&' { $2->offset = pc-$2; $$ = append($1, $3); }
	| expr or expr	%prec '|' { $2->offset = pc-$2; $$ = append($1, $3); }
	| expr CAT expr		{ emit1(Concatenate); $$ = append($1, $3); }
	| '~' expr		{ emit1(Not); $$ = $2; }
	| '(' expr ')'		{ $$ = $2; }
	| '(' cexprs ELSE expr ')'	{ backpatch($2->ip, pc);
				$$ = append($2->dlist, $4); bfree($2); }
	| id '[' expr ']'	{ emit1(Rindex); $$ = append(dlist($1), $3); }
	| '[' elist ']'		{ emit3(Call, Table, 2*$2->argn);
				$$ = $2->dlist; bfree($2); }
	| ID '(' arglist ')'	{ struct symbol *p; 
				if ((p = lookup($1, GLOBAL)) == NULL) {
					p = install($1);
					p->flags = GLOBAL;
					}
				emit3(Call, p, $3->argn); $$ = $3->dlist;
				bfree($3); }
	;

and	: '&'			{ emit2(Dup, JumpIfVoid); $$ = pc;
				emit2(0, Pop); }
	;

or	: '|'			{ emit2(Dup, JumpNotVoid); $$ = pc;
				emit2(0, Pop); }
	;

arglist	: /* nothing */		{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				$$->argn = 0; $$->dlist = NULL; }
	| expr			{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				$$->argn = 1; $$->dlist = $1; }
	| arglist ',' expr	{ $1->argn++;
				 $1->dlist = append($1->dlist, $3); }
	;

elist	: /* nothing */		{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				$$->argn = 0; $$->dlist = NULL; }
	| expr			{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				$$->argn = 1; $$->dlist = $1;
				emit3(Push, constant(T_REAL, "1.0"), Swap); }
	| expr ':' expr		{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				$$->argn = 1; $$->dlist = append($1, $3); }
	| elist ',' expr	{ char buf[30];
				sprintf(buf, "%d.0", ++$1->argn);
				emit3(Push, constant(T_REAL, buf), Swap);
				$1->dlist = append($1->dlist, $3); }
	| elist ',' expr ':' expr	{ ++$1->argn;
					$1->dlist = append($1->dlist, $3);
					$1->dlist = append($1->dlist, $5); }
	;

cexprs	: expr test expr	{ $$ = (struct anode *)
					alloc(sizeof(struct anode));
				emit1(Jump); $$->ip = pc; emit1(0);
				backpatch($2, pc);
				$$->dlist = append($1, $3); }
	| cexprs ',' expr test expr
				{ union inst *prev = $1->ip;
				emit1(Jump); $1->ip = pc; emit1(prev);
				backpatch($4, pc);
				$1->dlist = append($1->dlist, append($3,$5)); }
	;

test	: ':'			{ emit1(JumpIfVoid); $$ = pc; emit1(0); }
	;

defn	: FUNCTION procname '(' parms ')' stmt
			{ emit3(Push, constant(T_VOID, ""), Return);
			emit1(End);
			$2->amin = $2->amax = $4; $2->offset = offset;
			bfree($2->fcode); $2->fcode = savecode(pc);
			if (Tracecode.real) {
				dumpcode($2->fcode, stderr);
				Tracecode.real--;
				}
			expunge(); funcp = NULL; pc = code; changed++; }
	;

procname: ID		{ if (($$ = lookup($1, GLOBAL)) == NULL)
				$$ = install($1);
			$$->flags = FUNC|GLOBAL; offset = 0; funcp = $$; }
	;

parms	: /* nothing */ 		{ $$ = 0; }
	| parm				{ $$ = 1; }
	| parms ',' parm		{ $$ = $1 + 1; }
	;

parm	: ID	{ if ($$ = lookup($1, LOCAL))
			error("parameter previously declared: ", $$->name);
		else {
			$$ = install($1);
			$$->flags = LOCAL;
			$$->offset = ++offset;
			$$->class = locals;
			locals = $$;
			}
		}
	;

id	: ID	{ if (funcp) {
			if (($$ = lookup($1, LOCAL)) == NULL) {
				$$ = install($1);
				$$->flags |= LOCAL;
				$$->offset = ++offset;
				$$->class = locals;
				locals = $$;
				}
			emit2(LocalAddress, $$);
			}
		else {
			if (($$ = lookup($1, GLOBAL)) == NULL) {
				$$ = install($1);
				$$->flags |= GLOBAL;
				}
			emit2(GlobalAddress, $$);
			}
		}
%%

struct keyword {
	char *str;
	int token;
} keywords[] = {
	"if", 		IF,
	"else",		ELSE,
	"continue",	CONTINUE,
	"break",	BREAK,
	"while",	WHILE,
	"return",	RETURN,
	"for",		FOR,
	"function",	FUNCTION,
	"help",		HELP,
	"in",		IN,
	NULL,		0
};

int lineno = 1;			/* input line number */
FILE *infp;			/* input file */
char *infile;			/* input file name */
int nerrors;			/* error count */
YYSTYPE yylval;

/* backslash - get next char with \'s interpreted */
backslash(c)
int c;
{
	static char transtab[] = "b\bf\fn\nr\rt\t";

	if (c != '\\')
		return c;
	c = ngetc();
	if (c != EOF && islower(c) && strchr(transtab, c))
		return strchr(transtab, c)[1];
	return (c);
}

/* constant - install constant of type t, string representation str */
struct symbol *constant(t, str)
int t;
char *str;
{
	struct symbol *sym;

	if ((sym = lookup(str, CONST)) && t == sym->val.type)
		return (sym);
	sym = new(strsave(str));
	if (t == T_REAL)
		sym->val = newvalue(T_REAL, atof(str));
	else if (t == T_STRING)
		sym->val = newvalue(T_STRING, sym->name);
	else if (t != T_VOID)
		error("system error", " in constant: unknown type");
	sym->flags = CONST;
	return (sym);
}

/* error - issue error message */
error(msg, str)
char *msg, *str;
{
	if (str == NULL)
		str = "";
	if (infile)
		fprintf(stderr, "%s, ", infile);
	fprintf(stderr, "line %d: %s%s\n", lineno, msg, str);
	if (strcmp(msg, "system error") == 0)
		exit(1);
	nerrors++;
}

/* follow - look ahead for >=, etc. */
follow(expect, ifyes, ifno)
int expect, ifyes, ifno;
{
	int c = ngetc();

	if (c == expect)
		return (ifyes);
	if (c != EOF)
		ungetc(c, infp);
	return (ifno);
}

/* ngetc - returns next input character */
int ngetc()
{
	register int c;

	if ((c = getc(infp)) == '\n')
		lineno++;
	return (c);
}

/* yylex - lexical analyzer */
int yylex()
{
	int c, c1, n;
	char *p, tok[TOKSIZE];
	static int last = 0;

top:	while ((c = ngetc()) != EOF) {
		if (c == '#')
			while ((c = ngetc()) != '\n')
				;
		if (c != ' ' && c != '\t')
			break;
		}
	if (c == EOF)
		return (0);
	p = tok;
	if (isalpha(c)) {
		do
			*p++ = c;
		while ((c = ngetc()) != EOF && (isalnum(c) || c == '_'));
		if (c != EOF)
			ungetc(c, infp);
		*p = '\0';
		for (n = 0; keywords[n].token; n++)
			if (strcmp(keywords[n].str, tok) == 0)
				return (last = keywords[n].token);
		yylval.str = strsave(tok);
		return (last = ID);
		}
	if (isdigit(c)) {
		do
			*p++ = c;
		while ((c = ngetc()) != EOF && isdigit(c));
		if (c == '.') {
			do
				*p++ = c;
			while ((c = ngetc()) != EOF && isdigit(c));
			}
		if (c != EOF)
			ungetc(c, infp);
		*p = '\0';
		yylval.sym = constant(T_REAL, tok);
		return (last = FCON);
		}
	*p++ = c;
	switch (c) {
	case '\'': case '"':
		c1 = *--p;
		while ((c = ngetc()) != c1) {
			if (c == EOF || c == '\n') {
				error("missing quote", NULL);
				if (c == '\n')
					ungetc(c, infp);
				break;
				}
			*p++ = c == '\\' ? backslash(c) : c;
			}
		*p = '\0';
		yylval.sym = constant(T_STRING, tok);
		return (last = SCON);
	case '>': return last = follow('=', GE, '>');
	case '<': return last = follow('=', LE, '<');
	case '=': return last = follow('=', EQ, '=');
	case '!': return last = follow('=', NE, '~');
	case '|': return last = follow('|', CAT, '|');
	case ':': return last = follow('=', IS, ':');
	case '\n':
		if (strchr(",:+-/*[(<>=", last) || last == LE || last == GE ||
			last == NE || last == EQ || last == CAT || last == IS ||
			last == ELSE || last == IN)
				goto top;
	default: return last = c;
	}
}

/* yyerror - issue syntax error */
yyerror(s)
char *s;
{
	error(s, NULL);
}
