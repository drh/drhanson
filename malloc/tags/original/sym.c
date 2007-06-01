#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "sym.h"
#define T Sym_T

struct T {
	int nsyms;		/* number of elements in syms used */
	int size;		/* number of elements in syms */
	struct symbol {		/* symbol: */
		unsigned addr;		/* address */
		char name[16];		/* name */
	} stab[512];		/* an array of symbols */
};

static T expand(T stab) {
	static struct symbol *last;
	extern void *sbrk(int);

	if (stab) {
		struct symbol *new = sbrk(stab->size*sizeof *new);
		if (new == (void *)-1 || last != new)
			return 0;
		assert(last);
	} else {
		stab = sbrk(sizeof *stab);
		if (stab == (void *)-1)
			return 0;
		stab->nsyms = 0;
	}
	last = sbrk(0);
	stab->size = last - stab->stab;
	return stab;
}

T Sym_init(const char *file) {
	FILE *f;
	struct symbol *sp;
	T stab;

	assert(file);
	if ((f = fopen(file, "r")) == 0)
		return 0;
	if ((stab = expand(NULL)) == NULL)
		goto error;
	sp = stab->stab;
	while (!feof(f)) {
		unsigned addr;
		char junk[4], name[100];
		if (fscanf(f, "%x %[tT] _%s\n", (unsigned *)&addr, junk, name) != 3)
			goto error;
		if (stab->nsyms >= stab->size && !expand(stab))
			goto error;
		sp->addr = addr;
		strncpy(sp->name, name, sizeof sp->name);
		sp->name[sizeof(sp->name) - 1] = 0;
		sp++;
		stab->nsyms++;
	}
	fclose(f);
	return stab;

error:	fclose(f);
	return NULL;
}

char *Sym_find(T stab, void *addr) {
	int k = 0, lb = 0, ub;
	unsigned adr = (unsigned)addr;

	assert(stab);
	ub = stab->nsyms - 1;
	while (lb <= ub) {
		k = (lb + ub)/2;
		if (stab->stab[k].addr < adr) {
			if (k+1 == stab->nsyms || adr < stab->stab[k+1].addr)
				return stab->stab[k].name;
			lb = k + 1;
		} else if (stab->stab[k].addr > adr)
			ub = k - 1;
		else
			break;
	}
	return NULL;
}
