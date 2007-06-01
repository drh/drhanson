/* ez: cache management */

#include <stdio.h>
#include "ez.h"

struct cblock {			/* cache entry */
	int c_addr;		/* virtual address of the block */
	struct cblock *c_hback;	/* hash chain (doubly linked) */
	struct cblock *c_hnext;
	int c_count;		/* reference count */
	int c_dirty;		/* >0 if modified */
	int c_time; 		/* time of last reference */ 
	struct block c_data;	/* data */
};

#define addrtocblock(p) ((struct cblock *)(BLKTOP(p)-sizeof(struct cblock)+sizeof(struct block)))
#define HASH(a) (((int)a)/sizeof(struct block)%CHASHSIZE)

int fd = -1;			/* file descriptor of opened block file */
struct cblock ctab[CHASHSIZE];	/* hash table */
int timer = 0; 			/* lru timer */
int cachesize = 0;		/* number of blocks currently in the cache */
int cachelimit = 100;		/* limit on cachesize */
char *bname[] = {"?", "proc", "code", "string", "array", "data", "indir",
	"table", "telem", "points", "frame", "free"};
int filesize;			/* file system size */
struct root *rp;		/* pointer to root block */

/* initcache - initialize cache management */
initcache(fname)
char *fname;
{
	register int i;

	if ((fd = open(fname, 2)) == -1) {
		fprintf(stderr, "%s: can't open\n", fname);
		exit(1);
		}
	filesize = lseek(fd, 0, 2);
	if (filesize == 0)
		filesize = sizeof(struct block);
	for (i = 0; i < CHASHSIZE; i++) {
		ctab[i].c_addr = -1;
		ctab[i].c_hnext = ctab[i].c_hback = &ctab[i];
		}
	timer = cachesize = 0;
}

/* savecache - save state of the cache */
savecache()
{
	register int i;
	register struct cblock *p;

	for (i = 0; i < CHASHSIZE; i++)
		for (p = ctab[i].c_hnext; p->c_addr >= 0; p = p->c_hnext)
			if (p->c_dirty)
				cwrite(p);
}

/* expand - expand the cache by adding a block with address a */
static struct cblock *expand(a)
int a;
{
	register struct cblock *p;
	struct cblock *replace();
	register int i;

	if (cachesize > cachelimit)
		p = replace();
	else
		p = (struct cblock *) malloc(2*sizeof(struct cblock));
	if (p == NULL)
		p = replace();
	else 
		p = addrtocblock((int)&p->c_data + CHUNKMASK);
	if (p == NULL)
		err("expand: out of storage");
	i = HASH(a);
	p->c_addr = a;
	p->c_hnext = ctab[i].c_hnext;
	ctab[i].c_hnext->c_hback = p;
	p->c_hback = &ctab[i];
	ctab[i].c_hnext = p;
	p->c_count = 1;
	p->c_dirty = 0;
	p->c_time = ++timer;
	cachesize++;
	return (p);
}

/* balloc - allocate a new block */
struct block *balloc(type)
int type;
{
	register struct cblock *p;
	register int i;

	if (rp->r_freelist) {
		p = addrtocblock(getblk(rp->r_freelist));
		rp->r_freelist = p->c_data.b_ints[0];
		touch(rp);
		}
	else {
		p = expand(filesize);
		filesize += sizeof(struct block);
		}
	p->c_data.b_type = type;
	if (debug&DBCACHE)
		bprint(p, "allocating");
	for (i = 0; i < sizeof(p->c_data.b_ints)/sizeof(int); i++)
		p->c_data.b_ints[i] = 0;
	p->c_dirty++;
	return (&p->c_data);
}

/* salloc - allocate a new string block, return virtual address of 1st char */
int salloc()
{
	register struct string *q;
	int a;

	q = (struct string *) balloc(B_STRING);
	a = virtaddr(q->s_chars);
	putblk(q, 0);
	return (a);
}

/* getblk - get block containing virtual address a */
struct block *getblk(a)
int a;
{
	register int b;
	register struct cblock *p;

	b = BLKTOP(a);
	for (p = ctab[HASH(b)].c_hnext; p->c_addr >= 0; p = p->c_hnext)
		if (p->c_addr == b) {
			p->c_count++;
			p->c_time = ++timer;  
			return ((struct block *)((int)&p->c_data + (a&CHUNKMASK)));
			}
	p = expand(b);
	cread(b, p);
	return ((struct block *)((int)&p->c_data + (a&CHUNKMASK)));
}

/* replace - replace oldest cache entry */
static struct cblock *replace()
{
	register int i;
	register struct cblock *q, *p;

	p = &ctab[0];
	p->c_time = timer + 1;
	for (i = 0; i < CHASHSIZE; i++)
		for (q = ctab[i].c_hnext; q->c_addr >= 0; q = q->c_hnext)
			if (q->c_count == 0 && q->c_time < p->c_time)
				p = q;
	if (p == &ctab[0])
		err("replace: cache overflow");
	if (p->c_dirty)
		cwrite(p);
	if (debug&DBCACHE)
		bprint(p, "replacing");
	p->c_hback->c_hnext = p->c_hnext;
	p->c_hnext->c_hback = p->c_hback;
	cachesize--;
	return (p);
}

/* touch - `touch' the given cache block */
touch(bp)
struct block *bp;
{
	register struct cblock *p;

	p = addrtocblock(bp);
	p->c_dirty++;
	p->c_time = ++timer;
}

/* putblk - release cache block; dirty = 1 indicates modified block */
putblk(bp, dirty)
struct block *bp;
int dirty;
{
	register struct cblock *p;

	p = addrtocblock(bp);
	if (--p->c_count < 0)
		err("putblk: negative reference count");
	if (dirty)
		p->c_dirty++;
}

/* cread - read block at virtual address into cache entry p */
static cread(a, p)
int a;
struct cblock *p;
{
	lseek(fd, a, 0);							 /* read block from disk */
	read(fd, &p->c_data, sizeof(struct block));
	p->c_dirty = 0;
	if (debug&DBCACHE)
		bprint(p, "swapping in");
}

/* cwrite - write cache entry p */
static cwrite(p)
struct cblock *p;
{
	
	lseek(fd, p->c_addr, 0);
	write(fd, &p->c_data, sizeof(struct block));
	p->c_dirty = 0;
	if (debug&DBCACHE)
		bprint(p, "swapping out");
}

/* cdump - print current cache contents */
cdump()
{
	register int i;
	register struct cblock *p;
	char buf[TOKSIZE];

	fprintf(stderr, "cachesize = %d, cachelimit = %d\n", cachesize,
		cachelimit);
	for (i = 0; i < CHASHSIZE; i++)
		for (p = ctab[i].c_hnext; p->c_addr >= 0; p = p->c_hnext) {
			sprintf(buf, "count %d,", p->c_count);
			bprint(p, buf);
			}
}

static bprint(p, s)
struct cblock *p;
char *s;
{
	fprintf(stderr, "[%s block %d, type ", s,
		p->c_addr/sizeof(struct block));
	if (p->c_data.b_type > 0 &&
		p->c_data.b_type <= sizeof(bname)/sizeof(char *))
			fprintf(stderr, "%s]\n", bname[p->c_data.b_type]);
	else
		fprintf(stderr, "%s = %d]\n", bname[0], p->c_data.b_type);
}

/* virtaddr - convert address in cache to virtual address */
int virtaddr(q)
char *q;
{
	register struct cblock *p;

	p = addrtocblock(q);
	return (p->c_addr + ((int)q&CHUNKMASK));
}
