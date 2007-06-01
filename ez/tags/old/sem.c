/* semantics */

#include "s.h"
#include "g.h"

/* beginning of procedure declaration */
struct symbol *prochead(pnam, nargs, nlocs)
char *pnam;
int nargs, nlocs;
{
   struct proc *p;
   struct symbol *symptr, *clookup();
   struct block *balloc();

   p = balloc(B_PROC);
   p->p_next = NULL;
   if (nargs > MAXLOC || nlocs > MAXLOC)
      printf("too many locals or args: max = %d\n", MAXLOC);
   p->p_nargs = nargs;
   p->p_nlocals = nlocs;
   p->p_name = pnam;
   symptr = clookup(p, S_GLOBAL);
   emit(symptr, 0, 0, 1, "procbody");
   emit(&op[O_ASGN], 0, 0, 1, "asgn");
   symptr->y_val.v_type = T_PROC;
   symptr->y_val.v_addr = (int) p;
   if (sp >= LEVMAX)
      yyerror("procedures nested too deep");
   else {
      cpstor[sp] = cp;
      blkstor[sp++] = curblk;
      }
   cp = p->p_code;
   curblk = p;
   return(symptr);
}

/* procedure name */
procnam(id)
char *id;
{
   struct symbol *p, *lookup(), *install();

   if ((p=lookup(id)) == 0)
      p = install(id, S_GLOBAL);
   if (p->y_scope == S_GLOBAL)
      emit(p, 0, 0, 1, "gvar");
   else
      emit(&lvars[p->y_loff], 0, 0, 1, "lvar");
   level++;
   loff = LOFF;
   aoff = AOFF;
}

/* formal parameter */
param(id)
char *id;
{
   struct symbol *p, *lookup(), *install();

   if ((p=lookup(id)) != 0)
      if (p->y_scope == level) {
         yyerror("parameter previously declared");
         return;
        }
   p = install(id, S_PARAM);
   aoff += 1;
}

/* local declaration */
decl(id)
char *id;
{
   struct symbol *p, *lookup(), *install();

   if ((p=lookup(id)) != 0)
      if (p->y_scope == level && p->y_loff > AOFF) {
         yyerror("identifier previously declared");
         return;
         }
   p = install(id, S_LOCAL);
   loff += 1;
}

/* procedure end */
procend(q)
struct symbol *q;
{
   remove(level--);
   bflush();
   if (debug)
      cdump(q->y_val.v_addr);
   if (sp) {
      cp = cpstor[--sp];
      curblk = blkstor[sp];
      }
}

/* enumerating tables */
struct bplist *bang()
{
   struct symbol *idptr, *lookup(), *install();
   struct bplist *initbpl();
   int **cp1;

   emit(&op[O_BINIT], 0, 0, 1, "binit");
   if ((idptr=lookup(ident1)) == 0)
      idptr = install(ident1, S_GLOBAL);
   if (idptr->y_scope == S_GLOBAL)
      emit(idptr, 0, 0, 1, "gvar");
   else
      emit(&lvars[idptr->y_loff], 0, 0, 1, "lvar");
   cp1 = cp - 1;
   emit(&op[O_BNEXT], 0, 0, 2, "bnext");
   return (initbpl(cp1, cp - 1));
}

/* statement list */
struct stlist *stmtl(slst, m, s)
struct stlist *slst, *s;
int **m;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   backpatch(slst->s_next, m);
   rl = mklist(s->s_next, merge(slst->s_break, s->s_break),
          merge(slst->s_succ, s->s_succ));
   bfree((char *) s); bfree((char *) slst);
   return(rl);
}

/* if statement */
struct stlist *ifstmt(e, m, s)
struct bplist *e;
int **m;
struct stlist *s;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   backpatch(e->true, m);
   rl = mklist(merge(e->false, s->s_next), s->s_break, s->s_succ);
   bfree((char *) e); bfree((char *) s);
   return(rl);
}

/* if-else statement */
struct stlist *ifelse(e, m1, s1, n, m2, s2)
struct bplist *e, *n;
int **m1, **m2;
struct stlist *s1, *s2;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   backpatch(e->true, m1);
   backpatch(e->false, m2);
   rl = mklist(merge(merge(s1->s_next, n->true), s2->s_next),
      merge(s1->s_break, s2->s_break), merge(s1->s_succ, s2->s_succ));
   bfree((char *) e); bfree((char *) n);
   bfree((char *) s1); bfree((char *) s2);
   return(rl);
}

/* while statement */
struct stlist *whilest(m1, e, m2, s)
struct bplist *e;
int **m1, **m2;
struct stlist *s;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   backpatch(e->true, m2);
   backpatch(s->s_next, m1);
   backpatch(s->s_succ, m1);
   emit(&op[O_JUMP], m1, 0, 2, "jump");
   rl = mklist(merge(e->false, s->s_break), NULL, NULL);
   bfree((char *) e); bfree((char *) s);
   return(rl);
}

/* for statement */
struct stlist *forstmt(e1, m1, e2, m2, e3, m3, s)
struct bplist *e1, *e2, *e3;
int **m1, **m2, **m3;
struct stlist *s;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   if (e2 != 0)
      backpatch(e2->true, m3);
   backpatch(s->s_next, m2);
   backpatch(s->s_succ, m2);
   if (e3 != 0)
      backpatch(e3->true, m1);
   emit(&op[O_JUMP], m2, 0, 2, "jump");
   if (e2 != 0)
      rl = mklist(merge(e2->false, s->s_break), NULL, NULL);
   else
      rl = mklist(NULL, s->s_break, NULL);
   bfree((char *) e1); bfree((char *) e2); bfree((char *) e3);
   bfree((char *) s);
   return(rl);
}

/* table enumeration */
struct stlist *forin(bng, s)
struct bplist *bng;
struct stlist *s;
{
   struct stlist *mklist(), *rl;
   struct lelem *merge();

   backpatch(s->s_next, bng->true->bptr);
   backpatch(s->s_succ, bng->true->bptr);
   emit(&op[O_JUMP], bng->true->bptr, 0, 2, "jump");
   rl = mklist(merge(bng->false, s->s_break), NULL, NULL);
   bfree((char *) bng); bfree((char *) s);
   return(rl);
}

/* variable reference */
var(id)
char *id;
{
   struct symbol *idptr, *lookup(), *install();

   if ((idptr=lookup(id)) == 0)
      idptr = install(id, S_GLOBAL);
   if (idptr->y_scope == S_GLOBAL)
      emit(idptr, 0, 0, 1, "gvar");
   else
      emit(&lvars[idptr->y_loff], 0, 0, 1, "lvar");
}

/* constants */
struct symbol *const(id)
char *id;
{
   struct symbol *clookup();

   return(clookup(id, ctype, lensav));
}

/* backpatch list starting at h to ptr */
backpatch(h, ptr)
struct lelem *h;
int **ptr;
{
   struct lelem *pptr;

   while (h != NULL) {
      *h->bptr = (int *) ptr;
      pptr = h;
      h = h->link;
      bfree((char *) pptr);
      }
}

/* merge backpatch lists 1 & 2 */
struct lelem *merge(list1, list2)
struct lelem *list1, *list2;
{
   struct lelem *hlist;

   if (list1 == NULL)
      return(list2);
   if (list2 == NULL)
      return(list1);
   hlist = list1;
   while (list1->link != NULL)
      list1=list1->link;
   list1->link = list2;
   return(hlist);
}

/* make a true/false backpatch list from given lists */
struct bplist *mkbpl(ltrue,lfalse)
struct lelem *ltrue, *lfalse;
{
   struct bplist *h;
   char *malloc();

   h = (struct bplist *) malloc(sizeof(struct bplist));
   h->true = ltrue;
   h->false = lfalse;
   return(h);
}

/* initially create a true/false backpatch list */
struct bplist *initbpl(itrue,ifalse)
int **itrue, **ifalse;
{
   struct bplist *h;
   struct lelem *ht, *hf;
   char *malloc();

   if (itrue != NULL) {
      ht = (struct lelem *) malloc(sizeof(struct lelem));
      ht->link = NULL;
      ht->bptr = itrue;
      }
   else
      ht = NULL;
   if (ifalse != NULL) {
      hf = (struct lelem *) malloc(sizeof(struct lelem));
      hf->link = NULL;
      hf->bptr = ifalse;
      }
   else
      hf = NULL;
   h = (struct bplist *) malloc(sizeof(struct bplist));
   h->true = ht;
   h->false = hf;
   return(h);
}

/* make a next/break/succ backpatch list from given lists */
struct stlist *mklist(lnext, lbreak, lsucc)
struct lelem *lnext, *lbreak, *lsucc;
{
   struct stlist *h;
   char *malloc();

   h = (struct stlist *) malloc(sizeof(struct stlist));
   h->s_next = lnext;
   h->s_break = lbreak;
   h->s_succ = lsucc;
   return(h);
}

/* initially create a next/break/succ backpatch list */
struct stlist *initlst(inext, ibreak, isucc)
int **inext, **ibreak, **isucc;
{
   struct stlist *h;
   struct lelem *hn, *hb, *hs;
   char *malloc();

   if (inext != NULL) {
      hn = (struct lelem *) malloc(sizeof(struct lelem));
      hn->link = NULL;
      hn->bptr = inext;
      }
   else
      hn = NULL;
   if (ibreak != NULL) {
      hb = (struct lelem *) malloc(sizeof(struct lelem));
      hb->link = NULL;
      hb->bptr = ibreak;
      }
   else
      hb = NULL;
   if (isucc != NULL) {
      hs = (struct lelem *) malloc(sizeof(struct lelem));
      hs->link = NULL;
      hs->bptr = isucc;
      }
   else
      hs = NULL;
   h = (struct stlist *) malloc(sizeof(struct stlist));
   h->s_next = hn;
   h->s_break = hb;
   h->s_succ = hs;
   return(h);
}

/* free backpatch list element */
bfree(p)
char *p;
{
   if (p)
      free(p);
}

/* emit given arguments onto code blocks */
emit(e1, e2, e3, n, str)
int n;
int *e1, *e2, *e3;
char *str;
{
   struct code *p, *cballoc();

   if (cp+n >= curblk+1) {
      if (trace)
         printf("  link\n");
      *cp = (int *)(&op[O_LINK]);
      p = cballoc();
      curblk->c_next = p;
      curblk = p;
      }
   if (trace)
      printf("  %s\n", str);
   *cp++ = e1;
   if (n > 1)
      *cp++ = e2;
   if (n > 2)
      *cp++ = e3;
}

/* allocate a code block */
struct code *cballoc()
{
   struct code *p;
   struct block *balloc();

   p = balloc(B_CODE);
   p->c_next = NULL;
   cp = p->c_code;
   return(p);
}

/* flush rest of block */
bflush()
{
   while (cp < curblk+1)
      *cp++ = 0;
}
