%{
#include "s.h"
#include "g.h"

struct bplist *mkbpl(), *initbpl(), *bang();
struct symbol *const();
struct stlist *ifstmt(), *ifelse(), *whilest(), *forstmt(), *forin(),
   *stmtl(), *mklist(), *initlst();
struct lelem *merge();
struct code *cballoc();

%}

%union {
   int ival;
   int **cpval;
   char *cval;
   struct string *strval;
   struct bplist *bpval;
   struct stlist *stval;
   struct symbol *smval;
   }

%token RELOP UMINUS IDENTIFIER CONSTANT IF ELSE NEXT LOCAL BREAK CAT
       WHILE RETURN FOR END DO EQ NE LE GE PROCEDURE IN INCLUDE

%type <ival> lhs expr_list arguments decl_list decl

%type <cpval> m

%type <cval> identifier proc_name

%type <bpval> n o  expr optl_expr optl_expr3 cexpr optl_cexpr

%type <stval> proc stmt_list stmt

%type <smval> constant proc_head

%right '='
%left '|'
%left '&'
%left '<' LE EQ NE GE '>'
%left CAT
%left '+' '-'
%left '*' '/' '%'
%left UMINUS
%%

program : p stmt                         { backpatch($2->s_next, cp);
                                           backpatch($2->s_succ, cp);
                                           backpatch($2->s_break, cp);
                                           bfree((char *) $2);
                                           emit(&op[O_RET], 0, 0, 1, "return");
                                           bflush(); }
        ;

p :                                      { fstblk = curblk = cballoc(); }
  ;

proc : proc_head stmt_list m END         { backpatch($2->s_next,$3);
                                           backpatch($2->s_succ,$3);
                                           emit(lookup(0), 0, 0, 1, "gvar");
                                           emit(&op[O_RET], 0, 0, 1, "return");
                                           procend($1);
                                           $$ = mklist(0,$2->s_break,0);
                                           bfree((char *) $2); }
     | error END                         { $$ = mklist(0, 0, 0); }
     ;

proc_head : PROCEDURE proc_name '(' arguments DO decl_list ';'
                                         { $$ = prochead($2, $4, $6); }
          | PROCEDURE proc_name '(' arguments DO
                                         { $$ = prochead($2, $4, 0); }
          | PROCEDURE proc_name '(' DO decl_list ';'
                                         { $$ = prochead($2, 0, $5); }
          | PROCEDURE proc_name '(' DO   { $$ = prochead($2, 0, 0); }
          ;

proc_name : identifier                   { procnam($1); $$ = $1; }
          ;

arguments : identifier                   { param($1); $$ = 1; }
          | arguments ',' identifier     { param($3); $$ = $1+1; }
          ;

decl_list : decl
          | decl_list ';' decl           { $$ = $1 + $3; }
          ;

decl : LOCAL identifier                  { decl($2); $$ = 1; }
     | decl ',' identifier               { decl($3); $$ = $1+1; }
     ;

m :                                      { $$ = cp; }
  ;

n :                                      { emit(&op[O_JUMP], 0, 0, 2, "jump");
                                           $$ = initbpl(cp-1,0); }
  ;

o :                                      { $$ = bang(); }
  ;

stmt_list : {slevel++; } stmt            { $$ = $2; slevel--; }
          | stmt_list ';' m              {slevel++; }
                            stmt         { $$ = stmtl($1, $3, $5); slevel--; }
          ;

stmt : proc                              { emit(&op[O_BASE], 0, 0, 1, "base"); }
     | expr                              { $$ = mklist(0,0,0);
                                           emit(&op[O_BASE],0,0,1,"base"); }
     | IF '(' cexpr DO m stmt            { $$ = ifstmt($3, $5, $6); }
     | IF '(' cexpr DO m stmt n ELSE m stmt
                                      { $$ = ifelse($3, $5, $6, $7, $9, $10); }
     | WHILE '(' m cexpr DO m stmt    { $$ = whilest($3, $4, $6, $7); }
     | FOR '(' optl_expr ';' m optl_cexpr ';' m optl_expr3 DO m stmt
                           { $$ = forstmt($3, $5, $6, $8, $9, $11, $12); }
     | FOR '(' identifier IN                    { ident1 = $3; }
                             expr o DO stmt   { $$ = forin($7, $9); }
     | BREAK                             { emit(&op[O_JUMP], 0, 0, 2, "jump");
                                           $$ = initlst(0, cp-1, 0); }
     | NEXT                              { emit(&op[O_JUMP], 0, 0, 2, "jump");
                                           $$ = initlst(0, 0, cp-1); }
     | RETURN                            { emit(lookup(0), 0, 0, 1, "gvar");
                                           emit(&op[O_RET], 0, 0, 1, "return");
                                           $$ = mklist(0, 0, 0); }
     | RETURN '(' expr ')'               { emit(&op[O_RET], 0, 0, 1, "return");
                                           $$ = mklist(0, 0, 0); }
     | '{' stmt_list '}'                 { $$ = $2; }
     |                                   { $$ = mklist(0, 0, 0); }
     | error ';'                         { $$ = mklist(0, 0, 0); yyerrok; }
     ;

optl_expr : expr                         { emit(&op[O_BASE], 0, 0, 1, "base"); }
          |                              { $$ = 0; }
          ;

optl_expr3 : expr                        { emit(&op[O_BASE], 0, 0, 1, "base");
                                           emit(&op[O_JUMP], 0, 0, 2, "jump");
                                           $$ = initbpl(cp-1,0); }
          |                              { $$ = 0; }
          ;

expr : lhs '=' expr                      { if ($1)
                                              emit(&op[O_SSASN],0,0,1,"ssasn");
                                           else
                                              emit(&op[O_ASGN],0,0,1,"asgn");
                                           $$ = 0; }
     | expr '+' expr                     { emit(&op[O_ADD], 0, 0, 1, "add");
                                           $$ = 0; }
     | expr '-' expr                     { emit(&op[O_SUB], 0, 0, 1, "sub");
                                           $$ = 0; }
     | expr '*' expr                     { emit(&op[O_MUL], 0, 0, 1, "mul");
                                           $$ = 0; }
     | expr '/' expr                     { emit(&op[O_DIV], 0, 0, 1, "div");
                                           $$ = 0; }
     | expr '%' expr                     { emit(&op[O_MOD], 0, 0, 1, "mod");
                                           $$ = 0; }
     | '-' expr         %prec UMINUS     { emit(&op[O_NEG], 0, 0, 1, "neg");
                                           $$ = 0; }
     | '(' expr ')'                      { $$ = $2; }
     | expr '(' expr_list ')'            { emit(&op[O_CALL], $3, 0, 2, "call");
                                           $$ = 0; }
     | expr '(' ')'                      { emit(&op[O_CALL], 0, 0, 2, "call");
                                           $$ = 0; }
     | expr '[' expr ':' expr ']'        { emit(&op[O_SSTR], 0, 0, 1, "sstr");
                                           $$ = 0; }
     | expr '[' expr ']'                 { emit(&op[O_IDX], 0, 0, 1, "idx");
                                           $$ = 0; }
     | identifier                        { var($1); $$ = 0; }
     | constant                          { emit($1, 0, 0, 1, "gval"); $$ = 0; }
     | expr '<' expr                     { emit(&op[O_LT], 0, 0, 1, "lt");
                                           $$ = 0; }
     | expr LE expr                      { emit(&op[O_LE], 0, 0, 1, "le");
                                           $$ = 0; }
     | expr EQ expr                      { emit(&op[O_EQ], 0, 0, 1, "eq");
                                           $$ = 0; }
     | expr NE expr                      { emit(&op[O_NE], 0, 0, 1, "ne");
                                           $$ = 0; }
     | expr '>' expr                     { emit(&op[O_GT], 0, 0, 1, "gt");
                                           $$ = 0; }
     | expr GE expr                      { emit(&op[O_GE], 0, 0, 1, "ge");
                                           $$ = 0; }
     | expr CAT expr                     { emit(&op[O_CAT], 0, 0, 1, "cat");
                                           $$ = 0; }
     | cexpr '|' m cexpr   { backpatch($1->false, $3);
                             $$ = mkbpl(merge($1->true,$4->true), $4->false);
                             bfree((char *) $1); bfree((char *) $4); }
     | cexpr '&' m cexpr   { backpatch($1->true, $3);
                             $$ = mkbpl($4->true,merge($1->false,$4->false));
                             bfree((char *) $1); bfree((char *) $4); }
     ;

lhs : identifier                         { var($1); $$ = 0; }
    | expr '[' expr ']'                  { emit(&op[O_IDXL], 0, 0, 1, "idxl");
                                           $$ = 0; }
    | lhs '[' expr ']'                   { emit(&op[O_IDXL], 0, 0, 1, "idxl");
                                           $$ = 0; }
    | expr '[' expr ':' expr ']'         { $$ = 1; }
    | lhs '[' expr ':' expr ']'          { $$ = 1; }
    ;

cexpr : expr                             { if ($1 == 0) {
                                              emit(&op[O_JEV], 0, 0, 3, "jev");
                                              $$ = initbpl(cp-2,cp-1);
                                              }
                                         }
      ;

optl_cexpr : cexpr
           |                             { $$ = 0; }
           ;

expr_list : expr                         { $$ = 1; }
          | expr_list ',' expr           { $$ = $1 + 1; }
          ;

identifier : IDENTIFIER                  { $$ = yylval.cval; }
           ;

constant : CONSTANT                      { $$ = const(yylval.cval); }
         ;
%%
