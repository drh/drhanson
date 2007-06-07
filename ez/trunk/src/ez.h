/* ez: definitions */

/* implementation parameters */
#define	BPW	4		/* bytes per integer */
#define	CHUNKSIZE 32		/* granularity of the heap (integers) */
#define	MAJOR	4 		/* system major version number */
#define	MINOR	0 		/* system minor version number */
#define	VERSION "4.0"		/* value of version */
#define	FILESYS	"ez.sys"	/* default file system file name */

/* limits */
#define	TOKSIZE	120		/* size of token string */
#define	CHASHSIZE 137		/* initial size of the cache hash table */

/* types */
#define	T_VOID	0		/* void */
#define	T_INT	1		/* integer */
#define	T_REAL	2		/* real */
#define	T_VAR	3		/* variable */
#define	T_STRING 4		/* string */
#define	T_PROC	5		/* procedure */
#define	T_TABLE	6		/* table */

/* variable flags */
#define	PHYSICAL  0		/* variable points to a physical address */
#define VIRTUAL	  1		/* variable points to a virtual address */
#define SUBSTRING 2		/* variable denotes substring assignment */
#define SUBPROC	  3		/* variable denotes procedure modification */

/* masks */
#define	CHUNKMASK ((unsigned)(BPW*CHUNKSIZE-1)) 	/* block addresses */

/* debugging bits */
#define	DBCACHE	0001		/* traces cache operations */
#define	DBCODE	0002		/* prints compiled code after compilation */
#define	DBEMIT	0004		/* trace operator emission */
#define	DBINPUT	0010		/* print user input */

/* macros */
#define	TYPE(x)	((x).v_type)			/* extract type */
#define	XFIELD(v) ((v).v_x)			/* extract x field */
#define	BLKTOP(s) (((int)s)&~CHUNKMASK)		/* get to top of block */
#define	SOPENR(v) ((char *)getblk(v))		/* open string for reading */
#define	SOPENW(v) ((char *)getblk(v))		/* open string for writing */
#define	SGETC(p) (((int)p&CHUNKMASK)?*p++:snextr(&p)) /* get next char */
#define	SPUTC(p,c) if ((int)p&CHUNKMASK) *p++ = c; else snextw(&p, c);
	  					/* put char into string */
#define	SCLOSER(p) putblk(p-1,0)		/* release string after scan */
#define	SCLOSEW(p) putblk(p-1,1)		/* release string after scan */

/* block types */
#define B_FREE	0
#define	B_PROC	1
#define	B_CODE	2
#define	B_STRING 3
#define	B_ARRAY	4
#define	B_DATA	5
#define	B_INDIR	6
#define	B_TABLE	7
#define	B_TELEM	8
#define B_POINTS 9
#define B_FRAME	10
#define	B_BLOCK	11

/* operators */
#define	O_BAD	 0
#define	O_ADD	 1
#define	O_SUB	 2
#define	O_MUL	 3
#define	O_DIV	 4
#define	O_MOD	 5
#define	O_CAT	 6
#define	O_SSTR	 7
#define	O_IDX	 8
#define	O_CALL	 9
#define	O_GVAR	10
#define	O_GVAL	11
#define	O_LVAR	12
#define	O_AVAR	13
#define	O_JUMP	14
#define	O_LINK	15
#define	O_LT	16
#define	O_LE	17
#define	O_EQ	18
#define	O_NE	19
#define	O_GE	20
#define	O_GT	21
#define	O_JEV	22
#define	O_ASGN	23
#define	O_RET	24
#define	O_NEG	25
#define	O_ITER	26
#define	O_SSTRL	27
#define	O_IDXL	28
#define	O_BASE	29
#define	O_DEREF	30
#define	O_CVI	31
#define	O_CVR	32
#define	O_CVNUM	33
#define	O_CVSTR	34
#define	O_DUP	35
#define	O_NOOP	36
#define	O_LVAL	37
#define	O_AVAL	38
#define	O_QUIT	39
#define	O_NOT	40
#define	O_JNV	41
#define	O_BEV	42
#define	O_BNV	43
#define	O_POP	44
#define	O_ERR	45

/* data structures */

struct value {
	char v_type;			/* T field */
	short v_x;			/* X field */
	int v_addr;			/* A field */
};

struct bivalue {			/* built-in value */
	char *bi_name;			/* name */
	char bi_type;			/* type */
	short bi_x;			/* x field */
	char *bi_str;			/* string for T_STRING,T_INT,T_REAL */
	struct value (*bi_addr)();	/* pointer to C function for T_PROC */
	struct value bi_v;		/* value */
};

struct block {			/* common block format */
	int b_type;		/* block type */
	int b_ints[CHUNKSIZE-1];/* the rest */
};

struct code {
	int c_type;		/* block type */
	int c_next;	 	/* pointer to next code block */
	int c_code[CHUNKSIZE-2];/* code */
};

struct string {
	int s_type;		/* block type */
	int s_next;		/* pointer to next string block */
	char s_chars[BPW*(CHUNKSIZE-2)]; /* the characters */
};

struct proc {
	int p_type;		/* block type */
	struct value p_name;	/* procedure name */
	struct value p_sym;	/* table of locals */
	struct value p_source;	/* source code */
	int p_nargs; 		/* number of arguments */
	int p_nlocals;		/* number of locals */
	int p_code;		/* virtual address of code */
	int p_points[16];	/* pointers to resumption point blocks */
};

struct table {
	int t_type;		/* block type */
	int t_size;		/* number of elements in the table */
	int t_elem[CHUNKSIZE-2];/* pointers to element blocks */
};

struct elem {
	int e_type;		/* block type */
	int e_next;		/* pointer to next elem block on chain */
	struct value e_vals[(CHUNKSIZE-2)/(sizeof(struct value)/sizeof(int))];
				/* index, value pairs */
};

struct array {
	int a_type;		/* block type */
	int a_size;		/* number of elements in the table */
	int a_indir[CHUNKSIZE-2];/* pointers to indirect blocks */
};

struct indir {
	int i_type;		/* block type */
	int i_data[CHUNKSIZE-1];/* pointer to data blocks */
};

struct data {
	int d_type;		/* block type */
	struct value d_vals[(CHUNKSIZE-1)/(sizeof(struct value)/sizeof(int))];
				/* values */
};

struct point {			/* resumption point definition */
	short pos[2];		/* beginning, ending positions in source */
	int pc;			/* virtual address of code */
};

struct points {
	int n_type;		/* block type */
	struct point n_points[(CHUNKSIZE-1)/(sizeof(struct point)/sizeof(int))];
				/* resumption points */
};

struct frame {
	int f_type;		/* block type */
	short f_sp;		/* stack top */
	short f_lp;		/* offset to locals */
	int f_pc;		/* location counter when inactive */
	struct frame *f_prev;	/* previous (caller) frame */
	struct value f_table;	/* associated symbol table */
#define f_spbase f_table.v_x	/* initial value of f_sp */
	struct value f_vals[(CHUNKSIZE-6)/(sizeof(struct value)/sizeof(int))];
				/* arguments, locals, expression stack */
};

struct root {
	int r_type;		/* block type -- not used */
	short r_major, r_minor;	/* version numbers */
	struct value r_globals;	/* initial global symbol table */
	struct value r_wglobals;/* working global symbol table */
	struct value r_dotdot;	/* ancestor entry ("..") */
	int r_freelist;		/* head of free block list */
};

/* globals */
extern struct value VOID;	/* canonical void */
extern int debug;		/* debugging flag */
struct block *balloc();		/* cache.c */
struct block *getblk();
struct value apply();		/* code.c */
struct frame *falloc();
struct value xpop();
struct value cvi();		/* cvt.c */
struct value cvnum();
struct value cvr();
struct value cvstr();
struct value deref();
struct value excvi();
struct value excvnum();
struct value excvr();
struct value excvstr();
struct value excvproc();
float getr();
struct value image();
struct value mkint();
struct value mkreal();
struct value mkstr();
struct value mkval();
struct value *builtin();	/* func.c */
extern struct bivalue bivalues[];
char *alloc();			/* gen.c */
struct proc *procbeg();
struct value ezparse();		/* parse.c */
struct value Procedure;		/* proc.c */
struct value Resumption;
struct value create();
struct value pidx();
struct point *pindex();
struct value psubstr();
struct value tasgn();		/* table.c */
struct value tcat();
struct value *tindex();
struct value tsub();
char *getstr();			/* util.c */
char *sindex();
struct value stralloc();
struct value substr();
