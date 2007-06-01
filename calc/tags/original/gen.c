/* calc: code generation */

#include <stdio.h>
#include "calc.h"
#include "y.tab.h"

union inst *pc;			/* location counter */
union inst code[1000];		/* program code */

/* backpatch - fill in offsets to pc starting at p */
backpatch(p, pc)
union inst *p, *pc;
{
	union inst *q;

	for ( ; p; p = q) {
		q = p->addr;
		p->offset = pc - p;
		}
}

/* copycode - copy n code elements */
copycode(n, from, to)
int n;
union inst *from, *to;
{
	while (n--)
		*to++ = *from++;
}

/* dumpcode - print code beginning at pc to fp, default stderr */
dumpcode(pc, fp)
union inst *pc;
FILE *fp;
{
	extern End();
	union inst *base, *p, *dumpinst();

	if (fp == NULL)
		fp = stderr;
	for (base = pc; pc && pc->offset; pc = p) {
		p = dumpinst(pc, base, fp);
		if (pc->op == End)
			break;
		}
}

/* dumpinst - print instruction at pc on fp, default stderr, return new pc */
union inst *dumpinst(pc, base, fp)
union inst *pc, *base;
FILE *fp;
{
	extern struct operator opnames[];
	int i;		

	if (fp == NULL)
		fp = stderr;
	fprintf(fp, "%d:\t", pc - base);
	for (i = 0; opnames[i].op; i++)
		if (pc->op == opnames[i].op)
			break;
	if (opnames[i].op) {
		fprintf(fp, "%s", opnames[i].name);
		if (opnames[i].argc >= 1)
			if ((++pc)->sym->flags&CONST) {
				fprintf(fp, " ");
				image(pc->sym->val, "", fp);
				}
			else					
				fprintf(fp, " %s", pc->sym->name);
		if (opnames[i].argc >= 2)
			fprintf(fp, " %d", (++pc)->op);
		if (opnames[i].argc < 0)
			for (i = opnames[i].argc; i < 0; i++) {
				pc++;
				fprintf(fp, " %d", pc+pc->offset - base);
				}
		fprintf(fp, "\n");
		}
	else
		fprintf(fp, "%0o\n", pc->op);
	return (++pc);
}

/* emit - emit x into code */
emit(x)
union inst x;
{
	if (pc >= &code[sizeof(code)/sizeof(union inst)])
		error("system error", " in emit: code overflow");
	*pc++ = x;
}

/* savecode - save code generated up to pc */
union inst *savecode(pc)
union inst *pc;
{
	union inst *new;

	new = (union inst *) alloc((pc-code)*sizeof(union inst));
	copycode(pc - code, code, new);
	return (new);
}
