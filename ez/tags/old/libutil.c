/* library utility functions */

#include "s.h"
#include "g.h"

/* dump data block for arrays */
bdump(blk, indx)
struct block *blk;
int indx;
{
   int i;
   struct value *v, mkint();

   for (i=0; i<=CHUNKSIZE-2; i+=2)
      if (blk->b_ints[i] != 0) {
         v = (&blk->b_ints[i]);
         idxdump(mkint(indx+i/2), *v);
         }
}

/* dump index-value pairs for tables and array */
idxdump(v1, v2)
struct value v1, v2;
{
   idump(v1, stderr);
   fprintf(stderr, " : ");
   idump(v2, stderr);
   fprintf(stderr, "\n");
}

/* find char set c in s */
int fchars(n, ap, front)
int n, ap, front;
{
   struct value deref(), cv, sv, cvstr(), cvi();
   char *s, *c;
   int i, j, si, ei;

   val[ap] = void;
   if (n < 2)
      return(-1);
   cv = cvstr(deref(val[++ap]));
   sv = cvstr(deref(val[++ap]));
   c = cv.v_addr;
   s = sv.v_addr;
   si = 1; ei = XFIELD(sv)+1;
   if (n > 2)
      si = getidx(cvi(deref(val[++ap])).v_addr, XFIELD(sv));
   if (n > 3)
      ei = getidx(cvi(deref(val[++ap])).v_addr, XFIELD(sv));
   if (si < 0 || ei < 0)
      return(-1);
   if (si > ei) {
      i = si; si = ei; ei = i; }
   for (i=1; i<si; i++)
      if (((++s)&CHUNKMASK) == 0)
         s = STRTOP(s)->str_next->s_chars;
   for (i=si; i<ei; i++) {
      for (j=1; *s != *c++; j++) {
         if (j>XFIELD(cv))
            break;
         if ((c&CHUNKMASK) == 0)
            c = STRTOP(c)->str_next->s_chars;
         }
      if (*s == *--c) {
         if (front)
            return(i);
         for (; i<ei; i++) {
            if (((++s)&CHUNKMASK) == 0)
               s = STRTOP(s)->str_next->s_chars;
            c = cv.v_addr;
            for (j=1; *s != *c++; j++) {
               if (j>XFIELD(cv))
                  break;
               if ((c&CHUNKMASK) == 0)
                  c = STRTOP(c)->str_next->s_chars;
               }
            if (*s != *--c)
               break;
            }
         if (i < ei)
            i++;
         return(i);
         }
      c = cv.v_addr;
      if (((++s)&CHUNKMASK) == 0)
         s = STRTOP(s)->str_next->s_chars;
      }
   return(-1);
}

/* find substring s1 in s2 */
int fsstr(n, ap, front)
int ap, front, n;
{
   struct value s1v, s2v, deref(), cvstr();
   char *s1, *s2, *ns2;
   int i, j, k, si, ei;

   val[ap] = void;
   if (n < 2)
      return(-1);
   s1v = cvstr(deref(val[++ap]));
   s2v = cvstr(deref(val[++ap]));
   s1 = s1v.v_addr;
   s2 = s2v.v_addr;
   si = 1; ei = XFIELD(s2v)+1;
   if (n > 2)
      si = getidx(cvi(deref(val[++ap])).v_addr, XFIELD(s2v));
   if (n > 3)
      ei = getidx(cvi(deref(val[++ap])).v_addr, XFIELD(s2v));
   if (si < 0 || ei < 0)
      return(-1);
   if (si > ei) {
      i = si; si = ei; ei = i; }
   for (i=1; i<si; i++)
      if (((++s2)&CHUNKMASK) == 0)
         s2 = STRTOP(s2)->str_next->s_chars;
   for(k=si;;) {
      for (j=k; *s1 != *s2; j++) {
         s2++;
         if (j>=ei)
            return(-1);
         if ((s2&CHUNKMASK) == 0)
            s2 = STRTOP(s2)->str_next->s_chars;
         }
      ns2 = s2;
      for (i=1; *s1 == *s2; i++) {
         if (i == XFIELD(s1v))
            if (front)
               return(j);
            else
               return(j+i);
         if (j+i >= ei)
            return(-1);
         if ((++s1&CHUNKMASK) == 0)
            s1 = STRTOP(s1)->str_next->s_chars;
         if ((++s2&CHUNKMASK) == 0)
            s2 = STRTOP(s2)->str_next->s_chars;
         }
      k = j + 1;
      s1 = s1v.v_addr;
      s2 = ns2 + 1;
      }
}

/* explicit convert to integer */
struct value excvi(v)
struct value v;
{
   struct value mkint(), excvnum();
   float getr();

   v = excvnum(v);
   if (TYPE(v) == T_VOID)
      return(void);
   if (TYPE(v) == T_INT)
      return(v);
   return(mkint((int) getr(v)));
}

/* explicit convert to real */
struct value excvr(v)
struct value v;
{
   struct value mkreal(), excvnum();

   v = excvnum(v);
   if (TYPE(v) == T_VOID)
      return(void);
   if (TYPE(v) == T_REAL)
      return(v);
   return(mkreal((float) v.v_addr));
}

/* explicit convert to numeric */
struct value excvnum(v)
struct value v;
{
   int l;
   char *s, *hs;
   double atof();
   struct value mkreal(), mkint();

   if (TYPE(v) == T_INT || TYPE(v) == T_REAL)
      return(v);
   if (TYPE(v) != T_STRING)
      return(void);
   s = v.v_addr;
   l = XFIELD(v);
   v.v_type = T_INT;
   while ((*s == ' ' || *s == '\t') && --l >= 0) {
      s++;
      if ((s&CHUNKMASK) == 0 && (s=STRTOP(s)->str_next) != NULL)
         s = s->s_chars;
      }
   if (l < 0)
      return(void);
   hs = s;
   while (*s >= '0' && *s <= '9' && --l >= 0) {
      s++;
      if ((s&CHUNKMASK) == 0 && (s=STRTOP(s)->str_next) != NULL)
         s = s->s_chars;
      }
   if (l-- > 0 && *s == '.') {
      s++;
      if ((s&CHUNKMASK) == 0 && (s=STRTOP(s)->str_next) != NULL)
         s = s->s_chars;
      v.v_type = T_REAL;
      while (l-- > 0 && *s >= '0' && *s <= '9') {
         s++;
         if ((s&CHUNKMASK) == 0 && (s=STRTOP(s)->str_next) != NULL)
            s = s->s_chars;
         }
      }
   if (l > 0)
      return(void);
   if (TYPE(v) == T_INT)
      v.v_addr = atoi(hs);
   else
      v = mkreal((float) atof(hs));
   return(v);
}

/* explicit convert to string */
struct value excvstr(v)
struct value v;
{
   struct value mkstr();
   char *s, str[TOKSIZE], *slookup(), *sa;
   float getr();

   if (TYPE(v) == T_STRING)
      return(v);
   if (TYPE(v) != T_REAL && TYPE(v) != T_INT)
      return(void);
   s = str;
   if (TYPE(v) == T_INT)
      sprintf(s, "%d", v.v_addr);
   if (TYPE(v) == T_REAL)
      sprintf(s, "%g", getr(v));
   sa = stralloc(s, strlen(s), 1);
   return(mkstr(sa, strlen(s)));
}

/* get string */
char *getstr(vs)
char *vs;
{
   char filnam[NAMSIZE+1], *s;
   int i;

   s = filnam;
   for (i=1; i<=NAMSIZE; i++) {
      if ((vs&CHUNKMASK) == 0 && (vs=STRTOP(vs)->str_next) != NULL)
         vs = vs->s_chars;
      if (vs == NULL || *vs == '\0')
         break;
      *s++ = *vs++;
      }
   *s = '\0';
   return(filnam);
}

char *getsstr(vs, l)
char *vs;
int l;
{
   char str[TOKSIZE+1], *s;
   int i;

   s = str;
   for (i=1; i<=TOKSIZE; i++) {
      if (i > l)
         break;
      if ((vs&CHUNKMASK) == 0 && (vs=STRTOP(vs)->str_next) != NULL)
         vs = vs->s_chars;
      *s++ = *vs++;
      }
   *s = '\0';
   return(str);
}

char *getistr(vs, l)
char *vs;
int l;
{
   char str[TOKSIZE+1], *s, c, *sputc();
   int i;

   s = str;
   for (i=1; i<=TOKSIZE; i++) {
      if (i > l)
         break;
      if ((vs&CHUNKMASK) == 0 && (vs=STRTOP(vs)->str_next) != NULL)
         vs = vs->s_chars;
      if ((c=(*vs++)) < ' ')
         s = sputc(c, s);
      else
         *s++ = c;
      }
   *s = '\0';
   return(str);
}

/* change chars back to original form when read in and put in s */
char *sputc(c, s)
char *s, c;
{
   switch(c) {
      case '\n':
         *s++ = '\\';
         *s++ = 'n';
         return(s);
      case '\t':
         *s++ = '\\';
         *s++ = 't';
         return(s);
      case '\010':
         *s++ = '\\';
         *s++ = 'b';
         return(s);
      }
}

/* get string for mode */
char *getmode(v1)
struct value v1;
{
   struct value cvstr(), v;

   v = cvstr(v1);
   if (tstreq("r", (char *)v.v_addr) > 0)
      return("r");
   if (tstreq("w", (char *)v.v_addr) > 0)
      return("w");
   if (tstreq("rw", (char *)v.v_addr) > 0)
      return("rw");
   if (tstreq("a", (char *)v.v_addr) > 0)
      return("a");
   err(109, "invalid file mode", v);
   return(NULL);
}

/* print out proc name and arguments */
prproc(tap, fp, n)
FILE *fp;
int tap, n;
{
   struct value deref(), v;
   struct proc *p;
   char *getstr(), *nam;
   int i;

   v = deref(val[tap]);
   p = v.v_addr;
   if ((i=XFIELD(v)) > 0)
      nam = bfname[i];
   else
      nam = getstr(p->p_name);
   if (n < 0)
      n = p->p_nargs;
   fprintf(fp,"%s (", nam);
   for (i=1; i<=n; i++) {
      idump(deref(val[++tap]), fp);
      if (i < n)
         fprintf(fp,", ");
      }
   fprintf(fp, ")");
}

/* print out images of locals to a procedure */
prlocs(tap, tlp, fp)
FILE *fp;
int tap, tlp;
{
   struct value deref();
   struct proc *p;
   int i;

   p = deref(val[tap]).v_addr;
   for (i=1; i<=p->p_nlocals; i++) {
      fprintf(fp, "   ");
      idump(deref(val[++tlp]), fp);
      fprintf(fp, "\n");
      }
}

/* print out global names and their values */
prglobs(fp)
FILE *fp;
{
   int i;
   char *getstr();
   struct symbol *p;

   for (i=0; i<SYMSIZE; i++)
      for (p = symtab[i]; p != NULL; p = p->slink)
         if (p->y_scope == S_GLOBAL && p->y_name != NULL) {
            fprintf(fp, "%s = ", getstr(p->y_name));
            idump(p->y_val, fp);
            fprintf(fp, "\n");
            }
}
