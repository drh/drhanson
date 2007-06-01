/* ez: off-line garbage collector */

#include <stdio.h>
#include <ctype.h>
#include "ez.h"
#include <signal.h>

#define DBBMARK  0002	/* trace bmark */
#define DBADJUST 0004	/* trace adjust */

struct mmap {
	int m_size; 			/* number of blocks */
	int m_marked;			/* number of marked blocks */
	int *m_map; 			/* new address/marked indicator */
} map;
int sflag;				/* >0 for statistics output */
int cflag;				/* >0 for compaction */
int errcnt; 				/* number of errors */
int debug;
char *progname = "";
extern int cachelimit;
extern int filesize;
extern struct root *rp;

/* reclaim inaccessible space in ez file systems */
main(argc, argv)
int argc;
char *argv[];
{
	int i, nf;

	progname = *argv;
	debug = 0;
	sflag = 0;
	cachelimit = 100;
	nf = 0;
	for (i = 1; i < argc; i++)
		if (strncmp(argv[i], "-D", 2) == 0)
			if (isdigit(argv[i][2]))
				debug |= 1<<(atoi(&argv[i][2])-1);
			else
				debug = 0;
		else if (strncmp(argv[i], "-C", 2) == 0)
			if (argv[i][2])
				cachelimit = atoi(&argv[i][2]);
			else
				cachelimit = 100;
		else if (strncmp(argv[i], "-s", 2) == 0)
			sflag = 1;
		else if (strncmp(argv[i], "-S", 2) == 0)
			sflag = 2;
		else if (strncmp(argv[i], "-c", 2) == 0)
			cflag = 1;
		else {
			collect(argv[i]);
			nf++;
			}
	if (nf == 0)
		collect(FILESYS);
	exit(0);
}

/* collect - garbage collect ez file name */
static collect(name)
char *name;
{
	FILE *fopen(), *f;
	int i;

	if (access(name, 2)) {
		fprintf(stderr, "%s: can't open\n", name);
		return;
		}
	errcnt = 0;
	initcache(name);
	rp = (struct root *) getblk(0);
	if (rp->r_major != MAJOR) {
		fprintf(stderr, "%s: version mismatch; %s version is %d.%d, \
system version is %d.%d\n", progname, name, rp->r_major, rp->r_minor,
			MAJOR, MINOR);
		errcnt++;
		return;
		}
	map.m_size = filesize/sizeof(struct block);
	map.m_map = (int *) malloc(map.m_size*sizeof(int));
	if (map.m_map == NULL) {
		fprintf(stderr, "can't allocate map\n");
		errcnt++;
		return;
		}
	for (i = 0; i < map.m_size; i++)
		map.m_map[i] = 0;
	map.m_map[0] = -1;
	map.m_marked = 1;
	vmark(rp->r_globals);
	vmark(rp->r_wglobals);
	vmark(rp->r_dotdot);
	stats(name);
	if (cflag) {
		savecache();
		compact(name);
		}
	else {
		rp->r_freelist = freelist();
		savecache();
		putblk(rp, 1);
		}
	free(map.m_map);
}

/* bmark - mark block virtual address a and its successors */
static bmark(a)
int a;
{
	int i, bn;
	struct block *bp;
	char *btype();

top:	if (a == 0)
		return;
	bn = a/sizeof(struct block);
	if (bn >= map.m_size) {
		fprintf(stderr, "bad block %d\n", bn);
		errcnt++;
		return;
		}
	if (map.m_map[bn])
		return;
	map.m_map[bn] = -1;
	map.m_marked++;
	bp = (struct block *) BLKTOP(getblk(a));
	if (debug&DBBMARK)
		fprintf(stderr, "marking block %d, type %s\n", bn, btype(bp));
	switch (bp->b_type) {
		case B_CODE: { struct code *p = (struct code *) bp;
			for (i = 0; i < sizeof(p->c_code)/sizeof(int); i++)
				if (p->c_code[i] > 0377)
					bmark(p->c_code[i]);
			a = p->c_next;
			putblk(p, 0);
			goto top; }
		case B_PROC: { struct proc *p = (struct proc *) bp;
			vmark(p->p_name);
			vmark(p->p_sym);
			vmark(p->p_source);
			bmark(p->p_code);
			for (i = 0; i < sizeof(p->p_points)/sizeof(int); )
				bmark(p->p_points[i++]);
			break; }
		case B_TABLE: { struct table *p = (struct table *) bp;
			for (i = 0; i < sizeof(p->t_elem)/sizeof(int); )
				bmark(p->t_elem[i++]);
			break; }
		case B_ARRAY: { struct array *p = (struct array *) bp;
			for (i = 0; i < sizeof(p->a_indir)/sizeof(int);)
				bmark(p->a_indir[i++]);
			break; }
		case B_INDIR: { struct indir *p = (struct indir *) bp;
			for (i = 0; i < sizeof(p->i_data)/sizeof(int); )
				bmark(p->i_data[i++]);
			break; }
		case B_STRING:
			break;
		case B_TELEM: { struct elem *p = (struct elem *) bp;
			for (i = 0; i < sizeof(p->e_vals)/sizeof(struct value); )
				vmark(p->e_vals[i++]);
			a = p->e_next;
			putblk(p, 0);
			goto top; }
		case B_DATA: { struct data *p = (struct data *) bp;
			for (i = 0; i < sizeof(p->d_vals)/sizeof(struct value); )
				vmark(p->d_vals[i++]);
			break; }
		case B_POINTS: { struct points *p = (struct points *) bp;
			for (i = 0; i < sizeof(p->n_points)/sizeof(struct point); )
				bmark(p->n_points[i++].pc);

			break; }
		case B_FRAME: { struct frame *p = (struct frame *) bp;
			bmark(p->f_prev);
			bmark(p->f_pc);
			vmark(p->f_table);
			for (i = 0; i < sizeof(p->f_vals)/sizeof(struct value); )
				vmark(p->f_vals[i++]);
			break; }
		case B_BLOCK:
			break;
		default:
			fprintf(stderr, "bmark: bad block %d, type %s\n", bn,
				btype(bp)); 
			errcnt++;
		}
	putblk(bp, 0);
}

/* vmark - mark blocks pointed to by value v */
static vmark(v)
struct value v;
{
	int a, len;
	struct string *p;

	switch (TYPE(v)) {
		case T_VOID: case T_INT: case T_REAL:
			break;
		case T_VAR: 		/* bug? */
			if (XFIELD(v) == VIRTUAL)
				bmark(v.v_addr);
			break;
		case T_STRING:
			if (XFIELD(v) == 0)
				break;
			len = XFIELD(v) - (sizeof(struct block) -
				(v.v_addr&CHUNKMASK));
			bmark(v.v_addr);
			a = BLKTOP(v.v_addr);
			for ( ; len > 0; len -= sizeof(p->s_chars)) {
				p = (struct string *) getblk(a);
				a = p->s_next;
				putblk(p, 0);
				bmark(a);
				}
			break;
		case T_PROC: case T_TABLE:
			bmark(v.v_addr);
		}
}

/* compact - compact and copy file */
static compact(name)
char *name;
{
	int i, size, fd;
	char temp[TOKSIZE], *mktemp();
	struct block *p;
	struct value vadjust();

	if (errcnt)
		return;
	strcpy(temp, "ezXXXXXX");
	fd = creat(mktemp(temp), 0666);
	if (fd == -1) {
		fprintf(stderr, "%s: can't create\n", temp);
		errcnt++;
		return;
		}
	size = sizeof(struct block);
	for (i = 1; i < map.m_size; i++)  /* compute new block locations */
		if (map.m_map[i]) {
			map.m_map[i] = size;
			size += sizeof(struct block);
			}
	map.m_map[0] = 0;
	for (i = 1; i < map.m_size; i++)  /* adjust pointers, write new file */
		if (map.m_map[i]) {
			p = (struct block *) getblk(i*sizeof(struct block));
			if (debug&DBADJUST)
				fprintf(stderr,	"adjusting block %d, \
type %d, new address %d\n", i, btype(p), map.m_map[i]/sizeof(struct block));
			adjust(p, i);
			lseek(fd, map.m_map[i], 0);
			write(fd, p, sizeof(struct block));
			putblk(p, 0);
			}
	rp->r_globals = vadjust(rp->r_globals);
	rp->r_wglobals = vadjust(rp->r_wglobals);
	rp->r_dotdot = vadjust(rp->r_dotdot);
	lseek(fd, 0, 0);
	write(fd, rp, sizeof(struct block));
	close(fd);
	if (errcnt == 0) {
		unlink(name);
		link(temp, name);
		unlink(temp);
		}
}

#define NEWADDR(p) (map.m_map[((int)p)/sizeof(struct block)] + (((int)p)&CHUNKMASK))

/* adjust - adjust pointers in block n at bp */
static adjust(bp, n)
struct block *bp;
int n;
{
	int i;

	switch (bp->b_type) {
		case B_CODE: { struct code *p = (struct code *) bp;
			p->c_next = NEWADDR(p->c_next);
			for (i = 0; i < sizeof(p->c_code)/sizeof(int); i++)
				if (p->c_code[i] > 0377)
					p->c_code[i] = NEWADDR(p->c_code[i]);
			break; }
		case B_PROC: { struct proc *p = (struct proc *) bp;
			p->p_name = vadjust(p->p_name);
			p->p_sym = vadjust(p->p_sym);
			p->p_source = vadjust(p->p_source);
			p->p_code = NEWADDR(p->p_code);
			for (i = 0; i < sizeof(p->p_points)/sizeof(int); i++)
				p->p_points[i] = NEWADDR(p->p_points[i]);
			break; }
		case B_TABLE: { struct table *p = (struct table *) bp;
			for (i = 0; i < sizeof(p->t_elem)/sizeof(int); i++)
				p->t_elem[i] = NEWADDR(p->t_elem[i]);
			break; }
		case B_ARRAY: { struct array *p = (struct array *) bp;
			for (i = 0; i < sizeof(p->a_indir)/sizeof(int); i++)
				p->a_indir[i] = NEWADDR(p->a_indir[i]);
			break; }
		case B_INDIR: { struct indir *p = (struct indir *) bp;
			for (i = 0; i < sizeof(p->i_data)/sizeof(int); i++)
				p->i_data[i] = NEWADDR(p->i_data[i]);
			break; }
		case B_STRING: { struct string *p = (struct string *) bp;
			p->s_next = NEWADDR(p->s_next);
			break; }
		case B_TELEM: { struct elem *p = (struct elem *) bp;
			p->e_next = NEWADDR(p->e_next);
			for (i = 0; i < sizeof(p->e_vals)/sizeof(struct value); i++)
				p->e_vals[i] = vadjust(p->e_vals[i]);
			break; }
		case B_DATA: { struct data *p = (struct data *) bp;
			for (i = 0; i < sizeof(p->d_vals)/sizeof(struct value); i++)
				p->d_vals[i] = vadjust(p->d_vals[i]);
			break; }
		case B_POINTS: { struct points *p = (struct points *) bp;
			for (i = 0; i < sizeof(p->n_points)/sizeof(struct point); i++)
				p->n_points[i].pc = NEWADDR(p->n_points[i].pc);
			break; }
		case B_FRAME: { struct frame *p = (struct frame *) bp;
			p->f_prev = (struct frame *) NEWADDR(p->f_prev);
			p->f_pc = NEWADDR(p->f_pc);
			p->f_table = vadjust(p->f_table);
			for (i = 0; i < sizeof(p->f_vals)/sizeof(struct value); i++)
				p->f_vals[i] = vadjust(p->f_vals[i]);
			break; }
		case B_BLOCK:
			break;
		default:
			fprintf(stderr, "adjust: bad block %d, type %s\n", n,
				btype(bp)); 
			errcnt++;
		}
}

/* vadjust - adjust virtual address in value v */
static struct value vadjust(v)
struct value v;
{
	switch (TYPE(v)) {
		case T_VAR: 		/* bug? */
			if (XFIELD(v) == VIRTUAL)
				v.v_addr = NEWADDR(v.v_addr);
			break;
		case T_STRING: case T_PROC: case T_TABLE:
			v.v_addr = NEWADDR(v.v_addr);
		}
	return (v);
}

/* freelist - construct new free list */
static int freelist()
{
	int i, h, n;
	struct block *p;

	h = n = 0;
	for (i = map.m_size - 1; i > 0; i--)
		if (map.m_map[i] == 0) {
			p = getblk(i*sizeof(struct block));
			p->b_type = B_FREE;
			p->b_ints[0] = h;
			putblk(p, 1);
			h = i*sizeof(struct block);
			n++;
			}
	if (n != map.m_size - map.m_marked) {
		fprintf(stderr, "bad free block count: %d instead of %d\n", n,
			map.m_size - map.m_marked);
		errcnt++;
		h = 0;
		}
	return (h);
}

/* err - issue error message s */
err(n, s)
char *s;
{
	errcnt++;
	if (index(s, ':')) {
		fprintf(stderr, "system error in %s\n", s);
		savecache();
		exit(1);
		}
	fprintf(stderr, "%s\n", s);
}

/* stats - print reclamation statistics */
static stats(name)
char *name;
{
	int i, n;

	if (sflag < 1)
		return;
	fprintf(stdout, "%s: %d blocks, %d free, %d busy\n", name,
		map.m_size, map.m_size - map.m_marked, map.m_marked);
	if (sflag < 2) 
		return;
	for (i = 0; i < map.m_size; ) {
		fprintf(stdout, "%s:\t", map.m_map[i]?"busy":"free");
		n = i++;
		for ( ; i < map.m_size && map.m_map[i] == map.m_map[n]; i++);
		fprintf(stdout, "%d", n);
		if (n < i-1)
			fprintf(stdout, "-%d", i - 1);
		fprintf(stdout, "\n");
		}
}

/* btype - return string describing block pointed to by p */
char *btype(p)
struct block *p;
{
	static char *bname[] = {"000000000", "proc", "code", "string", "array",
		"data", "indir", "table", "telem", "points", "frame", "free"};

	if (p->b_type > 0 && p->b_type <= sizeof(bname)/sizeof(char *))
		return (bname[p->b_type]);
	sprintf(bname[0], "%d", p->b_type);
	return (bname[0]);
}
