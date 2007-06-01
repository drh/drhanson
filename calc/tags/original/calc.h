/* calc: definitions */

/* limits */
#define	TOKSIZE	120		/* size of token string */
#define HASHSIZE 119		/* hash table size */

/* types */
#define	T_VOID	0		/* void */
#define	T_REAL	1		/* real */
#define	T_VAR	2		/* variable */
#define	T_STRING 3		/* string */
#define	T_TABLE	4		/* table */

/* data structures */
struct value {
	char type;			/* datatype */
	union u_info {			/* associated information */
		double u_double;	/* real value */
		char *u_str;		/* string */
		struct value *u_var;	/* variable */
		struct table *u_tbl;	/* table */
		unsigned u_other;	/* everything else */
	} u_info;
};

#define real u_info.u_double		/* abbreviations */
#define string u_info.u_str
#define var u_info.u_var
#define tbl u_info.u_tbl
#define other u_info.u_other

struct table {			/* tables (associative arrays) */
	int size;		/* number of elements in the table */
	struct tnode *htab[37];	/* hash headers */
};
struct tnode {			/* table entry */
	struct value index;	/* index */
	struct value value;	/* associated value */
	struct tnode *link;	/* next entry on chain */
};

struct symbol { 		/* symbol table entries */
	char *name;		/* entry name */
	struct value val;	/* current value */
	struct dnode *depen;	/* dependents */
	struct dnode *succ;	/* successors */
	int count;		/* predecessor count */
	union inst *code;	/* pointer to code */
	struct value (*f)();	/* builtin function */
	union inst *fcode;	/* pointer to function code */
	short amin;		/* minimum number of arguments */
	short amax;		/* maximum number of arguments */
	short offset;		/* activation frame offset */
	short flags;		/* flags (see below) */
	struct symbol *class;	/* link to next entry in same class */
	struct symbol *link;	/* link to next table entry */
};

struct dnode {			/* dependency nodes */
	struct symbol *sym;	/* depends on */
	struct dnode *link;	/* next dnode */
};

union inst {	 		/* "machine" instruction */
	int (*op)();		/* operator function */
	union inst *addr;	/* link in backpatch list */
	int offset;		/* jump offset */
	struct value *val;	/* pointer to a constant */
	struct symbol *sym;	/* pointer to symbol entry */
};

struct operator {		/* operator information */
	int argc;		/* expected number of arguments */
	int (*op)();		/* function */
	char *name;		/* print name */
};

/* symbol table entry flags */
#define CONST	   01		/* symbol is a constant */
#define FUNC	   02		/* symbol is a function */
#define BUILTIN	   04 		/* symbol is a builtin */
#define DEFERRED  010		/* computation of symbol is deferred */
#define GLOBAL	  020		/* symbol is a global */
#define LOCAL	  040		/* symbol is a parameter or local */
#define	ARB	121		/* in amax, denotes arbitrary number of args */

/* globals */
struct value cvr();
struct value cvs();
struct value deref();
struct value newvalue();
double rnd();
struct tnode *tindex();
#undef atof
double atof();
char *alloc();
struct symbol *lookup();
struct symbol *install();
struct symbol *new();
char *strsave();
struct dnode *add();
struct dnode *append();
struct dnode *dlist();
struct dnode *tsort();
union inst *dumpinst();
union inst *savecode();

extern int nerrors;		/* error count */
extern FILE *infp;		/* input file */
extern char *infile;		/* input file name */
extern int lineno;		/* input line number */
extern union inst *pc;		/* location counter */
extern union inst code[];	/* program code */
extern char *progname;		/* program name */

/* built-in variables */
#define trace Trace.real	/* trace flag */
extern struct value Trace;
