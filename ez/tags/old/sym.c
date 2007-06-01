/* symbol table management */

#include "s.h"
#include "g.h"

#define SSIZE 119
struct sentry *stab[SSIZE];

/* initialize string and symbol table arrays to null, set initial values */
initsym()
{
   int i;
   char *slookup(), *s, *stralloc();
   struct symbol *install(), *p;
   struct value mkfile(), mkstr();

   for (i=0; i<SSIZE; i++)
      stab[i] = NULL;
   for (i=0; i<SYMSIZE; i++)
      symtab[i] = NULL;
   for (i=0; i<SYMSIZE; i++)
      constab[i] = NULL;
   install(NULL, S_GLOBAL);
   p = install(slookup("input"), S_GLOBAL);
   p->y_val = mkfile(stdin);
   p = install(slookup("output"), S_GLOBAL);
   p->y_val = mkfile(stdout);
   p = install(slookup("errout"), S_GLOBAL);
   p->y_val = mkfile(stderr);
   p = install(slookup("lcase"), S_GLOBAL);
   p->y_val = mkstr(stralloc("abcdefghijklmnopqrstuvwxyz", 26, 1), 26);
   p = install(slookup("ucase"), S_GLOBAL);
   p->y_val = mkstr(stralloc("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 26, 1), 26);
   p = install(slookup("ascii"), S_GLOBAL);
   s = "\0\01\02\03\04\05\07\b\t\n\013\014\r\016\017\020\021\022\023\024\
\025\026\027\030\031\032\033\034\035\036\037\040!\"#$%&\'()*+,-./\
0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnop\
qrstuvwxyz{|}~\177";
   p->y_val = mkstr(stralloc(s, 128, 1), 128);
   level = 0;
   cp = istr = NULL;
   void.v_type = T_VOID;
   void.v_addr = 0;
}

/* look for str in string table, install if neccessary, return ptr to entry */
char *slookup(str)
char *str;
{
   int hashval;
   char *malloc(), *hstr, *stralloc();
   struct sentry *p;

   hstr = str;
   for (hashval=0; *str!='\0';)
      hashval += *str++;
   for (p=stab[hashval=hashval%SSIZE]; p!=NULL; p=p->snext)
      if (tstreq(hstr,p->schar) > 0)
         return(p->schar);
   p = (struct sentry *) malloc(sizeof(struct sentry));
   p->snext = stab[hashval];
   stab[hashval] = p;
   return(p->schar=stralloc(hstr,strlen(hstr),1));
}

/* lookup and install in constant table */
struct symbol *clookup(id, ctype, lensav)
char *id;
int ctype, lensav;
{
   double atof();
   struct value mkint(), mkreal(), mkstr();
   struct symbol *p;
   char *malloc();
   int hash;

   for (p=constab[hash=((unsigned) id)%SYMSIZE]; p!=NULL; p=p->slink)
      if (id == p->y_name && ctype == TYPE(p->y_val))
         return(p);
   p = (struct symbol *) malloc(sizeof(struct symbol));
   p->y_name = id;
   p->y_loff = p->y_scope = 0;
   p->y_fcn = op[O_GVAL];
   p->slink = constab[hash];
   constab[hash] = p;
   if (ctype == T_INT)
      p->y_val = mkint(atoi(id));
   else if (ctype == T_REAL)
      p->y_val = mkreal(atof(id));
   else if (ctype == T_STRING)
      p->y_val = mkstr(id, lensav);
   return(p);
}

/* lookup id in symbol table and return ptr into the symbol table if found */
struct symbol *lookup(id)
char *id;
{
   struct symbol *p;

   for (p=symtab[((unsigned) id)%SYMSIZE]; p!=NULL; p=p->slink)
      if (id==p->y_name && (p->y_scope==level || p->y_scope<=S_GLOBAL))
         return(p);
   return(NULL);
}

/* install id as global or local and return ptr into the symbol table */
struct symbol *install(id, scope)
char *id;
int scope;
{
   struct symbol *p, *h, *t;
   char *malloc();
   int hash;

   p = (struct symbol *) malloc(sizeof(struct symbol));
   p->y_name = id;
   if (scope == S_GLOBAL) {
      p->y_loff = 0;
      p->y_scope = 0;
      p->y_fcn = op[O_GVAR];
      }
   else {
      p->y_loff = loff;
      p->y_scope = level;
      p->y_fcn = NULL;
      }
   if (scope == S_PARAM)
      p->y_loff = aoff;
   for (h=symtab[hash=((unsigned) id)%SYMSIZE]; h!=NULL; h=h->slink) {
      if (p->y_scope >= h->y_scope)
         break;
      t = h;
      }
   p->slink = h;
   if (h == symtab[hash])
      symtab[hash] = p;
   else
      t->slink = p;
   p->y_val.v_type = T_VOID;
   p->y_val.v_addr = 0;
   return(p);
}

char *stralloc(s, l, tokstr)
char *s;
int l, tokstr;
{
   char *h;
   struct string *p;
   struct block *balloc();

   if (istr == NULL) {
      p = balloc(B_STRING);
      istr = p->s_chars;
      }
   mstrcpy((h=istr), s, l, tokstr);
   return(h);
}

/* remove - remove symbols with scope == lev from symbol table */
remove(lev)
int lev;
{
   int i;
   struct symbol *p, *t;

   for (i=0; i<SYMSIZE; i++)
      for (p=symtab[i]; p!=NULL && p->y_scope >= lev; p=p->slink)
         if (p->y_scope == lev) {
            if (p == symtab[i])
               symtab[i] = p->slink;
            else
               t->slink = p->slink;
            bfree((char *) p);
            }
         else
            t = p;
}
