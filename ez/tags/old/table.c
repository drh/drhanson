/* handle array and table mapping */

#include "s.h"
#include "g.h"

struct value bmap(iv, vflg, barray)
int vflg;
struct value iv;
struct block *barray;
{
   struct value mkvar(), gvtbl(), *v;
   struct block *aballoc(), *b, *blk;
   int dindx, ibn, iibn, bn, indx;

   if (barray != NULL && BTYPE(barray) == B_TABLE)
      return(gvtbl(iv, vflg, barray));
   indx = iv.v_addr;
   /* blocks 0..CHUNKSIZE-4 are direct blocks */
   if (indx < SINDIR) {
      bn = indx / DBSIZE;
      dindx = indx % DBSIZE;
      if (vflg == RVAL) {
         if (barray == NULL || (b=barray->b_ints[bn]) == NULL)
            return(void);
         return(mkvar(&b->b_ints[dindx*2]));
         }
      if ((b=barray->b_ints[bn]) == NULL)
         b = barray->b_ints[bn] = (int) aballoc(B_DATA);
      v = &b->b_ints[dindx*2];
      if (TYPE((*v)) == T_VOID)
         b->b_type = b->b_type + 1;
      return(mkvar(v));
      }
   /* block CHUNKSIZE-3 is a single indirect block */
   if (indx < DINDIR) {
      bn = CHUNKSIZE-3;
      ibn = (indx-SINDIR) / DBSIZE;
      dindx = (indx-SINDIR) % DBSIZE;
      if (vflg == RVAL) {
         if (barray == NULL || (b=barray->b_ints[bn]) == NULL ||
             (b=b->b_ints[ibn]) == NULL)
            return(void);
         return(mkvar(&b->b_ints[dindx*2]));
         }
      if ((b=barray->b_ints[bn]) == NULL)
         b = barray->b_ints[bn] = (int) aballoc(B_INDIR);
      if (b->b_ints[ibn] == NULL)
         b->b_ints[ibn] = (int) aballoc(B_DATA);
      b = b->b_ints[ibn];
      v = &b->b_ints[dindx*2];
      if (TYPE((*v)) == T_VOID) {
         b->b_type = b->b_type + 1;
         barray->b_ints[bn]->b_type += 1;
         }
      return(mkvar(v));
      }
   /* block CHUNKSIZE-2 is a double indirect block */
   bn = CHUNKSIZE-2;
   iibn = (indx-DINDIR) / ((CHUNKSIZE-1)*DBSIZE);
   ibn = ((indx-DINDIR)%((CHUNKSIZE-1)*DBSIZE)) / DBSIZE;
   dindx = ((indx-DINDIR)%((CHUNKSIZE-1)*DBSIZE)) % DBSIZE;
   if (vflg == RVAL) {
      if (barray == NULL || (b=barray->b_ints[bn]) == NULL ||
          (b=b->b_ints[iibn]) == NULL || (b=b->b_ints[ibn]) == NULL)
         return(void);
      return(mkvar(&b->b_ints[dindx*2]));
      }
   if ((b=barray->b_ints[bn]) == NULL)
      b = barray->b_ints[bn] = (int) aballoc(B_INDIR);
   if (b->b_ints[iibn] == 0)
      b->b_ints[iibn] = (int) aballoc(B_INDIR);
   b = b->b_ints[iibn];
   if (b->b_ints[ibn] == 0)
      b->b_ints[ibn] = (int) aballoc(B_DATA);
   b = b->b_ints[ibn];
   v = &b->b_ints[dindx*2];
   if (TYPE((*v)) == T_VOID) {
      blk = barray->b_ints[bn];
      if (NVALS(blk)+1 <= MAXNVS)
         blk->b_type += 1;
      else
         blk->b_type -= MAXNVS;
      blk->b_ints[iibn]->b_type += 1;
      b->b_type += 1;
      }
   return(mkvar(v));
}

/* get a block and initialize */
struct block *aballoc(type)
int type;
{
   struct block *b, *balloc();
   int i;

   b = balloc(type);
   for (i=0; i<= CHUNKSIZE-2; i++)
      b->b_ints[i] = 0;
   return(b);
}

/* reorganize array into hash table */
struct block *cvtbl(barray)
struct block *barray;
{
   struct block *b, *aballoc(), *blk, *bt;
   struct value mktbl(), xpop();
   int i, j;

   if (BTYPE(barray) == B_TABLE)
      return(barray);
   b = aballoc(B_TABLE);
   xpush(mktbl(b));
   for (i=0; i<=CHUNKSIZE-4; i++)
      if ((blk=barray->b_ints[i]) != NULL)
         dstore(blk, b, i*DBSIZE);
   if ((blk=barray->b_ints[CHUNKSIZE-3]) != NULL)
      for (i=0; i<=CHUNKSIZE-2; i++)
         if ((bt=blk->b_ints[i]) != NULL)
            dstore(bt, b, SINDIR+i*DBSIZE);
   if ((blk=barray->b_ints[CHUNKSIZE-2]) != NULL)
      for (i=0; i<=CHUNKSIZE-2; i++)
         if (blk->b_ints[i] != 0)
            for (j=0; j<=CHUNKSIZE-2; j++)
               if ((bt=blk->b_ints[i]->b_ints[j]) != NULL)
                  dstore(bt, b, DINDIR+i*ISIZE+j*DBSIZE);
   xpop();
   return(b);
}

/* search thru array data blocks and store values found in hash table */
dstore(blk, ht, indx)
struct block *blk, *ht;
int indx;
{
   int hash, i;
   struct value *v, *efind(), *vt, mkint();
   struct block *h, *aballoc();

   for (i=0; i<=CHUNKSIZE-2; i+=2)
      if (blk->b_ints[i] != 0) {
         v = &blk->b_ints[i];
         h = ht->b_ints[hash=((unsigned)(indx+i/2))%(CHUNKSIZE-1)];
         if (h == NULL)
            h = ht->b_ints[hash] = (int) aballoc(B_TELEM);
         vt = efind(h);
         if (NVALS(h)+1 <= MAXNVS)
            h->b_type = h->b_type + 1;
         else
            h->b_type = h->b_type - MAXNVS;
         *vt = mkint(indx+i/2);
         *++vt = *v;
         }
}

/* find an empty slot */
struct value *efind(h)
struct block *h;
{
   int i;

   for (;;) {
      for (i=1; i<=CHUNKSIZE-7; i+=4)
         if (h->b_ints[i] == T_VOID)
            return((struct value *)(&h->b_ints[i]));
      if (h->b_ints[0] == 0)
         h->b_ints[0] = (int) aballoc(B_TELEM);
      h = h->b_ints[0];
      }
}

/* get value in table given index value */
struct value gvtbl(iv, vflg, btbl)
int vflg;
struct block *btbl;
struct value iv;
{
   struct block *h, *aballoc();
   struct value *v, *vsearch(), mkvar(), *efind();
   int hash;

   if (btbl == NULL)
      return(void);
   h = btbl->b_ints[hash=ghash(iv)];
   if (vflg == RVAL) {
      if ((v=vsearch(h, iv, vflg)) == NULL || TYPE((*v)) == T_VOID)
         return(void);
      return(mkvar(++v));
      }
   xpush(iv);
   if (h == NULL)
      h = btbl->b_ints[hash] = aballoc(B_TELEM);
   v = vsearch(h, iv, vflg);
   if (TYPE((*v)) == T_VOID) {
      *v = iv;
      if (NVALS(h)+1 <= MAXNVS)
         h->b_type = h->b_type + 1;
      else
         h->b_type = h->b_type - MAXNVS;
      }
   xpop();
   return(mkvar(++v));
}

/* get hash value */
int ghash(v)
struct value v;
{
   char *s, *getsstr();
   int hashval;

   if (TYPE(v) != T_STRING)
      return(((unsigned)(v.v_addr))%(CHUNKSIZE-1));
   s = getsstr(v.v_addr, XFIELD(v));
   for (hashval=0; *s!='\0';)
      hashval += *s++;
   return(hashval%(CHUNKSIZE-1));
}


/* search for index value */
struct value *vsearch(b, iv, vflg)
struct block *b;
struct value iv;
int vflg;
{
   int i;
   struct block *aballoc();
   struct value *v;
   float getr();

   for(;;) {
      if (b == NULL)
         return(NULL);
      for (i=1; i<=CHUNKSIZE-7; i+=4) {
         v = &b->b_ints[i];
         if (b->b_ints[i] == T_VOID)
            return(v);
         if (TYPE((*v)) == TYPE(iv)) {
            if (TYPE(iv) == T_INT || TYPE(iv) == T_PROC || TYPE(iv) == T_TABLE)
               if (iv.v_addr == (*v).v_addr)
                  return(v);
            if (TYPE(iv) == T_REAL)
               if (getr(iv) == getr(*v))
                  return(v);
            if (TYPE(iv) == T_STRING)
               if (mstreq(iv, (*v)) > 0)
                  return(v);
            }
         }
      if (vflg == LVAL)
         if (b->b_ints[0] == 0)
            b->b_ints[0] = (int) aballoc(B_TELEM);
      b = b->b_ints[0];
      }
}

/* find size of one hash chain of table */
int tblsize(h)
struct block *h;
{
   struct value *v;
   int n;

   n = 0;
   while (h->b_ints[0] != 0) {
      n += TBSIZE;
      h = h->b_ints[0];
      }
   for (v=(&h->b_ints[1]); TYPE((*v)) != T_VOID; v+=2)
      n++;
   return(n);
}

/* find size of double indirect block for arrays */
int arraysz(h)
struct block *h;
{
   int n, i;

   n = 0;
   for (i=0; i<=CHUNKSIZE-2; i++)
      if (h->b_ints[i] != 0)
         n += NVALS(h->b_ints[i]);
   return(n);
}
