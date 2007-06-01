/* utility routines for code functions */

#include "s.h"
#include "g.h"

/* push value on val stack */
xpush(v)
struct value v;
{
   if (++xsp < MAXVAL)
      val[xsp] = v;
   else
      err(1, "value stack overflow", void);
}

/* pop top value from stack */
struct value xpop()
{
   if (xsp >= 0)
      return(val[xsp--]);
   longjmp(env, 1);
}

/* error routine */
err(n, s, v)
int n;
char *s;
struct value v;
{
   fprintf(stderr, "error %d, %s\n", n, s);
   if (n >= 100) {
      fprintf(stderr, "offending value: ");
      idump(v, stderr);
      fprintf(stderr, "\n");
      }
   longjmp(env, 1);
}

/* print trace of procedure call */
ptrace(ap, n)
int ap, n;
{
   int i;

   if (ntrace) {
      for (i=1; i<=pnlevel; i++)
         fprintf(stderr,".");
      prproc(ap, stderr, n);
      fprintf(stderr, " called\n");
      ntrace--;
      }
   pnlevel++;
}

/* print trace of procedure return */
ptret(nam, ap)
char *nam;
int ap;
{
   struct value deref();
   int i;

   pnlevel--;
   if (ntrace) {
      for (i=1; i<=pnlevel; i++)
         fprintf(stderr,".");
      fprintf(stderr, "%s returned ", nam);
      idump(deref(val[ap]), stderr);
      fprintf(stderr, "\n");
      ntrace--;
      }
}

/* pop value from stack and print it */
valprnt()
{
   struct value v, deref(), xpop();

   v = deref(xpop());
   vdump(v, stdout);
   if (TYPE(v) != T_VOID)
      printf("\n");
}

/* search through array block for existing elements */
struct value bsear(b, bv, n, l, idx)
struct value bv;
struct block *b;
int n, l, idx;
{
   struct value mkint();
   int k;

   n++;
   for (k=0; k<=CHUNKSIZE-2; k+=2)
      if (b->b_ints[k] != 0)
         if (++l == n) {
            xpush(mkint(n));
            xpush(bv);
            cp++;
            return(mkint(idx+k/2));
            }
}

/* cdump - dump a proc or code block */
cdump(p)
struct block *p;
{
   int i, **q;
   char *getstr(), *nam;

   switch (BTYPE(p)) {
      case B_PROC:
         nam = getstr(p->p_name);
         fprintf(stderr, "%06o %s %d %d\n", p, nam, p->p_nargs, p->p_nlocals);
         q = p->p_code;
      case B_CODE:
         if (BTYPE(p) == B_CODE)
            q = p->c_code;
         for (;;) {
            fprintf(stderr, "%06o ", q);
            if ((int) *q <= MAXLOC) {
               fprintf(stderr, "%06o\n", *q);
               q++;
               }
            else {
               for (i = 0; op[i]; i++)
                  if (**q == (int) op[i]) {
                     fprintf(stderr, "%s\n", opname[i]);
                     break;
                     }
               if (op[i] == 0)
                  fprintf(stderr, "%06o\n", *q);
               if (**q == (int) op[O_LINK])
                  q = (&BLKTOP(((int)q))->c_next->c_code);
               else
                  q++;
               }
            if ((((int)q)&CHUNKMASK) == 0)
               break;
            }
      }
}

/* dump value */
vdump(v, fp)
struct value v;
FILE *fp;
{
   float getr();
   char *s;
   int i;

   switch(TYPE(v)) {
      case T_INT:
         fprintf(fp,"%d", v.v_addr);
         return;
      case T_REAL:
         fprintf(fp,"%g", getr(v));
         return;
      case T_STRING:
         s = v.v_addr;
         for (i=1; i<=XFIELD(v); i++) {
            if ((s&CHUNKMASK) == 0)
               s = STRTOP(s)->str_next->s_chars;
            putc(*s++, fp);
            }
         return;
      case T_PROC:
         fprintf(fp,"procedure");
         return;
      case T_TABLE:
         fprintf(fp,"table");
         return;
      case T_FILE:
         fprintf(fp,"file");
         return;
      }
}

/* change chars back to original form when read in and print out */
nputc(c, fp)
FILE *fp;
char c;
{
   switch(c) {
      case '\n':
         putc('\\', fp);
         putc('n', fp);
         return;
      case '\t':
         putc('\\', fp);
         putc('t', fp);
         return;
      case '\010':
         putc('\\', fp);
         putc('b', fp);
         return;
      }
}

/* image of value */
idump(v, fp)
struct value v;
FILE *fp;
{
   struct proc *p;
   char *getstr(), *s, c;
   float getr();
   int i;

   switch(TYPE(v)) {
      case T_INT:
         fprintf(fp,"%d", v.v_addr);
         return;
      case T_REAL:
         fprintf(fp,"%g", getr(v));
         return;
      case T_STRING:
         fprintf(fp, "\"");
         s = v.v_addr;
         for (i=1; i<=XFIELD(v); i++) {
            if ((s&CHUNKMASK) == 0)
               s = STRTOP(s)->str_next->s_chars;
            if ((c=(*s++)) < ' ')
               nputc(c, fp);
            else
               putc(c, fp);
            }
         fprintf(fp, "\"");
         return;
      case T_PROC:
         if ((i=XFIELD(v)) == 0) {
            p = v.v_addr;
            fprintf(fp,"procedure %s (%06o)", getstr(p->p_name), p);
            }
         else
            fprintf(fp, "builtin procedure %s (#%d)", bfname[i], i);
         return;
      case T_TABLE:
         fprintf(fp,"table (%06o)", v.v_addr);
         return;
      case T_FILE:
         fprintf(fp,"file (%06o)", v.v_addr);
         return;
      case T_VOID:
         fprintf(fp, "void");
         return;
      }
}

/* dereference a value */
struct value deref(v)
struct value v;
{
   while (TYPE(v) == T_VAR)
      v = *(struct value *) v.v_addr;
   return(v);
}

/* convert to numeric */
struct value cvnum(v)
struct value v;
{
   struct value excvnum(), v1;

   v1 = excvnum(v);
   if (TYPE(v1) == T_VOID)
      err(101, "numeric expected", v);
   return(v1);
}

/* convert to real */
struct value cvr(v)
struct value v;
{
   struct value excvr(), v1;

   v1 = excvr(v);
   if (TYPE(v1) == T_VOID)
      err(102, "real expected", v);
   return(v1);
}

/* convert to integer */
struct value cvi(v)
struct value v;
{
   struct value excvi(), v1;

   v1 = excvi(v);
   if (TYPE(v1) == T_VOID)
      err(105, "integer expected", v);
   return(v1);
}

/* convert to string */
struct value cvstr(v)
struct value v;
{
   struct value excvstr(), v1;

   v1 = excvstr(v);
   if (TYPE(v1) == T_VOID)
      err(103, "string expected", v);
   return(v1);
}

/* make a type real struct value */
struct value mkreal(r)
float r;
{
   struct value v;
   int *pr;

   pr = &r;
   v.v_type = SREAL(pr[1]);
   v.v_addr = pr[0];
   return(v);
}

/* make a type integer struct value */
struct value mkint(i)
int i;
{
   struct value v;

   v.v_type = T_INT;
   v.v_addr = i;
   return(v);
}

/* make a type string struct value */
struct value mkstr(s, l)
char *s;
int l;
{
   struct value v;

   v.v_type = T_STRING + l;
   v.v_addr = (int) s;
   return(v);
}

/* make a type var struct value */
struct value mkvar(p)
struct value *p;
{
   struct value v;

   v.v_type = T_VAR;
   v.v_addr = (int) p;
   return(v);
}

/* make a type file struct value */
struct value mkfile(fd)
FILE *fd;
{
   struct value v;

   v.v_type = T_FILE;
   v.v_addr = (int) fd;
   return(v);
}

/* make a type table struct value */
struct value mktbl(b)
struct block *b;
{
   struct value v;

   v.v_type = T_TABLE;
   v.v_addr = (int) b;
   return(v);
}


/* get the real number stored in a struct value */
float getr(v)
struct value v;
{
   int *pr;
   float r;

   pr = &r;
   pr[0] = v.v_addr;
   pr[1] = v.v_type<<3;
   return(r);
}

/* get and return the index value for substrings */
int getidx(i,l)
int i, l;
{
   if (i<-l || i>l+1)
      return(-1);
   if (i <= 0)
      i = l + i + 1;
   return(i);
}

/* concatenate two strings given their value structs */
char *vstrcat(av, bv)
struct value av, bv;
{
   char *hs, *s, *stralloc();
   int i;

   hs = s = stralloc((char *) av.v_addr, XFIELD(av), 0);
   for (i=1; i<=XFIELD(av); i++) {
      if ((s&CHUNKMASK) == 0)
         s = STRTOP(s)->str_next->s_chars;
      s++;
      }
   mstrcpy(s, bv.v_addr, XFIELD(bv), 0);
   return(hs);
}

/* compare strings given value structs */
int vstrcmp(av, bv)
struct value av, bv;
{
   char *sa, *sb;
   int i;

   sa = av.v_addr; sb = bv.v_addr;
   for (i=1; *sa++ == *sb++; i++) {
      if (i>=XFIELD(av) && i>=XFIELD(bv))
         return(0);
      if (i>=XFIELD(bv))
         return(1);
      if (i>=XFIELD(av))
         return(-1);
      if ((sb&CHUNKMASK) == 0)
         sb = STRTOP(sb)->str_next->s_chars;
      if ((sa&CHUNKMASK) == 0)
         sa = STRTOP(sa)->str_next->s_chars;
      }
   if (*--sa > *--sb)
      return(1);
   return(-1);
}

/* compare two strings in string blks for equality */
int mstreq(av, bv)
struct value av, bv;
{
   char *sa, *sb;
   int i;

   sa = av.v_addr; sb = bv.v_addr;
   for (i=1; *sa++ == *sb++; i++) {
      if (XFIELD(av) <= i && XFIELD(bv) <= i)
         return(1);
      if (XFIELD(av) <= i || XFIELD(bv) <= i)
         return(-1);
      if ((sb&CHUNKMASK) == 0)
         sb = STRTOP(sb)->str_next->s_chars;
      if ((sa&CHUNKMASK) == 0)
         sa = STRTOP(sa)->str_next->s_chars;
      }
   return(-1);
}

/* test for equality between a tokstr and a string in a string blk */
int tstreq(sa, sb)
char *sa, *sb;
{
   while (*sa++ == *sb++) {
      if ((sb&CHUNKMASK) == 0)
         if ((sb=STRTOP(sb)->str_next) != NULL)
            sb = sb->s_chars;
         else {
            if (*sa=='\0')
               return(1);
            return(-1);
            }
      if (*sa=='\0' && *sb=='\0')
         return(1);
      }
   return(-1);
}

/* copy string s1 to s2 */
mstrcpy(s2, s1, l, tokstr)
char *s2, *s1;
int l, tokstr;
{
   struct string *p;
   int i;

   for (i=1; i<=l; i++) {
      if ((s2&CHUNKMASK) == 0) {
         p = STRTOP(s2);
         p->str_next = balloc(B_STRING);
         s2 = p->str_next->s_chars;
         }
      if ((s1&CHUNKMASK) == 0 && tokstr == 0)
         s1 = STRTOP(s1)->str_next->s_chars;
      *s2++ = *s1++;
      }
   if ((s2&CHUNKMASK) != 0)
      *s2++ = '\0';
   istr = s2;
   if ((istr&CHUNKMASK) == 0)
      istr = NULL;
}

/* compare av and bv */
int valcmp(av, bv)
struct value av, bv;
{
   struct value cvr(), cvnum();
   float getr(), r1, r2;

   if (TYPE(av) == T_STRING && TYPE(bv) == T_STRING)
      return(vstrcmp(av, bv));
   if (TYPE(av) == T_REAL || TYPE(bv) == T_REAL) {
      r1 = getr(cvr(cvnum(av)));
      r2 = getr(cvr(cvnum(bv)));
      }
   else if (TYPE(av) == T_INT || TYPE(bv) == T_INT) {
      av = cvnum(av);
      bv = cvnum(bv);
      if (TYPE(av) == T_REAL || TYPE(bv) == T_REAL) {
         r1 = getr(cvr(av));
         r2 = getr(cvr(bv));
         }
      else {
         if (av.v_addr == bv.v_addr)
            return(0);
         if (av.v_addr > bv.v_addr)
            return(1);
         return(-1);
         }
      }
   else {
      err(104, "invalid type to comparison operation", av);
      err(104, "invalid type to comparison operation", bv);
      }
   if (r1 == r2)
      return(0);
   if (r1 > r2)
      return(1);
   return(-1);
}

