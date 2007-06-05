/* #included in malloc.c */
#ifndef MEMMON
#define MEMMON "./memmon"
#endif

extern int close(int);
extern int dup2(int, int);
extern int execl(const char *, const char *, ...);
extern int fork(void);
extern int pipe(int[]);
extern int write(int, char *, int);
#if sparc
extern struct frame { void *lreg[8], *ireg[8]; } *_framepointer(void);
asm(
".seg \"text\"\n"
".global _framepointer\n"
"_framepointer: ta 3\n"
"retl; mov %sp,%o0\n");
#elif i386
extern struct frame { void *savedbp, *retaddr; } *_framepointer(void);
asm(
".text\n"
".globl __framepointer, _framepointer\n"
"__framepointer:\n"
"_framepointer: mov %ebp,%eax\n"
"ret\n");
#else
Unsupported platform
#endif

/* trace - insert call trace from sp into pc[0..ncalls-1] */
static void trace(struct frame *fp, void *pc[], int ncalls) {
	int i;

#if sparc
	for (i = 0; fp && i < ncalls; i++) {
		pc[i] = fp->ireg[7];	/* return address */
		fp = fp->ireg[6];
	}
	if (fp == 0 && i > 1)
		i -= 2;
	else if (fp && fp->ireg[6] == 0 && i > 0)
		i -= 1;
#elif i386
  	for (i = 0; fp && i < ncalls; i++) {
		pc[i] = fp->retaddr;
    		fp = fp->savedbp;
	}
#if __APPLE__
	if (fp == 0 && i > 2)
		i -= 3;
	else
#endif
	if (fp == 0 && i > 1)
		i -= 2;
	else if (fp == 0 && i > 0)
		i -= 1;
#endif
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
