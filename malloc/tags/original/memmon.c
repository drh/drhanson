#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sym.h"
#include "memmon.h"
#define T Memmon_T

#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))
#define NCALLS NELEMS(((T*)0)->calls)

static char rcsid[] = "$Id: memmon.c,v 1.16 1994/12/06 14:35:30 drh Exp $";
/*lint -esym(528,rcsid) */

extern int read(int, char *, int);
extern int unlink(char *);

struct block {			/* block descriptor: */
	struct block *link;		/* next block on the hash chain */
	struct block *next;		/* next block in creation order */
	enum { FREE, BUSY } state;	/* allocation state */
	void *ptr;			/* block address */
	size_t nobj;			/* number of objects */
	size_t size;			/* object size */
	void *calls[NCALLS];		/* creator's call stack */
} *htab[1024];			/* hash table, indexed by block address */
struct block *head;		/* list of all blocks, in creation order */
#define hash(a) ((unsigned)(a)>>3)

int inuse_at_exit = 1;
int show_all_calls = 0;
char *a_out = "a.out";
char *log_file = NULL;
char *temp_file = NULL;
FILE *errfp = stderr;
Sym_T stab = NULL;

/* print - print the call trace in pc[0..NCALLS-1] */
static void print(void *pc[]) {
	int i;

	for (i = 0; i < NCALLS && pc[i]; i++) {
		char *s = stab ? Sym_find(stab, pc[i]) : NULL;
		fprintf(errfp, "\t%-16.16s [pc=0x%p]\n", s ? s : "?", pc[i]);
	}
}

/* block - allocate and initialize a block descriptor for a BUSY block */
static void block(void *ptr, unsigned nobj, unsigned size, void *calls[NCALLS]) {
	struct block *bp = malloc(sizeof *bp);
	unsigned h = hash(ptr)%NELEMS(htab);
	static struct block **tail = &head;

	assert(bp);
	bp->state = BUSY;
	bp->ptr = ptr;
	bp->nobj = nobj;
	bp->size = size;
	memcpy(bp->calls, calls, sizeof bp->calls);
	bp->link = htab[h];
	htab[h] = bp;
	bp->next = NULL;
	*tail = bp;
	tail = &bp->next;
}

/* find - find the block descriptor for the block at address ptr */
static struct block *find(void *ptr) {
	struct block *bp = htab[hash(ptr)%NELEMS(htab)];

	while (bp && bp->ptr != ptr)
		bp = bp->link;
	return bp;
}

/* _free - free block at address ptr */
static void _free(void *ptr, void *calls[]) {
	struct block *bp;

	if (show_all_calls) {
		fprintf(errfp, "\n== free(0x%p) called from:\n", ptr);
		print(calls);
	}
	if (ptr == NULL)
		return;
	bp = find(ptr);
	if (bp == NULL) {
		fprintf(errfp, "\n** free'ing unallocated memory\n");
		fprintf(errfp, "   free(0x%p) called from:\n", ptr);
		print(calls);
	} else if (bp->state == FREE) {
		fprintf(errfp, "\n** free'ing free memory\n");
		fprintf(errfp, "   free(0x%p) called from:\n", ptr);
		print(calls);
		fprintf(errfp, "   This block is %u bytes long and was malloc'd from:\n", bp->nobj*bp->size);
		print(bp->calls);
	} else
		bp->state = FREE;
}

/* _malloc - allocate nbytes of memory */
static void _malloc(void *ptr, size_t size, void *calls[]) {
	struct block *bp;

	if (show_all_calls) {
		fprintf(errfp, "\n== malloc(%u) called from:\n", size);
		print(calls);
		fprintf(errfp, "   malloc returned 0x%p\n", ptr);
	}
	if (size == 0) {
		fprintf(errfp, "\n** malloc'ing a zero-length block\n");
		fprintf(errfp, "** malloc(%u) called from:\n", size);
		print(calls);
		fprintf(errfp, "   malloc returned 0x%p\n", ptr);
	}
	if (ptr && (bp = find(ptr)) != NULL) {
		if (bp->state == BUSY) {
			fprintf(errfp, "\n** malloc'ing allocated memory\n");
			fprintf(errfp, "** malloc(%u) called from:\n", size);
			print(calls);
			fprintf(errfp, "   malloc returned 0x%p\n", ptr);
		}
		bp->nobj = 1;
		bp->size = size;
		bp->state = BUSY;
	} else if (ptr)
		block(ptr, 1, size, calls);
}

/* _calloc - allocate nobj*size of memory */
static void _calloc(void *ptr, size_t nobj, size_t size, void *calls[]) {
	struct block *bp = find(ptr);

	if (show_all_calls) {
		fprintf(errfp, "\n== calloc(%u,%u) called from:\n", nobj, size);
		print(calls);
		fprintf(errfp, "   calloc returned 0x%p\n", ptr);
	}
	if (nobj == 0 || size == 0) {
		fprintf(errfp, "\n** calloc'ing a zero-length block\n");
		fprintf(errfp, "   calloc(%u,%u) called from:\n", nobj, size);
		print(calls);
		fprintf(errfp, "   calloc returned 0x%p\n", ptr);
	}
	if (ptr && (bp = find(ptr)) != NULL) {
		if (bp->state == BUSY) {
			fprintf(errfp, "\n** calloc'ing allocated memory\n");
			fprintf(errfp, "** calloc(%u,%u) called from:\n", nobj, size);
			print(calls);
			fprintf(errfp, "   calloc returned 0x%p\n", ptr);
		}
		bp->nobj = nobj;
		bp->size = size;
		bp->state = BUSY;
	} else if (ptr)
		block(ptr, nobj, size, calls);
}

/* _realloc - reallocate the block at ptr[0] to be size bytes */
static void _realloc(void *ptr[], size_t size, void *calls[]) {
	struct block *bp = find(ptr[0]);

	if (show_all_calls) {
		fprintf(errfp, "\n== realloc(0x%p,%u) called from:\n", ptr[0], size);
		print(calls);
		fprintf(errfp, "   realloc returned 0x%p\n", ptr[1]);
	}
	if (ptr[0] == NULL && size == 0) {
		fprintf(errfp, "\n** realloc'ing a NULL pointer to zero bytes\n");
		fprintf(errfp, "   realloc(0x%p,%u) called from:\n", ptr[0], size);
		print(calls);
		fprintf(errfp, "   realloc returned 0x%p\n", ptr[1]);
	}
	if (ptr[0] && bp == NULL) {
		fprintf(errfp, "\n** realloc'ing unallocated memory\n");
		fprintf(errfp, "   realloc(0x%p,%u) called from:\n", ptr[0], size);
		print(calls);
		fprintf(errfp, "   realloc returned 0x%p\n", ptr[1]);
	} else if (ptr[0] && bp->state == FREE) {
		fprintf(errfp, "\n** realloc'ing free memory\n");
		fprintf(errfp, "   realloc(0x%p,%u) called from:\n", ptr[0], size);
		print(calls);
		fprintf(errfp, "   This block is %u bytes long and was malloc'd from:\n",
			bp->nobj*bp->size);
		print(bp->calls);
		fprintf(errfp, "   realloc returned 0x%p\n", ptr[1]);
	} else if (ptr[0] && bp)
		bp->state = FREE;
	if (ptr[1] && (bp = find(ptr[1])) != NULL && bp->state == BUSY) {
		fprintf(errfp, "\n** realloc'ing allocated memory\n");
		fprintf(errfp, "   realloc(0x%p,%u) called from:\n", ptr[0], size);
		print(calls);
		fprintf(errfp, "   realloc returned 0x%p\n", ptr[1]);
	} else if (ptr[1] && bp && bp->state == FREE) {
		bp->nobj = 1;
		bp->size = size;
		bp->state = BUSY;
	} else if (ptr[1])
		block(ptr[1], 1, size, calls);
}

/* doopt - process option in arg */
void doopt(char *arg) {
	if (strcmp(arg, "-inuse-at-exit") == 0
	||  strcmp(arg, "-inuse-at-exit=yes") == 0)
		inuse_at_exit = 1;
	else if (strcmp(arg, "-inuse-at-exit=no") == 0)
		inuse_at_exit = 0;
	else if (strcmp(arg, "-show-all-calls") == 0
	||       strcmp(arg, "-show-all-calls=yes") == 0)
		show_all_calls = 1;
	else if (strcmp(arg, "-show-all-calls=no") == 0)
		show_all_calls = 0;
	else if (strcmp(arg, "-a.out") == 0)
		a_out = "a.out";
	else if (strncmp(arg, "-a.out=", 7) == 0)
		a_out = arg + 7;
	else if (strncmp(arg, "-log-file=", 10) == 0)
		log_file = arg + 10;
	else if (strncmp(arg, "-temp-file=", 11) == 0)
		temp_file = arg + 11;
}

/* inuse - print memory in use */
void inuse(void) {
	struct block *bp;

	for (bp = head; bp; bp = bp->next)
		if (bp->state == BUSY) {
			fprintf(errfp, "\n** Memory in use at 0x%p\n", bp->ptr);
			fprintf(errfp, "   This block is %u bytes long and "
				"was malloc'd from:\n", bp->nobj*bp->size);
			print(bp->calls);

		}
}

/* unknown - print info about unrecognized n-byte msg */
void unknown(int n, T *msg) {
	int i;

	if (!show_all_calls)
		return;
	if (n != (int)sizeof *msg) {
		fprintf(errfp, "\n?? unknown %d-byte memmon message\n", n);
		if ((int)sizeof *msg - n > 0)
			memset(msg, 0, (int)sizeof *msg - n);
	} else
		fprintf(errfp, "\n?? unknown memmon message\n");
	fprintf(errfp, "   opcode=%d ptr[]={0x%p,0x%p} "
		"size[]={%u,%u}\n   calls[]={0x%p",
		msg->opcode, msg->ptr[0], msg->ptr[1],
		msg->size[0], msg->size[1], msg->calls[0]);
	for (i = 1; i < NCALLS; i++)
		fprintf(errfp, ",0x%p", msg->calls[i]);
	fprintf(errfp, "}\n");
}

int main(int argc, char *argv[]) {
	int i, n;
	char *opts, temp[L_tmpnam] = {0};
	T msg;

	for (i = 1; i < argc; i++)
		doopt(argv[i]);
	if ((opts = getenv("MEMMONOPTIONS")) != NULL) {
		char *arg = strtok(opts, " \t");
		for ( ; arg; arg = strtok(NULL, " \t"))
			doopt(arg);
	}
	if (log_file && (errfp = fopen(log_file, "w")) == NULL) {
		errfp = stderr;
		fprintf(errfp, "%s: can't create `%s'\n", argv[0], log_file);
	}
	if (a_out) {
		char cmd[256];
		if (temp_file == NULL)
			temp_file = tmpnam(temp);
		sprintf(cmd, "/bin/nm -n %s | /bin/grep '[tT] _' >%s",
			a_out, temp_file);
		(void)system(cmd);
		stab = Sym_init(temp_file);
	}
	fprintf(errfp, "%s $Revision: 1.16 $\nOptions:", argv[0]);
	fprintf(errfp, " a.out=%s", a_out);
	fprintf(errfp, " -inuse-at-exit=%s",  inuse_at_exit  ? "yes" : "no");
	fprintf(errfp, " -show-all-calls=%s", show_all_calls ? "yes" : "no");
	fprintf(errfp, " -log-file=%s", log_file ? log_file : "stderr");
	fprintf(errfp, " -temp-file=%s\n", temp_file);
	while ((n = read(0, (char *)&msg, sizeof msg)) > 0)
		if (n == (int)sizeof msg)
			switch (msg.opcode) {
			case Memmon_free:
				_free(msg.ptr[0], msg.calls);
				break;
			case Memmon_malloc:
				_malloc(msg.ptr[1], msg.size[0], msg.calls);
				break;
			case Memmon_calloc:
				_calloc(msg.ptr[1], msg.size[1], msg.size[0], msg.calls);
				break;
			case Memmon_realloc:
				_realloc(msg.ptr, msg.size[0], msg.calls);
				break;
			default:
				unknown(n, &msg);
			}
		else
			unknown(n, &msg);
	if (inuse_at_exit)
		inuse();
	if (errfp != stderr)
		fclose(errfp);
	if (temp[0])
		unlink(temp);
	return EXIT_SUCCESS;
}
