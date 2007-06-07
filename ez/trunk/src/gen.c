/* ez compiler: code generation */

#include <stdio.h>
#include <stdlib.h>
#include "ez.h"
#include "tokens.h"

char *opname[256] = {		/* operator names */
	"bad",	"add",	"sub",	"mul",	"div",	"mod",	"cat",	"sstr",
	"idx",	"call",	"gvar",	"gval",	"lvar",	"avar",	"jump",	"link",
	"lt",	"le",	"eq",	"ne",	"ge",	"gt",	"jev",	"asgn",
	"ret",	"neg",	"iter",	"sstrl","idxl",	"base",	"deref","cvi",
	"cvr",	"cvnum","cvstr","dup",	"noop",	"lval",	"aval",	"quit",
	"not",	"jnv",	"bev",	"bnv",  "pop",	"err",
	0
};

struct compile c;		/* current compilation variables */
static struct bpnode *freelist = NULL;
extern struct root *rp;

/* alloc - allocate n bytes of memory, die if allocation fails */
char *alloc(n)
int n;
{
	char *p;

	if ((p = malloc(n)) == NULL)
		err("alloc: allocation failed");
	return (p);
}

/* begpoint - beginning of next resumption point */
int begpoint()
{
	extern int tcursor;

	if (++c.point >= sizeof(c.points)/sizeof(struct point))
		err("begpoint: too many resumption points");
	c.points[c.point].pos[0] = tcursor - c.offset;
	c.points[c.point].pos[1] = tcursor - c.offset;
	c.points[c.point].pc = virtaddr(c.pc);
	return (c.point);
}

/* bfree - free storage allocated by alloc */
bfree(p)
char *p;
{
	if (p)
		free(p);
}

/* deflabel - define label lab */
deflabel(lab)
int lab;
{
	struct bpnode *bp, *bp1;
	int *p;

	if (c.labels[lab].addr)
		err("deflabel: label redefinition");
	c.labels[lab].addr = virtaddr(c.pc);
	for (bp = c.labels[lab].bplist; bp; bp = bp1) {
		p = (int *) getblk(bp->addr);
		*p = c.labels[lab].addr;
		putblk(p, 1);
		bp1 = bp->next;
		bp->next = freelist;
		freelist = bp;
		}
	c.labels[lab].bplist = NULL;
}

/* emit - emit n arguments into code block */
emit(n, args)
int n, args;
{
	int *argv;
	struct code *p;

	argv = &args;
	if (c.pc == 0 || c.pc+n >= (int *)(c.cblock+1)) {
		if (c.pc) {
			if (debug&DBEMIT)
				fprintf(stderr, "%06o %s\n", c.pc,
					opname[O_LINK]);
			*c.pc = O_LINK;
			}
		p = (struct code *) balloc(B_CODE);
		p->c_next = 0;
		if (c.cblock) {
			c.cblock->c_next = virtaddr(p);
			putblk(c.cblock, 1);
			}
		c.cblock = p;
		c.pc = p->c_code;	
		}
	while (n--) {
		if (debug&DBEMIT) {
			fprintf(stderr, "%06o ", c.pc);
			if (*argv >= 0 && *argv <= 0377 && opname[*argv])
				fprintf(stderr, "%s\n", opname[*argv]);
			else
				fprintf(stderr, "%06o\n", *argv);
			}
		*c.pc++ = *argv++;
		}
}

/* endpoint - end of resumption point n */
endpoint(n)
int n;
{
	extern int tcursor;
	struct point *pt;

	c.points[n].pos[1] = tcursor - c.offset;
	pt = pindex(c.proc, n, &c.points[n]);
	if (debug&DBEMIT)
		fprintf(stderr, "resumption point %d: pos = %d..%d, pc = %o\n",
			n, pt->pos[0], pt->pos[1], pt->pc);
	putblk(pt, 1);
}

/* genlabel - generate n labels, return first one */
int genlabel(n)
int n;
{
	int i;

	c.label += n;
	if (c.label >= sizeof(c.labels)/sizeof(struct label))
		err("genlabel: too many labels");
	for (i = c.label - n; i < c.label; i++)
		c.labels[i].addr = 0;
	return (c.label - n);
}

/* jump - jump via operator op to label lab */
jump(op, lab)
int lab;
{
	struct bpnode *bp;

	if (c.labels[lab].addr)
		emit(2, op, c.labels[lab].addr);
	else {
		if (bp = freelist)
			freelist = bp->next;
		else
			bp = (struct bpnode *) alloc(sizeof(struct bpnode));
		emit(2, op, 0);
		bp->addr = virtaddr(c.pc - 1);
		bp->next = c.labels[lab].bplist;
		c.labels[lab].bplist = bp;
		}
}

/* procbeg - beginning of procedure id */
struct proc *procbeg(id)
char *id;
{
	struct value *vp;
	struct table *bp;

	c.proc = (struct proc *) balloc(B_PROC);
	c.proc->p_source = VOID;
	c.proc->p_nargs = 0;
	c.proc->p_nlocals = 0;
	bp = (struct table *) balloc(B_TABLE);
	c.proc->p_sym = mkval(T_TABLE, 0, virtaddr(bp));
	putblk(bp, 1);
	vp = tindex(c.proc->p_sym, rp->r_dotdot, 1);
	*vp = rp->r_wglobals;
	putblk(vp, 1);
	if (id)
		c.proc->p_name = stralloc(id, -1);
	else
		c.proc->p_name = VOID;
	c.cblock = (struct code *) balloc(B_CODE);
	c.cblock->c_next = 0;
	c.pc = c.cblock->c_code;
	c.proc->p_code = virtaddr(c.pc);
	c.proc->p_source = VOID;
	c.offset = 0;
	c.label = 1;
	c.loop = 0;
	c.forlevel = 0;
	c.point = 0;
	bfree(id);
	return (c.proc);
}

/* procend - end of procedure p */
int procend(p)
struct proc *p;
{
	extern int ncount;
	int a;

	if (c.cblock) {
		while (c.pc < (int *)(c.cblock+1))
			*c.pc++ = 0;
		putblk(c.cblock, 1);
		c.cblock = NULL;
		c.pc = NULL;
		}
	a = virtaddr(p);
	putblk(p, 0);
	if (debug&DBCODE)
		bdump(a, stderr);
	while (--c.label > 0)
		if (c.labels[c.label].bplist)
			err("procend: undefined label");
/*	fprintf(stderr, "ncount = %d\n", ncount); */
	return (a);
}
