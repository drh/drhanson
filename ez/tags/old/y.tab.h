
typedef union  {
   int ival;
   int **cpval;
   char *cval;
   struct string *strval;
   struct bplist *bpval;
   struct stlist *stval;
   struct symbol *smval;
   } YYSTYPE;
extern YYSTYPE yylval;
# define RELOP 257
# define UMINUS 258
# define IDENTIFIER 259
# define CONSTANT 260
# define IF 261
# define ELSE 262
# define NEXT 263
# define LOCAL 264
# define BREAK 265
# define CAT 266
# define WHILE 267
# define RETURN 268
# define FOR 269
# define END 270
# define DO 271
# define EQ 272
# define NE 273
# define LE 274
# define GE 275
# define PROCEDURE 276
# define IN 277
# define INCLUDE 278
