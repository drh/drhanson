/* s definitions */

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

/* sizes */
#define MAXFILES 10             /* maximum number of included files */
#define TOKSIZE 120             /* size of token string */
#define NAMSIZE 60              /* maximum length of a name */
#define SYMSIZE 37              /* size of symbol hash table */
#define CHUNKSIZE 32            /* granularity of the heap (words) */
#define MAXLOC 10               /* maximum number of locals */
#define NBLOCK 10               /* number of blocks allocated at once */
#define MAXVAL 200              /* maximum depth of value stack */
#define MAXIDX 15315            /* maximum index allowed for arrays */
#define DBSIZE 15               /* array data block size - no. of values */
#define TBSIZE 7                /* table block size - no. of values */
#define ISIZE 465               /* no. of values in indirect block */
#define MAXNVS 2047             /* maximum number of values to be stored */
#define LEVMAX 15               /* maximum procedure nesting */

/* types */
#define T_VOID (0<<13)           /* void */
#define T_INT (1<<13)            /* integer */
#define T_REAL (2<<13)           /* real */
#define T_VAR (3<<13)            /* variable */
#define T_FILE (4<<13)           /* file */
#define T_STRING (5<<13)         /* string */
#define T_PROC (6<<13)           /* procedure */
#define T_TABLE (7<<13)          /* table */

/* masks */
#define CHUNKMASK 077       /* mask for heap addresses */
#define XMASK 017777        /* mask for extracting T and X fields */
#define MRKMASK 077777      /* mask for extracting mark bit */
#define BTMASK 074000       /* mask for extracting block type */
#define NVMASK 003777       /* mask for extracting length of hash chain */

/* macros */
#define TYPE(x) (x.v_type&~XMASK)          /* extract type */
#define SREAL(r) (((r>>3)&XMASK)|T_REAL)   /* extract low order bits of real */
#define XFIELD(v) (v.v_type&XMASK)         /* extract x field */
#define BLKTOP(s) (s&~CHUNKMASK)           /* get to top of blk while in blk */
#define STRTOP(s) (s-CHUNKSIZE*2)          /* get to top of blk from next top */
#define MARK(p) (p->b_type&~MRKMASK)       /* extract mark bit */
#define UNMARK(p) (p->b_type&MRKMASK)      /* unmark blocks */
#define BTYPE(p) (p->b_type&BTMASK)        /* extract block type */
#define NVALS(p) (p->b_type&NVMASK)        /* extract no. of values stored */

/* table or array flags */
#define RVAL 0                      /* read value flag */
#define LVAL 1                      /* assign to value flag */
#define SINDIR 435                  /* starting index for single indirection */
#define DINDIR 900                  /* starting index for double indirection */

/* block types */
#define B_PROC (1<<11)
#define B_CODE (2<<11)
#define B_STRING (3<<11)
#define B_ARRAY (4<<11)
#define B_DATA (5<<11)
#define B_INDIR (6<<11)
#define B_TABLE (7<<11)
#define B_TELEM (8<<11)
#define B_FREE (9<<11)
#define B_MARK (1<<15)

/* scopes */
#define S_GLOBAL 0
#define S_LOCAL 1
#define S_PARAM 2

/* starting offsets */
#define LOFF 10
#define AOFF 0

/* code function addresses */
#define O_ADD 0
#define O_SUB 1
#define O_MUL 2
#define O_DIV 3
#define O_MOD 4
#define O_CAT 5
#define O_SSTR 6
#define O_IDX 7
#define O_CALL 8
#define O_GVAR 9
#define O_GVAL 10
#define O_LVAR 11
#define O_AVAR 12
#define O_JUMP 13
#define O_LINK 14
#define O_LT 15
#define O_LE 16
#define O_EQ 17
#define O_NE 18
#define O_GE 19
#define O_GT 20
#define O_JEV 21
#define O_ASGN 22
#define O_RET 23
#define O_NEG 24
#define O_BINIT 25
#define O_BNEXT 26
#define O_SSASN 27
#define O_IDXL 28
#define O_BASE 29

/* data structures */
struct bheap {
   struct bheap *h_list;                    /* pointer next block heap */
   int h_size;                              /* size of heap */
   char h_slot[NBLOCK*CHUNKSIZE*2];         /* rest of heap */
   };

struct value {
   int v_type;                  /* T and X fields */
   int v_addr;                  /* A field */
   };

struct symbol {
   int (*y_fcn)();              /* pointer to o_gvar */
   struct value y_val;          /* value for global variables */
   int y_loff;                  /* offset for local variables */
   int y_scope;                 /* scope during compilation */
   char *y_name;                /* variable name */
   struct symbol *slink;        /* link together symbol entries */
   };

struct block {                  /* common block format */
   int b_type;                  /* block type */
   int b_ints[CHUNKSIZE-1];     /* the rest */
   };

struct code {
   int c_type;                  /* block type */
   struct code *c_next;         /* pointer to next code block */
   int *c_code[CHUNKSIZE-2];    /* code pointers */
   };

struct string {
   int s_type;                  /* block type */
   struct string *str_next;     /* pointer to next string block */
   char s_chars[2*CHUNKSIZE-4]; /* the characters */
   };

struct proc {
   int p_type;                  /* block type */
   struct code *p_next;         /* pointer to next code block */
   char *p_name;                /* pointer to proc name */
   int p_nargs;                 /* number of arguments */
   int p_nlocals;               /* number of locals */
   int *p_code[CHUNKSIZE-5];    /* code pointers */
   };

/* code functions */
extern int o_add();             /* addition */
extern int o_sub();             /* subtraction */
extern int o_mul();             /* multiplication */
extern int o_div();             /* division */
extern int o_mod();             /* modulus */
extern int o_cat();             /* concatenation */
extern int o_sstr();            /* substring */
extern int o_idx();             /* indexing */
extern int o_call();            /* procedure invocation */
extern int o_gvar();            /* load global variable */
extern int o_gval();            /* load global constant */
extern int o_lvar();            /* load local variable */
extern int o_avar();            /* load arguments */
extern int o_jump();            /* unconditional jump */
extern int o_link();            /* jump to new code block */
extern int o_lt();              /* less than */
extern int o_le();              /* less than or equal */
extern int o_eq();              /* equal */
extern int o_ne();              /* not equal */
extern int o_ge();              /* greater than or equal */
extern int o_gt();              /* greater than */
extern int o_jev();             /* jump if void */
extern int o_asgn();            /* assignment */
extern int o_ret();             /* return from procedure */
extern int o_neg();             /* negation */
extern int o_binit();           /* enumerating tables */
extern int o_bnext();           /* enumerating tables */
extern int o_ssasn();           /* substring assignment */
extern int o_idxl();            /* indexed assignment */
extern int o_base();            /* clear stack to base */

/* built-in functions */
extern int x_close();
extern int x_compile();
extern int x_display();
extern int x_dump();
extern int x_find();
extern int x_image();
extern int x_integer();
extern int x_many();
extern int x_map();
extern int x_match();
extern int x_numeric();
extern int x_open();
extern int x_read();
extern int x_real();
extern int x_reverse();
extern int x_seek();
extern int x_size();
extern int x_string();
extern int x_trace();
extern int x_type();
extern int x_upto();
extern int x_write();

struct tlvars {
   int (*f)();
   int off;
   } lvars [];

int (*(op[]))();
char *opname[];
char *bfname[];

struct bplist {
   struct lelem *true;
   struct lelem *false;
   };

struct stlist {
   struct lelem *s_next;
   struct lelem *s_break;
   struct lelem *s_succ;
   };

struct lelem {
   struct lelem *link;
   int **bptr;
   };

struct sentry {
   struct sentry *snext;
   char *schar;
   };
