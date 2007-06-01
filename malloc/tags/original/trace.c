/* #included in malloc.c */
#ifndef MEMMON
#define MEMMON "/u/cs217/7/memmon"
#endif

extern int close(int);
extern int dup2(int, int);
extern int execl(char *, ...);
extern int fork(void);
extern int pipe(int[]);
extern int write(int, char *, int);
extern struct frame { void *lreg[8], *ireg[8]; } *_flush(void);

/* trace - insert call trace from sp into pc[0..ncalls-1]; SPARC version */
static void trace(struct frame *sp, void *pc[], int ncalls) {
	int i;

	for (i = 0; sp && i < ncalls; i++) {
		pc[i] = sp->ireg[7];	/* return address */
		sp = sp->ireg[6];
	}
	if (sp == 0 && i > 1)
		i -= 2;
	else if (sp && sp->ireg[6] == 0 && i > 0)
		i -= 1;
	while (i < ncalls)
		pc[i++] = NULL;
}

/* send - send *msg to memmon, starting it if necessary; sp is caller's sp */
static void send(Memmon_T *msg, struct frame *sp) {
	static int inited = 0, fd[2];

	if (inited == 0) {
		inited = -1;
		if (pipe(fd) < 0)
			return;
		switch (fork()) {
		case -1:
			return;
		default:	/* parent: close fd[0], and write *msg */
			close(fd[0]);
			inited = 1;
			break;
		case 0:		/* child: rearrange i/o, and run memmon */
			close(fd[1]);
			if (dup2(fd[0], 0) >= 0) {
				close(fd[0]);
				execl(MEMMON, "memmon", NULL);
			}
			exit(EXIT_FAILURE);
		}
	}
	trace(sp, msg->calls, sizeof msg->calls/sizeof msg->calls[0]);
	if (inited > 0
	&& write(fd[1], (char *)msg, sizeof *msg) != (int)sizeof *msg)
		inited = -1;
}
