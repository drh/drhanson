/* interpreter and code functions */

#include "s.h"
#include "g.h"

/* initialize stack pointers for execution */
xinit()
{
   xsp = -1;
   pnlevel = 0;
   xpush(void);
   xap = xsp;
   xpush(void);
   xpush(void);
   xlp = xsp;
}

/* interpreter */
interp(bp)
int **bp;
{
   int (**f)(), **cp1;

   cp1 = cp;
   cp = bp;
   for(;;) {
      f = *cp++;
      if ((**f)(f) == 0)
         xpush(void);
      }
   cp = cp1;
}

/* code functions */

int o_add(p)
int *p;
{
   struct value av, bv;
   struct value xpop(), deref(), cvnum(), mkreal(), cvr(), mkint();
   float getr();

   bv = cvnum(deref(xpop()));
   av = cvnum(deref(xpop()));
   if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
      xpush(mkint(av.v_addr + bv.v_addr));
   else
      xpush(mkreal(getr(cvr(av)) + getr(cvr(bv))));
   return(1);
}

int o_sub(p)
int *p;
{
   struct value av, bv;
   struct value xpop(), deref(), cvnum(), mkreal(), cvr(), mkint();
   float getr();

   bv = cvnum(deref(xpop()));
   av = cvnum(deref(xpop()));
   if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
      xpush(mkint(av.v_addr - bv.v_addr));
   else
      xpush(mkreal(getr(cvr(av)) - getr(cvr(bv))));
   return(1);
}

int o_mul(p)
int *p;
{
   struct value av, bv;
   struct value xpop(), deref(), cvnum(), mkreal(), cvr(), mkint();
   float getr();

   bv = cvnum(deref(xpop()));
   av = cvnum(deref(xpop()));
   if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
      xpush(mkint(av.v_addr * bv.v_addr));
   else
      xpush(mkreal(getr(cvr(av)) * getr(cvr(bv))));
   return(1);
}

int o_div(p)
int *p;
{
   struct value av, bv;
   struct value xpop(), deref(), cvnum(), mkreal(), cvr(), mkint();
   float getr(), r;

   bv = cvnum(deref(xpop()));
   av = cvnum(deref(xpop()));
   if (TYPE(av) == T_INT && TYPE(bv) == T_INT) {
      if (bv.v_addr == 0)
         err(10, "division by zero", void);
      xpush(mkint(av.v_addr / bv.v_addr));
      }
   else {
      if ((r=getr(cvr(bv))) == 0.0)
         err(10, "division by zero", void);
      xpush(mkreal(getr(cvr(av)) / r));
      }
   return(1);
}

int o_mod(p)
int *p;
{
   struct value av, bv;
   struct value xpop(), deref(), cvi(), mkreal(), cvr(), mkint();
   float getr();

   bv = cvi(deref(xpop()));
   av = cvi(deref(xpop()));
   if (bv.v_addr == 0)
      err(11, "remaindering by zero", void);
   xpush(mkint(av.v_addr % bv.v_addr));
   return(1);
}

int o_cat(p)
int *p;
{
   struct value av, bv, xpop(), deref(), mkstr(), cvstr();
   char *ps, *vstrcat();

   bv = cvstr(deref(xpop()));
   av = cvstr(deref(xpop()));
   ps = vstrcat(av,bv);
   xpush(mkstr(ps, XFIELD(av)+XFIELD(bv)));
   return(1);
}

int o_sstr(p)
int *p;
{
   int i1, i2, i;
   struct value sv, av, bv, xpop(), deref(), mkstr(), cvstr(), cvi();
   char *s, *stralloc();

   bv = cvi(deref(xpop()));
   av = cvi(deref(xpop()));
   sv = cvstr(deref(xpop()));
   s = sv.v_addr;
   i1 = getidx(av.v_addr, XFIELD(sv));
   i2 = getidx(bv.v_addr, XFIELD(sv));
   if (i1 < 0 || i2 < 0)
      return(0);
   if (i2 < i1) {
      i = i1; i1 = i2; i2 = i;
      }
   for (i=1; i<i1; i++) {
      s++;
      if ((s&CHUNKMASK) == 0)
         s = STRTOP(s)->str_next->s_chars;
      }
   if (i2 == i1)
      xpush(mkstr(stralloc("", 0, 1), 0));
   else
      xpush(mkstr(s, i2-i1));
   return(1);
}

int o_idx(p)
int *p;
{
   struct value av, iv, xpop(), deref(), bmap(), gvtbl(), v;
   struct block *cvtbl(), *b;

   iv = deref(xpop());
   v = xpop();
   av = deref(v);
   if (TYPE(iv) == T_INT && iv.v_addr >= 0 && iv.v_addr < MAXIDX)
      xpush(bmap(iv, RVAL, (struct block *)av.v_addr));
   else {
      if ((b=av.v_addr) != NULL && BTYPE(b) == B_ARRAY) {
         av.v_addr = (int) cvtbl(b);
         xpush(v);
         xpush(av);
         o_asgn(p);
         xpop();
         }
      xpush(gvtbl(iv, RVAL, (struct block *)av.v_addr));
      }
   return(1);
}

int o_call(p)
int *p;
{
   struct value deref(), v;
   struct proc *pr;
   int n, i, nxap, (*f)();

   n = *cp++;
   nxap = xsp - n;
   v = deref(val[nxap]);
   if (TYPE(v) != T_PROC)
      err(106, "procedure expected", v);
   ptrace(nxap, n);
   if ((i=XFIELD(v)) > 0) {
      f = (int *) v.v_addr;
      (*f)(n, nxap);
      xsp = nxap;
      ptret(bfname[i], xsp);
      return(1);
      }
   pr = v.v_addr;
   for (i=1; i<=pr->p_nargs; i++)
      if (i<=n)
         val[nxap+i]=deref(val[nxap+i]);
      else
         xpush(void);
   v.v_type = T_VOID;
   v.v_addr = cp;
   xpush(v);
   v.v_type = T_VOID;
   v.v_addr = xap;
   xpush(v);
   v.v_type = T_VOID + pr->p_nlocals;
   v.v_addr = xlp;
   xpush(v);
   xap = nxap;
   xlp = xsp;
   cp = pr->p_code;
   for (i=1; i<=pr->p_nlocals; i++)
      xpush(void);
   return(1);
}

int o_gvar(p)
struct symbol *p;
{
   struct value mkvar();

   xpush(mkvar(&p->y_val));
   return(1);
}

int o_gval(p)
struct symbol *p;
{
   xpush(p->y_val);
   return(1);
}

int o_lvar(p)
struct tlvars *p;
{
   struct value mkvar();

   xpush(mkvar(&val[xlp+p->off]));
   return(1);
}

int o_avar(p)
struct tlvars *p;
{
   struct value mkvar();

   xpush(mkvar(&val[xap+p->off]));
   return(1);
}

int o_jump(p)
int *p;
{
   cp = *cp;
   return(1);
}

int o_link(p)
int *p;
{
   cp--;
   cp = BLKTOP(((int) cp))->c_next->c_code;
   return(1);
}

int o_lt(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) >= 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_le(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) > 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_eq(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) != 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_ne(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) == 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_ge(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) < 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_gt(p)
int *p;
{
   struct value av, bv, xpop();

   bv = deref(xpop());
   av = deref(xpop());
   if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID || valcmp(av, bv) <= 0)
      return(0);
   xpush(bv);
   return(1);
}

int o_jev(p)
int *p;
{
   struct value deref(), xpop();

   if (TYPE((deref(xpop()))) != T_VOID)
      cp = *cp;
   else
      cp = *(++cp);
   return(1);
}

int o_asgn(p)
int *p;
{
   struct value v, a, a1, deref(), xpop();

   v = deref(xpop());
   a = xpop();
   if (TYPE(a) != T_VAR)
      err(107, "variable expected", a);
   if (TYPE(v) == T_VOID)
      return(0);
   a1 = *(struct value *) a.v_addr;
   while (TYPE(a1) == T_VAR) {
      a = a1;
      a1 = *(struct value *) a.v_addr;
      }
   *(struct value *) a.v_addr = v;
   xpush(v);
   return(1);
}

int o_ret(p)
int *p;
{
   struct value xpop(), deref();
   struct proc *pr;
   char *getstr();

   if (xap <= 0)
      longjmp(env, 2);
   pr = deref(val[xap]).v_addr;
   val[xap] = xpop();
   ptret(getstr(pr->p_name), xap);
   xsp = xap;
   cp = val[xlp-2].v_addr;
   xap = val[xlp-1].v_addr;
   xlp = val[xlp].v_addr;
   return(1);
}

int o_neg(p)
int *p;
{
   struct value v, mkint(), mkreal(), cvnum(), deref(), xpop();
   float getr();

   v = cvnum(deref(xpop()));
   if (TYPE(v) == T_INT)
      xpush(mkint(-v.v_addr));
   else
      xpush(mkreal(-getr(v)));
   return(1);
}

int o_binit(p)
int *p;
{
   struct value v, xpop(), deref(), mkint();
   struct block *b;

   v = deref(xpop());
   if (TYPE(v) != T_TABLE)
      return(0);
   o_base(p);
   b = v.v_addr;
   xpush(mkint(0));
   xpush(v);
   val[xlp].v_type += 2;
   return(1);
}

int o_bnext(p)
int *p;
{
   struct value vx, bv, xpop(), *v, mkint(), mkvar(), *pv, bsear(), vv;
   struct block *b, *blk, *bt;
   int n, i, l, j, sz;

   vx = xpop();
   o_base(p);
   bv = xpop();
   if (TYPE(bv) != T_TABLE) {
      cp = *cp;
      return(0);
      }
   b = bv.v_addr;
   n = xpop().v_addr;
   if (BTYPE(b) == B_TABLE) {
      l = 0;
      for (i=0; i<= CHUNKSIZE-2; i++)
         if ((blk=b->b_ints[i]) != NULL) {
            if ((sz=NVALS(blk)) == 0)
               sz = tblsize(blk);
            if (l+sz > n)
               break;
            l += sz;
            }
      n++;
      for (; i<= CHUNKSIZE-2; i++) {
         if (n-l > DBSIZE)
            if ((blk=b->b_ints[i]) != NULL) {
               l += DBSIZE;
               while (n-l > DBSIZE && (blk=blk->b_ints[0]) != NULL)
                  l += DBSIZE;
               }
         if (n-l <= DBSIZE) {
            v = &blk->b_ints[1];
            j = n-l;
            v += (j-1)*2;
            xpush(mkint(n));
            xpush(bv);
            *(struct value *)vx.v_addr = *v;
            cp++;
            return(1);
            }
         }
      }
   if (BTYPE(b) == B_ARRAY) {
      l = 0;
      for (i=0; i<=CHUNKSIZE-4; i++)
         if ((blk=b->b_ints[i]) != NULL && (l=l+NVALS(blk)) > n) {
            vv = bsear(blk, bv, n, l-NVALS(blk), i*DBSIZE);
            *(struct value *)vx.v_addr = vv;
            return(1);
            }
      if ((blk=b->b_ints[CHUNKSIZE-3]) != NULL && (l=l+NVALS(blk)) > n) {
         l -= NVALS(blk);
         for (i=0; i<=CHUNKSIZE-2; i++)
            if ((bt=blk->b_ints[i]) != NULL && (l=l+NVALS(bt)) > n) {
               l -= NVALS(bt);
               vv = bsear(bt, bv, n, l, SINDIR+i*DBSIZE);
               *(struct value *)vx.v_addr = vv;
               return(1);
               }
         }
      if ((blk=b->b_ints[CHUNKSIZE-2]) != NULL) {
         if ((sz=NVALS(blk)) == 0)
            sz = arraysz(blk);
         if (l+sz > n)
            for (i=0; i<=CHUNKSIZE-2; i++)
               if ((blk=blk->b_ints[i]) != 0 && (l=l+NVALS(blk)) > n) {
                  l -= NVALS(blk);
                  for (j=0; j<=CHUNKSIZE-2; j++)
                     if ((bt=blk->b_ints[j]) != NULL && (l=l+NVALS(bt)) > n) {
                        l -= NVALS(bt);
                        vv = bsear(bt,bv,n,l,DINDIR+i*ISIZE+j*DBSIZE);
                        *(struct value *)vx.v_addr = vv;
                        return(1);
                        }
                  }
         }
      }
   *(struct value *)vx.v_addr = void;
   val[xlp].v_type -= 2;
   cp = *cp;
   return(1);
}

int o_ssasn(p)
int *p;
{
   struct value v, sv, s, av, bv, mkint(), deref(), xpop();
   int i1, i2, i;

   v = deref(xpop());
   bv = deref(xpop());
   av = deref(xpop());
   sv = xpop();
   s = deref(sv);
   i1 = getidx(av.v_addr, XFIELD(s));
   i2 = getidx(bv.v_addr, XFIELD(s));
   if (i1 < 0 || i2 < 0)
      return(0);
   if (i2 < i1) {
      i = i1; i1 = i2; i2 = i;
      }
   xpush(sv);
   xpush(s);
   xpush(mkint(1));
   xpush(mkint(i1));
   o_sstr(p);
   xpush(v);
   o_cat(p);
   xpush(s);
   xpush(mkint(i2));
   xpush(mkint(0));
   o_sstr(p);
   o_cat(p);
   o_asgn(p);
   return(1);
}

int o_idxl(p)
int *p;
{
   struct value v, xpop(), deref(), bmap(), av, iv, gvtbl();
   struct block *aballoc(), *cvtbl();

   iv = deref(xpop());
   v = xpop();
   av = deref(v);
   if (TYPE(av) != T_TABLE) {
      av.v_type = T_TABLE;
      if (TYPE(iv) == T_INT && iv.v_addr >= 0 && iv.v_addr < MAXIDX)
         av.v_addr = (int) aballoc(B_ARRAY);
      else
         av.v_addr = (int) aballoc(B_TABLE);
      xpush(v);
      xpush(av);
      o_asgn(p);
      xpop();
      }
   if (TYPE(iv) == T_INT && iv.v_addr >= 0 && iv.v_addr < MAXIDX)
      xpush(bmap(iv, LVAL, (struct block *)av.v_addr));
   else {
      if (BTYPE(((struct block *)av.v_addr)) == B_ARRAY) {
         av.v_addr = (int) cvtbl((struct block *)av.v_addr);
         xpush(v);
         xpush(av);
         o_asgn(p);
         xpop();
         }
      xpush(gvtbl(iv, LVAL, (struct block *)av.v_addr));
      }
   return(1);
}

/* set stack back to base */
int o_base(p)
int *p;
{
   xsp = xlp + XFIELD(val[xlp]);
   return(1);
}
