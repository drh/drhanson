/* storage allocation */

#include "s.h"
#include "g.h"

struct block *freelist;                /* ptr to head of freelist */
struct bheap *blocklist;               /* ptr to head of list of all blocks */


/* initialize storage allocator */
initalloc()
{
   blocklist = NULL;
   freelist = NULL;
}

/* return a free block - garbage collect & allocate more blks if neccessary */
struct block *balloc(type)
int type;
{
   struct block *b, *t;
   char *malloc();
   struct bheap *p;
   int i;

   if (freelist == NULL && blocklist != NULL && gcoff != 1)
      gc();
   if (freelist == NULL) {
      p = (struct bheap *) malloc(sizeof(struct bheap));
      p->h_list = blocklist;
      blocklist = p;
      freelist = t = b = (((int)p->h_slot+CHUNKMASK)&(~CHUNKMASK));
      for (i=0; b<freelist+(NBLOCK-1); b++) {
         i++;
         b->b_type = B_FREE;
         t->b_ints[0] = (int) b;
         b->b_ints[0] = NULL;
         t = b;
         }
      p->h_size = i;
      }
   b = freelist;
   freelist = (struct block *) b->b_ints[0];
   b->b_type = type;
   b->b_ints[0] = NULL;
   return(b);
}

/* garbage collector */
gc()
{
   int i;
   struct symbol *p;
   struct bheap *b;
   struct block *blk;

   for (i=0; i<=xsp; i++)
      vmark(val[i]);
   for (i=0; i<SYMSIZE; i++)
      for (p = symtab[i]; p != NULL; p = p->slink) {
         vmark(p->y_val);
         bmark(BLKTOP(p->y_name));
         }
   for (i=0; i<SYMSIZE; i++)
      for (p = constab[i]; p != NULL; p = p->slink) {
         vmark(p->y_val);
         bmark(BLKTOP(p->y_name));
         }
   bmark(BLKTOP(istr));
   bmark(fstblk);
   bmark(curblk);
   for (i=0; i<sp; i++)
      bmark(blkstor[i]);
   for (b=blocklist; b != NULL; b = b->h_list) {
      blk = (struct block *) (((int)b->h_slot+CHUNKMASK)&(~CHUNKMASK));
      for (i=1; i<=b->h_size; i++) {
         if (BTYPE(blk) != B_FREE)
            if (MARK(blk) == 0) {
               blk->b_type = B_FREE;
               blk->b_ints[0] = (int) freelist;
               freelist = blk;
               }
            else
               blk->b_type = UNMARK(blk);
         blk++;
         }
      }
}

bmark(p)
struct block *p;
{
   int i;
   struct value *v;

   if (p == NULL)
      return;
   if (MARK(p) != B_MARK) {
      p->b_type += B_MARK;
      switch (BTYPE(p)) {
         case B_STRING:
         case B_CODE:
         case B_PROC:
            bmark(p->p_next);
            break;
         case B_TABLE:
         case B_ARRAY:
         case B_INDIR:
            for (i=0; i<=CHUNKSIZE-2; i++)
               if (p->b_ints[i] != 0)
                  bmark((struct block *)p->b_ints[i]);
            break;
         case B_TELEM:
            if (p->b_ints[0] != 0)
               bmark((struct block *)p->b_ints[0]);
            for (v=(&p->b_ints[1]); TYPE((*v)) != T_VOID; v+=2) {
               vmark(*v);
               vmark(*(v+1));
               }
            break;
         case B_DATA:
            for (i=0; i<=CHUNKSIZE-2; i+=2) {
               v = &p->b_ints[i];
               if (TYPE((*v)) != T_VOID)
                  vmark(*v);
               }
            break;
         }
   }
}

/* mark values if necessary */
vmark(v)
struct value v;
{
   if (TYPE(v) == T_STRING || (TYPE(v) == T_PROC &&
       XFIELD(v) == 0) || TYPE(v) == T_TABLE)
      bmark(BLKTOP(v.v_addr));
}
