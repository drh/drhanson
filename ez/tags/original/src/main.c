/* ez: main program */

#include <stdio.h>
#include <ctype.h>
#include "ez.h"
#include "tokens.h"
#include <setjmp.h>
#include <signal.h>

int execute;		/* set to execute input */
jmp_buf restart;	/* to restart after runtime errors */
int debug;		/* debugging bits */
char *progname = "";
char *fsysname = FILESYS;/* ez file system name */
extern int cachelimit;	/* cache size limit */
extern int trace;	/* trace counter */
extern int slength;	/* source code length */
extern char *source;	/* source code */
extern struct root *rp;	/* pointer to root block */

/* main program - parse and execute statements */
main(argc, argv)
int argc;
char *argv[];
{
	FILE *f;
	int i, (*isig)(), onintr();
	extern struct value errors;
	struct value v, getline();

	progname = *argv;
	isig = signal(SIGINT, SIG_IGN);
	debug = trace = 0;
	cachelimit = 100;
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
		else if (strncmp(argv[i], "-t", 2) == 0)
			if (argv[i][2])
				trace = atoi(&argv[i][2]);
			else
				trace = -1;
		else
			fsysname = argv[i];
	initialize(fsysname);
	for (;;) {
		setjmp(restart);
		if (isig != SIG_IGN)
			signal(SIGINT, onintr);
		v = getline(stdin);	  
		if (TYPE(v) == T_VOID)
			break;
		if (debug&DBINPUT) {
			image(v, stderr, 1);
			putc('\n', stderr);
			}
		v = excvproc(v);
		if (nerrors)
			excvstr(errors, stdout);
		else if (execute) {
			v = apply(v);
			excvstr(v, stdout);
			if (TYPE(v) != T_VOID)
				putchar('\n');
			}
		}
	putblk(rp, 1);
	savecache();
}

/* getline - get next input source item */
struct value getline(f)
FILE *f;
{
	struct value v;
	char *s;
	int t, va, n, c1, c2;

	execute = 0;
	initlex(f, NULL, stderr);
	va = salloc();
	source = SOPENW(va);   
	n = 0;
	switch (c1 = t = gtok('\n')) {
		case EOF:
			c2 = EOF;
			break;
		case '{':
			execute++;
			c2 = '}';
			break;
		case PROCEDURE:
			execute++;
			c2 = END;
			break;
		default:
			execute++;
			c1 = 0;
			c2 = '\n';
			break;
		}
	do {
		if (t == c2 && --n < 0)
			break;
		t = gtok(0);
		if (t == c1)
			n++;
		} while (t > 0);
	SCLOSEW(source);
	source = NULL;
	if (c2 == -1)
		return (VOID);
	return (mkstr(va, slength));
}

/* initialize - initialize ez file system in fname */
initialize(fname)
char *fname;
{
	struct value *vp;
	struct table *bp;
	int fd, i;
	extern int filesize, cnstp, strp, level;

	if (access(fname, 2)) {
		if ((fd = creat(fname, 0666)) == -1) {
			fprintf(stderr, "%s: can't create\n", fname);
			exit(1);
			}
		close(fd);
		}
	initcache(fname);
	rp = (struct root *) getblk(0);
	if (filesize > sizeof(struct block) && rp->r_major != MAJOR) {
		fprintf(stderr, "%s: version mismatch; %s version is %d.%d, \
system version is %d.%d\n", progname, fname, rp->r_major, rp->r_minor,
			MAJOR, MINOR);
		exit(1);
		}
	touch(rp);
	*builtin("root") = rp->r_globals;
	cnstp = strp = level = 0;
	Procedure = stralloc("Procedure", -1);
	Resumption = stralloc("Resumption", -1);
	if (rp->r_major == MAJOR)
		return;
	rp->r_type = B_DATA;
	rp->r_major = MAJOR;
	rp->r_minor = MINOR;
	rp->r_freelist = 0;
	bp = (struct table *) balloc(B_TABLE);
	rp->r_globals = mkval(T_TABLE, 0, virtaddr(bp));
	rp->r_wglobals = rp->r_globals;
	putblk(bp, 0);
	rp->r_dotdot = stralloc("..", -1);
	VOID = mkval(T_VOID, 0, 0);
	*builtin("root") = rp->r_globals;
	for (i = 0; bivalues[i].bi_name; i++) {
		vp = tindex(rp->r_globals,stralloc(bivalues[i].bi_name,-1),1);
		*vp = *builtin(bivalues[i].bi_name);
		putblk(vp, 1);
		}
}

/* onintr - called on Interrupt signal */
onintr()
{
	fprintf(stderr, "\nInterrupt\n");
	longjmp(restart, 1);
}

