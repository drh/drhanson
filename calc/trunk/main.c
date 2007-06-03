/* calc: main program */

#include <stdio.h>
#include <ctype.h>
#include "calc.h"
#include <setjmp.h>
#include <signal.h>

jmp_buf restart;	/* to restart after runtime errors */
int debug;		/* debugging bits */
char *progname = "";
extern struct value Trace, Tracecode, TraceCompute;
static void onintr();

/* main program - parse expressions */
main(argc, argv)
int argc;
char *argv[];
{
        void (*isig)();
	int nf;

	progname = *argv;
	isig = signal(SIGINT, SIG_IGN);
	debug = 0;
	setjmp(restart);
	if (isig != SIG_IGN)
		signal(SIGINT, onintr);
	init();
	for (nf = 0; --argc > 0; )
		if (strcmp(*++argv, "-d") == 0)
			debug++;
		else if (strcmp(*argv, "-trace") == 0)
			trace = -1.0;
		else if (strcmp(*argv, "-tracecode") == 0)
			Tracecode.real = -1.0;
		else if (strcmp(*argv, "-tracecompute") == 0)
			TraceCompute.real = -1.0;
		else if (strcmp(*argv, "-traceexecute") == 0)
			TraceCompute.real = -1.0;
		else
			run(*argv), nf++;
	if (nf == 0)
		run("-");
	return (0);
}

/* init - initialize */
init()
{
	extern struct symbol *Show, *Table;

	initsym();
	initbuiltin();		/* built-in functions */
	Show = lookup(strsave("show"), GLOBAL);
	Table = lookup(strsave("table"), GLOBAL);
}

/* onintr - called on Interrupt signal */
static void onintr()
{
	fprintf(stderr, "\nInterrupt\n");
	longjmp(restart, 1);
}

/* run - execute file s */
run(s)
char *s;
{
	infile = s;
	lineno = 1;
	nerrors = 0;
	if (strcmp(s, "-") == 0) {
		infp = stdin;
		infile = NULL;
		}
	else if ((infp = fopen(s, "r")) == NULL) {
		fprintf(stderr, "%s: can't open %s\n", progname, s);
		return;
		}
	setjmp(restart);
	pc = code;
	while (yyparse()) {
		if (Tracecode.real) {
			dumpcode(code, stderr);
			Tracecode.real--;
			}
		execute(pc = code);
		}
	if (infp != stdin)
		fclose(infp);
}
