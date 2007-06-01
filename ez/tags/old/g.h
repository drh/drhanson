/* globals */

struct value val[MAXVAL];               /* value stack for interp */
int xsp;                                /* value stack pointer */
int xap;                                /* store stack argument pointer */
int xlp;                                /* store stack local pointer */
struct value void;                      /* void value */
struct code *curblk;                    /* pointer to current block */
struct code *fstblk;                    /* pointer to first code block */
char *ident1;                           /* variable name in current for in */
int **cp;                               /* code pointer */
int loff;                               /* current local offset index */
int aoff;                               /* current argument offset index */
int level;                              /* procedure nesting level */
int slevel;                             /* statement nesting level */
int trace;                              /* trace flag */
int ntrace;                             /* execution trace counter */
int pnlevel;                            /* execution time proc nesting level */
int debug;                              /* debug flag */
int gcoff;                              /* garbage collector flag */
jmp_buf env;                            /* stack state for errors */
int eofflag;                            /* input eof flag */
int doflag;                             /* DO flag, turns ) into DO */
int ctype;                              /* type of last constant */
int lensav;                             /* length of last constant */
struct symbol *symtab[SYMSIZE];         /* symbol table hash headers */
struct symbol *constab[SYMSIZE];        /* constant table hash headers */
char *istr;                             /* tmporary string storage */
int sp;                                 /* stack pointer */
int **cpstor[LEVMAX];                   /* previous cp */
struct code *blkstor[LEVMAX];           /* previous curblk */
