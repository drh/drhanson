/* ez: lexical analyzer */

#include <stdio.h>
#include <ctype.h>
#include "ez.h"
#include "tokens.h"

struct keyword {
	char *str;
	int token;
} keywords[] = {
	"if", 		IF,
	"else",		ELSE,
	"continue",	CONTINUE,
	"local",	LOCAL,
	"break",	BREAK,
	"while",	WHILE,
	"return",	RETURN,
	"for",		FOR,
	"end",		END,
	"procedure",	PROCEDURE,
	"in",		IN,
	NULL,		0
};

struct lexval lexval;		/* current lexical value */
int t;				/* current token */
char tok[TOKSIZE];		/* current input token */
int tcursor;			/* position in input of current token */
int nerrors;			/* error count */
struct value src;		/* source string */
char *input;			/* input string */
int cursor;			/* input position */
int lineno;			/* input line number */
FILE *infp;			/* input file */
FILE *errfp;			/* error file (or NULL) */
char *errs;			/* evolving error string */
int elength;			/* length of error string */
char *source;			/* evolving source code */
int slength;			/* length of source */
int buf[2*TOKSIZE];		/* pushback buffer */
int bp;				/* pushback buffer index */
int eofflag;			/* on at end of input */

/* error - issue error message */
error(msg, str)
char *msg, *str;
{
	char buf[TOKSIZE], *s;

	nerrors++;
	if (errfp) {
		fprintf(errfp, "line %d: %s", lineno, msg);
		if (str)
			fprintf(errfp, "%s", str);
		fprintf(errfp, "\n");
		return;
		}
	if (errs) {
		sprintf(buf, "line %d: %s", lineno, msg);
		for (s = buf; *s; elength++)
			SPUTC(errs, *s++);
		if (str)
			for (s = str; *s; elength++)
				SPUTC(errs, *s++);
		SPUTC(errs, '\n');
		elength++;
		}
}

/* follow - check if c is next input character */
int follow(c, p)
char c, *p;
{
	int c1;

	if ((c1 = ngetc()) == c) {
		*p++ = c;
		*p = 0;
		return (c);
		}
	putback(c1);
	return (0);
}

/* initlex - initialize lexical analyzer, take input from fp, or *vp,
 * direct errors to efp */
initlex(fp, vp, efp)
FILE *fp;
struct value *vp;
FILE *efp;
{
	input = NULL;
	bp = nerrors = slength = eofflag = 0;
	cursor = tcursor = 1;
	lineno = 1;
	source = NULL;
	infp = fp;
	if (vp) {
		src = *vp;
		input = SOPENR(vp->v_addr);
		}
	errs = NULL;
	elength = 0;
	errfp = efp;
}

/* gtok - lexical analyzer */
int gtok(ic)
int ic;
{
	register int c;
	int c1, n;
	char *p, *index(), *strsave();

top:
	while ((c = ngetc()) != EOF) {
		if (c == '#')
			while ((c = ngetc()) != '\n')
				;
		if (c != ' ' && c != '\t' && c != ic)
			break;
		}
	tcursor = cursor - 1;
	p = tok;
	if (isalpha(c)) {
		*p++ = c;
		while (isalpha(c = ngetc()) || isdigit(c) || c == '_')
			*p++ = c;
		putback(c);
		*p++ = '\0';
		for (n = 0; keywords[n].token; n++)
			if (strcmp(keywords[n].str, tok) == 0)
				return (keywords[n].token);
		lexval.l_str = strsave(tok);
		return (ID);
		}
	if (isdigit(c)) {
		*p++ = c;
		while (isdigit(c = ngetc()))
			*p++ = c;
		if (c == '.') {
			*p++ = c;
			while (isdigit(c = ngetc()))
				*p++ = c;
			putback(c);
			*p = 0;
			lexval.l_str = strsave(tok);
			return (FCON);
			}
		putback(c);
		*p = 0;
		lexval.l_str = strsave(tok);
		return (CON);
		}
	*p++ = c;
	switch (c) {
	case '\'': case '"': case '`':
		p--;
		while ((c1 = ngetc()) != c) {
			if (c1 == EOF || c1 == '\n') {
				error("missing quote");
				if (c1 == '\n')
					putback(c1);
				break;
				}
			if (c1 == '\\') {
				if ((c1 = ngetc()) == EOF) {
					error("missing quote");
					break;
					}
				if (c1 == '\n')
					continue;
				if (index("ntb", c1))
					c1 = *(index("n\nt\tb\010", c1)+1);
				else if (c1 >= '0' && c1 <= '7') {
					for (n = 0; c1>='0' && c1<='7'; c1=ngetc())
						 n = (n<<3) + c1 - '0';
					putback(c1);
					c1 = n&0377;
					}
				}
			*p++ = c1;
			}
		*p++ = '\0';
		lexval.l_str = strsave(tok);
		lexval.l_length = strlen(tok);
		return (c == '`' ? BCON : SCON);
	case '=':
		if (follow('=', p))
			return (EQ);
		break;
	case '<':
		if (follow('=', p))
			return (LE);
		break;
	case '~':
		if (follow('=', p))
			return (NE);
		break;
	case '>':
		if (follow('=', p))
			return (GE);
		break;
	case '|':
		if (follow('|', p))
			return (CAT);
		break;
		}
	*p = 0;
	return (c);
}

/* mustbe - check for single character token c, advance if t == c */
mustbe(c, ic)
int c, ic;
{
	char str[2];

	if (t == c)
		t = gtok(ic);
	else {
		str[0] = c;
		str[1] = 0;
		error("missing ", str);
		}
}

/* newline - discards newlines */
int newline()
{
	if (t == '\n')
		t = gtok('\n');
	return (t);
}

/* ngetc - returns next input character */
int ngetc()
{
	register int c;

	if (bp)
		return (buf[bp--]);
	else if (eofflag)
		return (EOF);
	else if (infp) {
		if ((c = getc(infp)) == EOF)
			return (eofflag = EOF);
		}
	else if (input) {
		if (cursor > XFIELD(src)) {
			SCLOSER(input);
			return (eofflag = EOF);
			}
		c = SGETC(input);
		cursor++;
		}
	else
		err("ngetc: no input source");
	if (source)
		SPUTC(source, c);
	slength++;
	if (c == '\n')
		lineno++;
	return (c);
}

/* pbstr - put C string s back onto input stream */
pbstr(s)
char s[];
{
	int i;

	for (i = strlen(s); i > 0; putback(s[--i]));
}

/* putback - put c back onto input stream, return c */
putback(c)
int c;
{
	if (bp >= sizeof(buf)/sizeof(int) - 1)
		err("putback: buffer overflow");
	buf[++bp] = c;
	return (c);
}

/* strsave - save a copy of string s */
static char *strsave(s)
char *s;
{
	char *strcpy();

	return (strcpy(alloc(strlen(s)+1), s));
}
