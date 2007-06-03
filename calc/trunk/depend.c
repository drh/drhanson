/* calc: dependency analysis */

#include <stdio.h>
#include "calc.h"

static struct dnode *freelist = NULL;	/* list of free dnodes */
struct dnode *queue = NULL;		/* computation order */
struct symbol *current = NULL;		/* current computation entry */

extern struct value TraceCompute;

/* add - add a to d if not already present, return new list */
struct dnode *add(a, d)
struct symbol *a;
struct dnode *d;
{
	struct dnode *dp;

	if ((dp = d) == NULL)
		return (dlist(a));
	do {
		dp = dp->link;
		if (dp->sym == a)
			return (d);
		} while (dp != d);
	return (append(d, dlist(a)));
}

/* append - append d2 to d1, return d2 */
struct dnode *append(d1, d2)
struct dnode *d1, *d2;
{
	struct dnode *dp;

	if (d2 == NULL)
		return (d1);
	if (d1 == NULL)
		return (d2);
	dp = d2->link;
	d2->link = d1->link;
	d1->link = dp;
	return (d2);
}

/* compute - compute values for all variables on d */
compute(d)
struct dnode *d;
{
	struct dnode *dp;

	if (dp = d)
		do {
			dp = dp->link;
			current = dp->sym;
			if (TraceCompute.real)
				fprintf(stderr, "computing %s = ",
					current->name);
			execute(current->code);
			if (TraceCompute.real) {
				image(current->val, "\n", stderr);
				TraceCompute.real--;
				}
			} while (dp != d);
	current = NULL;
}

/* dlist - create a 1-element dlist for p */
struct dnode *dlist(p)
struct symbol *p;
{
	struct dnode *dp;

	if (dp = freelist)
		freelist = dp->link;
	else
		dp = alloc(sizeof(struct dnode));
	dp->sym = p;
	dp->link = dp;
	return (dp);
}

/* enter - enter relation a < b */
enter(a, b)
struct symbol *a, *b;
{
	if ((a->flags&DEFERRED) == 0)
		return;
	b->count++;
	a->succ = add(b, a->succ);
}

/* printd - print dependency list d followed by s on fp (default stderr) */
printd(d, s, fp)
struct dnode *d;
char *s;
FILE *fp;
{
	struct dnode *dp;
	int i;

	if (fp == NULL)
		fp = stderr;
	if (d) {
		dp = d;
		i = 0;
		do {
			dp = dp->link;
			if (dp == d)
				fprintf(fp, "%s", i >= 1 ? " and " : "");
			else if (i)
				fprintf(fp, ", ");
			fprintf(fp, "%s", dp->sym->name);
			i++;
			} while (dp != d);
		}
	if (s)
		fprintf(fp, "%s", s);
}

/* release - release dlist d */
release(d)
struct dnode *d;
{
	struct dnode *dp;

	if (d) {
		dp = d->link;
		d->link = freelist;
		freelist = dp;
		}
}

/* tsort - topologically sort those symbols with flags and return list */
struct dnode *tsort(flags)
int flags;
{
	int i, n;
	extern struct symbol *htab[];
	struct symbol *p;
	struct dnode *dp, *dp1, *q, *new;

	n = 0;
	for (i = 0; i < HASHSIZE; i++)	/* create successor lists and counts */
		for (p = htab[i]; p; p = p->link)
			if (p->flags&flags) {
				if (dp1 = p->depen) 
					do {
						dp1 = dp1->link;
						enter(dp1->sym, p);
						} while (dp1 != p->depen);
				n++;
				}
	if (n == 0)
		return (NULL);
	if (TraceCompute.real) {
		fprintf(stderr, "sorting\n");
		TraceCompute.real--;
		}
	q = NULL;
	for (i = 0; i < HASHSIZE; i++)	/* find zero counts, build queue */
		for (p = htab[i]; p; p = p->link)
			if ((p->flags&flags) && p->count == 0)
				q = append(q, dlist(p));
	new = NULL;
	while (q) {		/* build new list in topogical order */
		dp = q->link;
		if (dp == q)
			q = NULL;
		else
			q->link = dp->link;
		new = append(new, dlist(dp->sym));
		n--;
		if (dp1 = dp->sym->succ)
			do {
				dp1 = dp1->link;
				if (--dp1->sym->count == 0)
					q = append(q, dlist(dp1->sym));
				} while (dp1 != dp->sym->succ);
		release(dp->sym->succ);
		dp->sym->succ = NULL;
		dp->link = freelist;
		freelist = dp;
		}
	if (n) {
		for (i = 0; i < HASHSIZE; i++)	/* clean counts & lists */
			for (p = htab[i]; p; p = p->link)
				if (p->flags&flags) {
					p->count == 0;
					release(p->succ);
					p->succ = NULL;
					}
		fprintf(stderr, "partial list is ");
		printd(new, "\n", stderr);
		release(new);
		runtimeError("circular dependencies", "", NULL);
		}
	return (new);
}

