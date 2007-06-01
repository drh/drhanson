/* calc: symbol table management */

#include <stdio.h>
#include "calc.h"

struct symbol *htab[HASHSIZE];	/* hash table */
static struct str {		/* string table */
	char *str;		/* string */
	struct str *next;	/* next entry */
} *stab[HASHSIZE];
static struct symbol *freelist = NULL; /* free list of unused entries */
int indent = 0;			/* amount to indent symbol attributes */

/* alloc - allocate n bytes of memory, die if allocation fails */
char *alloc(n)
int n;
{
	char *p, *malloc();

	if ((p = malloc(n)) == NULL)
		error("system error", " in alloc: allocation failed");
	return (p);
}

/* bfree - free storage allocated by alloc */
bfree(p)
char *p;
{
	if (p)
		free(p);
}

/* dump - print entire symbol table or one entry */
dump(p, fp)
struct symbol *p;
FILE *fp;
{
	char *s;
	int i;

	if (fp == NULL)
		fp = stderr;
	if (p == NULL) {
		for (i = 0; i < HASHSIZE; i++)
			for (p = htab[i]; p; p = p->link)
				dump(p, fp);
		return;
		}
	s = p->name;
	if (p->val.type != T_VOID) {
		fprintf(fp, "%*s is ", indent, s);
		image(deref(p->val), "\n", fp);
		s = "";
		}
	if (p->depen) {
		fprintf(fp, "%*s depends on ", indent, s);
		printd(p->depen, "\n", fp);
		s = "";
		}
	if (p->succ) {
		fprintf(fp, "%*s successors are ", indent, s);
		printd(p->succ, "\n", fp);
		s = "";
		}
	if (p->count) {
		fprintf(fp, "%*s has %d predecessor%s\n", indent, s,
			p->count, p->count > 1 ? "s" : "");
		s = "";
		}
	if (p->flags&FUNC) {
		fprintf(fp, "%*s is a %sfunction with ", indent, s,
			p->flags&BUILTIN ? "builtin " : "");
		if (p->amin == p->amax)
			fprintf(fp, p->amin ? "%d" : "no", p->amin);
		else
			fprintf(fp, p->amax!=ARB ? "%d to %d" :	"%d or more",
				p->amin, p->amax);
		fprintf(fp, " argument%s\n", p->amax>1||p->amax==0 ? "s" : "");
		s = "";
		}
}

/* expunge - remove parameters and locals */
expunge()
{
	register int i;
	register struct symbol *p, **q, *r;

	for (i = 0; i < HASHSIZE; i++) {
		q = &htab[i];
		for (p = htab[i]; p; p = r) {
			r = p->link;
			if (p->flags&LOCAL)
				*q = p->link;
			else
				q = &p->link;
			}
		}
}

/* image - print "image" of v on fp, following by str, default stderr */
image(v, str, fp)
struct value v;
char *str;
FILE *fp;
{
	int n, i;
	struct tnode *tp;
	char *s;

	if (fp == NULL)
		fp = stderr;
	switch (v.type) {
		case T_VOID:
			fprintf(fp, "undefined");
			break;
		case T_STRING:
			fprintf(fp, "\"");
			for (n = 40, s = v.string; *s && n; n--, s++)
				if (*s < ' ' || *s >= 0177)
					fprintf(fp, "\\%o", *s);
				else
					fprintf(fp, "%c", *s);
			if (n <= 0)
				fprintf(fp, "...");
			fprintf(fp, "\"");
			break;
		case T_REAL:
			fprintf(fp, "%g", v.real);
			break;
		case T_TABLE:
			fprintf(fp, "[");
			n = i = 0;
			while (i < sizeof(v.tbl->htab)/sizeof(struct tnode *)) {
				for (tp = v.tbl->htab[i++]; tp; tp = tp->link) {
					if (n)
						fprintf(fp, ", ");
					if (tp->index.type == T_TABLE)
						fprintf(fp, "[...]");
					else
						image(tp->index, "", fp);
					fprintf(fp, ":");
					if (tp->value.type == T_TABLE)
						fprintf(fp, "[...]");
					else
						image(tp->value, "", fp);
					if (++n >= 5)
						break;
					}
				if (n >= 5) {
					fprintf(fp, ", ...");
					break;
					}
				}
			fprintf(fp, "]");
			break;
		case T_VAR:
			fprintf(fp, "variable -> ");
			image(deref(v), str, fp);
			return;
		default:
			fprintf(fp, "unknown value [%d, %0o]", v.type,
				v.other);
		}
	fprintf(fp, str);
}


/* initsym - initialize symbol table management */
initsym()
{
	register int i;

	freelist = NULL;
	for (i = 0; i < HASHSIZE; i++) {
		htab[i] = NULL;
		stab[i] = NULL;
		}
	indent = 0;
}

/* install - allocate a new entry for name and install it */
struct symbol *install(name)
char *name;
{
	struct symbol *p;
	int len;

	p = new(name);
	p->link = htab[((int) name)%HASHSIZE];
	htab[((int) name)%HASHSIZE] = p;
	if ((len = strlen(name)) > -indent)
		indent = -len;
	return (p);
}

/* lookup - lookup name with flags, return pointer to entry */
struct symbol *lookup(name, flags)
char *name;
int flags;
{
	struct symbol *p;

	for (p = htab[((int) name)%HASHSIZE]; p; p = p->link)
		if (name == p->name && p->flags&flags)
			return (p);
	return (NULL);
}

/* new - allocate and initialize and new symbol table entry for name */
struct symbol *new(name)
char *name;
{
	struct symbol *p;

	if (p = freelist)
		freelist = p->link;
	else
		p = (struct symbol *) alloc(sizeof (struct symbol));
	p->name = name;
	p->val = newvalue(T_VOID, 0);
	p->flags = p->offset = p->amin = p->amax = p->count = 0;
	p->succ = NULL;
	p->code = p->fcode = NULL;
	return (p);
}

/* strsave - lookup str and install if necessary, return pointer */
char *strsave(str)
char *str;
{
	int h, len;
	char *s, *strcpy();
	struct str *p;

	h = 0;
	for (s = str, len = 0; *s; len++)
		h += *s++;
	for (p = stab[h%HASHSIZE]; p; p = p->next)
		if (strcmp(str, p->str) == 0)
			return (p->str);
	p = (struct str *) alloc(sizeof(struct str));
	p->str = strcpy(alloc(len + 1), str);
	p->next = stab[h%HASHSIZE];
	stab[h%HASHSIZE] = p;
	return (p->str);
}
