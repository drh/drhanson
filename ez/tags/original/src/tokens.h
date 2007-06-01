/* ez tokens (other than single characters) */

#define EQ	256		/* == */
#define NE	257		/* != */
#define LE	258		/* <= */
#define GE	259		/* >= */
#define CAT	260		/* || */
#define CON	261		/* integer constant */
#define FCON	262		/* floating constant */
#define SCON	263		/* string constant */
#define	BCON	264		/* built-in value constant */
#define ID	265		/* identifier */
#define IF	266		/* if */
#define ELSE	267		/* else */
#define WHILE	268		/* while */
#define FOR	269		/* for */
#define RETURN	270		/* return */
#define BREAK	271		/* break */
#define CONTINUE 272		/* continue */
#define	PROCEDURE 273		/* procedure */
#define	IN	274		/* in */
#define	LOCAL	275		/* local */
#define	END	276		/* end */

#define	emit1(x) emit(1,x)
#define UNARY	  01000|	/* unary operator indicator */
#define SUFFIX	  02000|	/* suffix operator indicator */
#define BINARY	  04000|	/* binary operator indicator */

struct node {			/* expression nodes */
	int ex_op;		/* operator */
	int ex_type;		/* type of result */
	struct node *ex_l;	/* operands */
	struct node *ex_r;
	int ex_result;		/* result flag */
	int ex_count;		/* reference count */
};

struct bpnode {			/* backpatch list node */
	int addr;		/* virtual address to patch */
	struct bpnode *next;	/* next node */
};

struct label {			/* label definitions */
	int addr;		/* virtual address of label */
	struct bpnode *bplist;	/* backpatch list until defined */
};

struct compile {		/* compilation variables */
	struct proc *proc;	/* current procedure */
	int offset;		/* offset into input of beginning of proc */
	int loop;		/* current loop handle */
	int forlevel;		/* nesting depth for for (x in e) loops */
	int label;		/* next generated label */
	struct code *cblock;	/* current code block */
	int *pc;		/* pointer to next word in cblock->c_code */
	struct label labels[200];/* labels */
	int point;		/* next resumption point */
	struct point points[100];/* resumption points */
};

struct lexval { 		/* values set by gtok */
	char *l_str;		/* string and name token */
	int l_length;		/* length of string constant */
	int l_ival;		/* integer and character constant */
};

/* global variables */
extern int t;			/* current token */
extern struct lexval lexval;	/* associated value */
extern int nerrors;		/* error count */
extern struct compile c;	/* current compilation record */
