/* built-in functions */

#include "s.h"
#include "g.h"

/* close file */
x_close(n, ap)
int n, ap;
{
   struct value deref(), v;

   val[ap] = void;
   if (n < 1)
      return;
   v = deref(val[++ap]);
   if (TYPE(v) != T_FILE)
      return;
   fclose((FILE *) v.v_addr);
}

x_compile(n, ap)
int n, ap;
{ return; }

x_display(n, ap)
int n, ap;
{
   struct value deref(), v, cvi();
   int l, tlp, tap;

   val[ap] = void;
   if (n < 1)
      return;
   l = n = (cvi(deref(val[++ap]))).v_addr;
   tlp = xlp;
   tap = xap;
   while (tap > 0) {
      if (l != 0 && --n < 0)
         break;
      prproc(tap, stderr, -1);
      fprintf(stderr, "\n");
      prlocs(tap, tlp, stderr);
      tap = val[tlp-1].v_addr;
      tlp = val[tlp].v_addr;
      }
   prglobs(stderr);
}

/* dump arguments */
x_dump(n, ap)
int n, ap;
{
   struct value v, deref(), *vb, *vi;
   struct block *b, *bt, *blk;
   int i, j;

   val[ap] = void;
   while (n-- > 0) {
      v = deref(val[++ap]);
      if (TYPE(v) == T_TABLE) {
         b = v.v_addr;
         if (BTYPE(b) == B_ARRAY) {
            for (i=0; i<=CHUNKSIZE-4; i++)
               if ((blk=b->b_ints[i]) != NULL)
                  bdump(blk, i*DBSIZE);
            if ((blk=b->b_ints[CHUNKSIZE-3]) != NULL)
               for (i=0; i<=CHUNKSIZE-2; i++)
                  if ((bt=blk->b_ints[i]) != NULL)
                     bdump(bt, SINDIR+i*DBSIZE);
            if ((blk=b->b_ints[CHUNKSIZE-2]) != NULL)
               for (i=0; i<=CHUNKSIZE-2; i++)
                  if (blk->b_ints[i] != 0)
                     for (j=0; j<=CHUNKSIZE-2; j++)
                        if ((bt=blk->b_ints[i]->b_ints[j]) != NULL)
                           bdump(bt, DINDIR+i*ISIZE+j*DBSIZE);
            }
         if (BTYPE(b) == B_TABLE)
            for (i=0; i<=CHUNKSIZE-2; i++)
               if ((blk=b->b_ints[i]) !=NULL)
                  for (;;) {
                     for (j=1; j<=CHUNKSIZE-7; j+=4) {
                        vi = &blk->b_ints[j];
                        if (TYPE((*vi)) != T_VOID) {
                           vb = vi++;
                           idxdump(*vb, *vi);
                           }
                        }
                     if ((blk=blk->b_ints[0]) == NULL)
                        break;
                     }
         }
      }
}

/* find leftmost position in s2 where s1 occurs as a substring */
x_find(n, ap)
int n, ap;
{
   struct value mkint();

   if ((n=fsstr(n, ap, 1)) > 0)
      val[ap] = mkint(n);
}

/* return image of argument */
x_image(n, ap)
int n, ap;
{
   struct value deref(), v, mkstr();
   struct proc *p;
   char *slookup(), str[TOKSIZE+3], *getstr(), *getistr();
   int i;
   float getr();

   val[ap] = void;
   if (n < 1)
      return;
   v = deref(val[++ap]);
   switch(TYPE(v)) {
      case T_INT:
         sprintf(str,"%d", v.v_addr);
         break;
      case T_REAL:
         sprintf(str,"%g", getr(v));
         break;
      case T_STRING:
         sprintf(str,"\"%s\"", getistr(v.v_addr, XFIELD(v)));
         break;
      case T_PROC:
         if ((i=XFIELD(v)) == 0) {
            p = v.v_addr;
            sprintf(str,"procedure %s (%06o)", getstr(p->p_name), p);
            }
         else
            sprintf(str, "builtin procedure %s (#%d)", bfname[i], i);
         break;
      case T_TABLE:
         sprintf(str,"table (%06o)", v.v_addr);
         break;
      case T_FILE:
         sprintf(str,"file (%06o)", v.v_addr);
         break;
      case T_VOID:
         sprintf(str, "void");
         break;
      }
   val[--ap] = mkstr(slookup(str), strlen(str));
}

/* convert argument to type integer */
x_integer(n, ap)
int n, ap;
{
   struct value deref(), excvi(), v;

   val[ap] = void;
   if (n < 1)
      return;
   v = excvi(deref(val[ap+1]));
   val[ap] = v;
}

/* return position in s after the longest initial substring in s consisting
   only of characters in c */
x_many(n, ap)
int n, ap;
{
   struct value mkint();

   if ((n=fchars(n, ap, 0)) > 0)
      val[ap] = mkint(n);
}

/* returns string resulting from a character mapping on s1, where each char
   of s1 that is contained in s2 is replaced by the char in s3 */
x_map(n, ap)
int n, ap;
{
   struct value deref(), s1v, s2v, s3v, cvstr(), mkstr();
   char *s1, *s2, *s3, *c, str[TOKSIZE], *sm, *slookup();
   int sap, i, j;

   val[sap=ap] = void;
   if (n < 3)
      return;
   s1v = deref(val[++ap]);
   if (TYPE(s1v) == T_VOID)
      return;
   s1v = cvstr(s1v);
   s2v = cvstr(deref(val[++ap]));
   s3v = cvstr(deref(val[++ap]));
   s1 = s1v.v_addr;
   s2 = s2v.v_addr;
   s3 = s3v.v_addr;
   sm = str;
   c = NULL;
   if (XFIELD(s2v) != XFIELD(s3v))
      err(12, "second and third arguments to map of unequal length", void);
   for (i=1; i<=XFIELD(s1v); i++) {
      if ((s1&CHUNKMASK) == 0)
         s1 = STRTOP(s1)->str_next->s_chars;
      for (j=1; j<=XFIELD(s2v); j++) {
         if ((s2&CHUNKMASK) == 0)
            s2 = STRTOP(s2)->str_next->s_chars;
         if ((s3&CHUNKMASK) == 0)
            s3 = STRTOP(s3)->str_next->s_chars;
         if (*s1 == *s2)
            c = s3;
         s2++;
         s3++;
         }
      if (c == NULL)
         *sm = *s1;
      else
         *sm = *c;
      c = NULL;
      s2 = s2v.v_addr;
      s3 = s3v.v_addr;
      s1++;
      sm++;
      }
   *sm = '\0';
   val[sap] = mkstr(slookup(str), strlen(str));
}

/* return position of end of s1 in s2 */
x_match(n, ap)
int n, ap;
{
   struct value mkint();

   if ((n=fsstr(n, ap, 0)) > 0)
      val[ap] = mkint(n);
}

/* convert argument to type numeric */
x_numeric(n, ap)
int n, ap;
{
   struct value deref(), excvnum(), v;

   val[ap] = void;
   if (n < 1)
      return;
   v = excvnum(deref(val[++ap]));
   val[ap] = v;
}

x_open(n, ap)
int n, ap;
{
   FILE *fd, *fopen();
   char *s, *getstr(), *getmode();
   int sap;
   struct value deref(), mkfile(), v, cvstr();

   val[sap=ap] = void;
   if (n < 1)
      return;
   v = cvstr(deref(val[++ap]));
   s = getstr((char *) v.v_addr);
   if (n == 1)
      fd = fopen(s, "r");
   else {
      v = deref(val[++ap]);
      fd = fopen(s, getmode(v));
      }
   if (fd == NULL)
      return;
   val[sap] = mkfile(fd);
}

/* read a line or n chars from an optionally specified input stream */
x_read(n, ap)
int n, ap;
{
   struct value deref(), v, mkstr(), cvi();
   char *s, str[TOKSIZE], *fgets(), *sa, *stralloc();
   FILE *fp;
   int sap, cn;

   val[sap=ap] = void;
   cn = TOKSIZE;
   fp = stdin;
   if (n > 0) {
      v = deref(val[++ap]);
      if (TYPE(v) == T_FILE) {
         fp = v.v_addr;
         if (n > 1)
            cn = cvi(deref(val[++ap])).v_addr;
         }
      else
         cn = cvi(v).v_addr;
      }
   if ((s = fgets(str, cn, fp)) == NULL)
      return;
   if (cn == TOKSIZE) {
      sa = s + strlen(s) - 1;
      *sa = '\0';
      }
   sa = stralloc(s, strlen(s), 1);
   val[sap] = mkstr(sa, strlen(s));
}

/* convert argument to type real */
x_real(n, ap)
int n, ap;
{
   struct value deref(), excvr(), v;

   val[ap] = void;
   if (n < 1)
      return;
   v = excvr(deref(val[ap+1]));
   val[ap] = v;
}

x_reverse(n, ap)
int n, ap;
{
   struct value deref(), cvstr(), v, mkstr();
   char *s, str[TOKSIZE], *getsstr(), *rs, *slookup();

   val[ap] = void;
   if (n < 1)
      return;
   v = cvstr(deref(val[++ap]));
   s = getsstr(v.v_addr, XFIELD(v));
   n = XFIELD(v);
   rs = str;
   s = s + (n-1);
   while (n-- > 0)
      *rs++ = *s--;
   *rs = '\0';
   val[--ap] = mkstr(slookup(str), strlen(str));
}

/* set the position of the next input or output operation on the stream */
x_seek(n, ap)
int n, ap;
{
   struct value deref(), v, v1, cvi();
   FILE *fd;

   val[ap] = void;
   if (n < 3)
      return;
   v = deref(val[++ap]);
   if (TYPE(v) != T_FILE)
      err(104, "file expected", v);
   fd = v.v_addr;
   v = cvi(deref(val[++ap]));
   v1 = cvi(deref(val[++ap]));
   if (v1.v_addr > 2 || v1.v_addr < 0)
      err(110, "invalid third argument for seek", v1);
   fseek(fd, (long)v.v_addr, v1.v_addr);
}

/* size of tables or strings */
x_size(n, ap)
int n, ap;
{
   struct value av, deref(), mkint(), cvstr();
   struct block *b, *blk;
   int sz, i, j;

   val[ap] = void;
   if (n < 1)
      return;
   av = deref(val[++ap]);
   if (TYPE(av) == T_VOID || TYPE(av) == T_PROC || TYPE(av) == T_FILE)
      return;
   if (TYPE(av) == T_TABLE) {
      b = av.v_addr;
      sz = 0;
      if (BTYPE(b) == B_ARRAY) {
         for (i=0; i<=CHUNKSIZE-4; i++)
            if (b->b_ints[i] != 0)
               sz += NVALS(b->b_ints[i]);
         if (b->b_ints[CHUNKSIZE-3] != 0)
            sz += NVALS(b->b_ints[CHUNKSIZE-3]);
         if (b->b_ints[CHUNKSIZE-2] != 0)
            sz += NVALS(b->b_ints[CHUNKSIZE-2]);
         }
      if (BTYPE(b) == B_TABLE)
         for (i=0; i<=CHUNKSIZE-2; i++)
            if ((blk=b->b_ints[i]) !=NULL)
               for (;;) {
                  for (j=1; j<=CHUNKSIZE-7; j+=4)
                    if (blk->b_ints[j] != T_VOID)
                        sz += 1;
                  if ((blk=blk->b_ints[0]) == NULL)
                     break;
                  }
      val[--ap] = mkint(sz);
      return;
      }
   av = cvstr(av);
   val[--ap] = mkint(XFIELD(av));
}

/* convert argument to string */
x_string(n, ap)
int n, ap;
{
   struct value deref(), excvstr(), v;

   val[ap] = void;
   if (n < 1)
      return;
   v = excvstr(deref(val[ap+1]));
   val[ap] = v;
}

/* trace the next n procedure calls */
x_trace(n, ap)
int n, ap;
{
   struct value deref(), cvi();

   val[ap] = void;
   if (n < 1)
      return;
   ntrace = cvi(deref(val[++ap])).v_addr;
}

/* return type of argument */
x_type(n, ap)
int n, ap;
{
   struct value deref(), v, mkstr();
   char *slookup();

   val[ap] = void;
   if (n < 1)
      return;
   v = deref(val[++ap]);
   switch(TYPE(v)) {
      case T_VOID:
         val[--ap] = mkstr(slookup("void"), 4);
         return;
      case T_INT:
         val[--ap] = mkstr(slookup("integer"), 7);
         return;
      case T_REAL:
         val[--ap] = mkstr(slookup("real"), 4);
         return;
      case T_FILE:
         val[--ap] = mkstr(slookup("file"), 4);
         return;
      case T_STRING:
         val[--ap] = mkstr(slookup("string"), 6);
         return;
      case T_PROC:
         val[--ap] = mkstr(slookup("procedure"), 9);
         return;
      case T_TABLE:
         val[--ap] = mkstr(slookup("table"), 5);
         return;
      }
}

/* return leftmost position in s of the first instance of a character of c */
x_upto(n, ap)
int n, ap;
{
   struct value mkint();

   if ((n=fchars(n, ap, 1)) > 0)
      val[ap] = mkint(n);
}

/* write out arguments */
x_write(n, ap)
int n, ap;
{
   struct value deref(), v;
   FILE *fp;

   val[ap] = void;
   if (n < 1)
      return;
   v = deref(val[++ap]);
   if (TYPE(v) == T_FILE)
      fp = v.v_addr;
   else
      vdump(v, (fp=stdout));
   n--;
   while (n-- > 0)
      vdump(deref(val[++ap]), fp);
}


