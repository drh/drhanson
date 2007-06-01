#include "s.h"
#include "y.tab.h"
#include "g.h"

int fsp;                                /* file stack pointer */
FILE *filestor[MAXFILES];               /* file stack */
FILE *fp;                               /* current input source */
int npar;                               /* count of () pairs */
int nbrk;                               /* count of [] pairs */
int lasttok;                            /* last token returned by yylex */
int lastval;                            /* last value returned by yylex */

/* initialization */
initlex(f)
FILE *f;
{
   fp = f;
   fsp = -1;
   initrw("if", IF, -2);
   initrw("else", ELSE, -1);
   initrw("next", NEXT, -1);
   initrw("local", LOCAL, -1);
   initrw("break", BREAK, -1);
   initrw("while", WHILE, -2);
   initrw("return", RETURN, -1);
   initrw("for", FOR, -2);
   initrw("end", END, -1);
   initrw("procedure", PROCEDURE, -2);
   initrw("in", IN, -1);
   initrw("include", INCLUDE, -1);
   initbf("close", 1, &x_close);
   initbf("compile", 2, &x_compile);
   initbf("display", 3, &x_display);
   initbf("dump", 4, &x_dump);
   initbf("find", 5, &x_find);
   initbf("image", 6, &x_image);
   initbf("integer", 7, &x_integer);
   initbf("many", 8, &x_many);
   initbf("map", 9, &x_map);
   initbf("match", 10, &x_match);
   initbf("numeric", 11, &x_numeric);
   initbf("open", 12, &x_open);
   initbf("read", 13, &x_read);
   initbf("real", 14, &x_real);
   initbf("reverse", 15, &x_reverse);
   initbf("seek", 16, &x_seek);
   initbf("size", 17, &x_size);
   initbf("string", 18, &x_string);
   initbf("trace", 19, &x_trace);
   initbf("type", 20, &x_type);
   initbf("upto", 21, &x_upto);
   initbf("write", 22, &x_write);
}

/* initialize a reserved word entry */
initrw(str, tokval, scope)
char *str;
int tokval, scope;
{
   struct symbol *p, *install();
   char *slookup();

   p = install(slookup(str), S_GLOBAL);
   p->y_scope = scope;
   p->y_val.v_type = T_INT;
   p->y_val.v_addr = tokval;
}

/* initialize built-in functions */
initbf(str, n, funcp)
char *str;
int (*funcp)(), n;
{
   char *slookup();
   struct symbol *p, *install();

   p = install(slookup(str), S_GLOBAL);
   p->y_val.v_type = T_PROC + n;
   p->y_val.v_addr = funcp;
}

/* push files onto stack */
pfile(p)
struct symbol *p;
{
   FILE *inclf, *fopen();

   if ((inclf=fopen(p->y_name,"r")) == NULL)
      fprintf(stderr, "s : can't open file %s\n", p->y_name);
   else
      if (++fsp < MAXFILES) {
         filestor[fsp] = fp;
         fp = inclf;
         }
      else
         fprintf(stderr, "file stack overflow\n");
}

/* returns next char from nested input sources */
int ngetc()
{
   int c;

   while ((c=getc(fp)) == EOF) {
      if (fp != stdin)
         fclose(fp);
      if (fsp < 0)
         return(c);
      fp = filestor[fsp--];
      }
   return(c);
}

/* initialize for lexical analysis */
startlex()
{
   npar = 0;
   nbrk = 0;
   doflag = 0;
   slevel = 0;
   lasttok = 0;
   lastval = 0;
   sp = 0;
}

/* lexical analyzer */
int yylex()
{
   register int c;
   int c1, n;
   char tok[TOKSIZE], *p, *slookup(), *stralloc();
   struct symbol *sptr, *lookup(), *const();

top:
   while ((c=ngetc()) != EOF) {
      if (c == '#')
         while ((c=ngetc()) != '\n')
            ;
      if (c != ' ' && c != '\t')
         break;
      }
   if (c == EOF) {
      eofflag++;
      return (-1);
      }
   if (isalpha(c)) {
      p = tok;
      *p++ = c;
      while (isalpha(c=ngetc()) || isdigit(c) || c=='_')
         *p++ = c;
      ungetc(c, fp);
      *p++ = '\0';
      if (strcmp("include", tok) == 0) {
         while (ngetc() != '\"')
            ;
         for (p = tok; (c = ngetc()) != '\"'; *p++ = c)
            if (c == '\n') {
               yyerror("missing quote");
               ungetc(c, fp);
               break;
               }
         *p = '\0';
         lensav = strlen(tok);
         ctype = T_STRING;
         if (lensav > NAMSIZE)
            lensav = NAMSIZE;
         pfile(const(slookup(tok)));
         goto top;
         }
      yylval.cval = slookup(tok);
      lasttok = IDENTIFIER;
      if ((sptr=lookup(yylval.cval)) != NULL)
         if (sptr->y_scope < 0) {
            if (sptr->y_scope == -2)
               doflag++;
            lasttok = sptr->y_val.v_addr;
            }
      return (lasttok);
      }
   if (isdigit(c)) {
      p = tok;
      *p++ = c;
      ctype = T_INT;
      while (isdigit(c=ngetc()))
         *p++ = c;
      if (c == '.') {
         *p++ = c;
         ctype = T_REAL;
         if ((c=ngetc()) != '.') {
            *p++ = c;
            while (isdigit(c=ngetc()))
               *p++ = c;
            }
         else {
            ungetc(c, fp);
            c = '.';
            }
         }
      *p++ = '\0';
      ungetc(c, fp);
      yylval.cval = slookup(tok);
      lasttok = CONSTANT;
      return (lasttok);
      }
   switch (c) {
      case '\"':
         ctype = T_STRING;
         p = tok;
         while ((c1=ngetc()) != c) {
            if (c1 == EOF || c1 == '\n') {
               yyerror("missing quote");
               if (c1 == '\n')
                  ungetc(c1, fp);
               break;
               }
            if (c1 == '\\') {
               if ((c1=ngetc()) == EOF) {
                  yyerror("missing quote");
                  break;
                  }
               else if (c1 == 'n')
                  *p++ = '\n';
               else if (c1 == 't')
                  *p++ = '\t';
               else if (c1 == 'b')
                  *p++ = '\010';
               else if (c1 >= '0' && c1 <= '7') {
                  for (n=0; c1>='0' && c1<='7'; c1=ngetc())
                     n = (n<<3) + c1 - '0';
                  ungetc(c1, fp);
                  *p++ = n & 0377;
                  }
               else
                  *p++ = c1;
               }
           else
               *p++ = c1;
            }
         *p++ = '\0';
         yylval.cval = slookup(tok);
         lensav = strlen(tok);
         lasttok = CONSTANT;
         return (lasttok);
      case '=':
         if ((c1 = ngetc()) == '=')
            c = EQ;
         else
            ungetc(c1, fp);
         break;
      case '<':
         if ((c1 = ngetc()) == '=')
            c = LE;
         else
            ungetc(c1, fp);
         break;
      case '~':
         if ((c1 = ngetc()) == '=')
            c = NE;
         else
            ungetc(c1, fp);
         break;
      case '>':
         if ((c1 = ngetc()) == '=')
            c = GE;
         else
            ungetc(c1, fp);
         break;
      case '|':
         if ((c1 = ngetc()) == '|')
            c = CAT;
         else
            ungetc(c1, fp);
         break;
      case '\n':
         if (lasttok == ELSE)
            goto top;
         else if (lasttok == END)
            c = (level > 1) ? ';' : -1;
         else if (lasttok == IDENTIFIER || lasttok == ',' || lasttok == CONSTANT ||
             lasttok == BREAK || lasttok == NEXT || lasttok == RETURN ||
             lasttok == '}' ||
             (lasttok == ')' && npar == 0) || (lasttok == ']' && nbrk == 0))
                c = (level || slevel>0) ? ';' : -1;
         else
            goto top;   /* ignore this newline */
         break;
      case '(':
         npar++;
         break;
      case ')':
         if (--npar == 0 && doflag) {
            doflag = 0;
            while ((c = ngetc()) == '\n')
               ;
            ungetc(c, fp);
            c = DO;
            }
      case '[':
         nbrk++;
         break;
      case ']':
         nbrk--;
         break;
      }
   yylval.ival = c;
   lasttok = c;
   return (c);
}

