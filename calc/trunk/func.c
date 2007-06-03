/* calc: built-in functions */

#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "calc.h"

extern int errno;
struct value voidval;
struct value Trace;
struct value Tracecode;
struct value TraceCompute;
struct value TraceExecute;
struct value Precision;

/* abs(x) - return |x| */
static struct value Abs(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvr(*ap);
	v.real = fabs((double) v.real);
	return (v);
}

/* exp(x) - return e ^ x */
static struct value Exp(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvr(*ap);
	v.real = exp((double) v.real);
	errcheck(ap, "");
	return (v);
}

/* log(x) - return natural log of x */
static struct value Log(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvr(*ap);
	v.real = log((double) v.real);
	errcheck(ap, "");
	return (v);
}

/* log10(x) - return log base 10 of x */
static struct value Log10(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvr(*ap);
	v.real = log10((double) v.real);
	errcheck(ap, "");
	return (v);
}

/* max(x,...) - return maximum of x,... */
static struct value Max(n, ap)
int n;
struct value *ap;
{
	struct value v, t;

	v = cvr(*ap++);
	for (n--; n--; ) {
		t = cvr(*ap++);
		if (t.real > v.real)
			v = t;
		}
	return (v);
}

/* mean(x,...) - return mean of of x,... */
static struct value Mean(n, ap)
int n;
struct value *ap;
{
	int k;
	struct value v, t;

	v = newvalue(T_REAL, 0.0);
	k = n;
	while (n--) {
		t = cvr(*ap++);
		v.real += t.real;
		}
	v.real /= k;
	return (v);
}

/* min(x,...) - return minimum of x,... */
static struct value Min(n, ap)
int n;
struct value *ap;
{
	struct value v, t;

	v = cvr(*ap++);
	for (n--; n--; ) {
		t = cvr(*ap++);
		if (t.real < v.real)
			v = t;
		}
	return (v);
}

/* print(x,...) - print arguments x,... */
static struct value Print(n, ap)
int n;
struct value *ap;
{
	struct symbol *sym;
	extern struct symbol *globals;

	if (n)
		while (n-- > 0)
			print(*ap++, stdout);
	else if (sym = globals)
		do {
			sym = sym->class;
			if (sym->flags&DEFERRED) {
				fprintf(stdout, "%s = ", sym->name);
				print(sym->val, stdout);
				fprintf(stdout, "\n");
				}
			} while (sym != globals);
	return (newvalue(T_VOID, 0));
}

/* read(x) - print x and read a line */
static struct value Read(n, ap)
int n;
struct value *ap;
{
	char line[120];

	if (ap->type != T_VOID)
		print(cvs(*ap), stdout);
	if (fgets(line, sizeof(line), stdin)) {
		line[strlen(line)-1] = '\0';
		return (newvalue(T_STRING, strsave(line)));
		}
	return (newvalue(T_VOID, 0));
}
	
/* round(x [, n]) - round x to n decimal places, default is 0 */
static struct value Round(n, ap)
int n;
struct value *ap;
{
	struct value v;
	int k = 0;

	if (n == 2) {
		v = cvr(*(ap+1));
		k = (int) v.real;
		}
	if (k < 0 || k > 6)
		runtimeError("second", " argument out of domain", ++ap);
	v = cvr(*ap);
	v.real = rnd(v.real, k);
	return (v);
}

/* show(x,...) - print information about x,... */
static struct value Show(n, ap)
int n;
struct value *ap;
{
	struct value v;
	struct symbol *p;

	if (n == 0)
		dump(NULL, stdout);
	else
		while (n-- > 0) {
			v = cvs(*ap++);
			if (p = lookup(strsave(v.string), GLOBAL))
				dump(p, stdout);
			}
	return (newvalue(T_VOID, 0));
}

/* showcode(x,...) - print compiled code for functions x,... */
static struct value Showcode(n, ap)
int n;
struct value *ap;
{
	struct value v;
	struct symbol *p;
	int i;

	for (i = 0; i < n; i++) {
		v = cvs(*ap++);
		if ((p = lookup(strsave(v.string), GLOBAL)) &&
			p->flags&FUNC && p->fcode) {
				if (n > 1)
					fprintf(stdout, "%s:\n", p->name);
				dumpcode(p->fcode, stdout);
				}
		}
	return (newvalue(T_VOID, 0));
}

/* sqrt(x) - return square root of x */
static struct value Sqrt(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvr(*ap);
	v.real = sqrt((double) v.real);
	errcheck(ap, "");
	return (v);
}

/* sum(x,...) - compute sum of arguments, including tables */
static struct value Sum(n, ap)
int n;
struct value *ap;
{
	struct value v, sum;
	struct tnode *tp;
	int i;

	sum = newvalue(T_REAL, 0.0);
	for ( ; n--; ap++)
		if (ap->type == T_TABLE) {
			i = 0;
			while (i<sizeof(ap->tbl->htab)/sizeof(struct tnode *))
				for (tp = ap->tbl->htab[i++]; tp; tp=tp->link){
					v = cvr(tp->value);
					sum.real += v.real;
					}
			}
		else {
			v = cvr(*ap);
			sum.real += v.real;
			}
	return (sum);
}

/* system(s) - execute Unix command s */
static struct value System(n, ap)
int n;
struct value *ap;
{
	struct value v;

	v = cvs(*ap);
	system(v.string);
	return (newvalue(T_VOID, 0));
}

/* table(x,y,...) - create table t with t[x]=y,... */
static struct value Table(n, ap)
int n;
struct value *ap;
{
	struct value t;
	struct tnode *tp;

	t = newvalue(T_TABLE, 0);
	while (n-- > 0) {
		tp = tindex(t, *ap++, 1);
		if (n--)
			tp->value = *ap++;
		else
			tp->value = newvalue(T_VOID, 0);
		}
	return (t);
}

/* type(x) - return type of x */
static struct value Type(n, ap)
int n;
struct value *ap;
{
	switch (ap->type) {
		case T_VOID:
			return (newvalue(T_STRING, ""));
		case T_REAL:
			return (newvalue(T_STRING, "real"));
		case T_STRING:
			return (newvalue(T_STRING, "string"));
		case T_TABLE:
			return (newvalue(T_STRING, "table"));
		}
}

/* errcheck - check for domain and range errors */
errcheck(vp, s)
struct value *vp;
char *s;
{
	extern int errno;

	if (errno == EDOM)
		runtimeError(s, "argument out of domain", vp);
	else if (errno == ERANGE)
		runtimeError("", "result too large", vp);
	errno = 0;
}

struct functions {
	char *name;
	short amin;
	short amax;
	struct value (*f)();
} functions[] = {
	"abs", 1, 1, Abs,
	"exp", 1, 1, Exp,
	"log", 1, 1, Log,
	"log10", 1, 1, Log10,
	"max", 1, ARB, Max,
	"mean", 1, ARB, Mean,
	"min", 1, ARB, Min,
 	"print", 0, ARB, Print,
	"read", 1, 1, Read,
	"round", 1, 2, Round,
	"show", 0, ARB, Show,
	"showcode", 0, ARB, Showcode,
	"sqrt", 1, 1, Sqrt,
	"sum", 1, ARB, Sum,
	"system", 1, 1, System,
	"table", 0, ARB, Table,
 	"type",	1, 1, Type,
	NULL,
};
struct values {
	char *name;
	int type;
	char *val;
	struct value *vp;
} values[] = {
 	"trace", T_REAL, "0", &Trace,
 	"tracecode", T_REAL, "0", &Tracecode,
 	"tracecompute", T_REAL, "0", &TraceCompute,
 	"traceexecute", T_REAL, "0", &TraceExecute,
 	"precision", T_VOID, NULL, &Precision,
	"undefined", T_VOID, NULL, &voidval,
	NULL,
};

/* initbuiltin - initialize builtin functions and values */
initbuiltin()
{
	struct functions *f;
	struct values *v;
	struct symbol *p;

	for (v = values; v->name; v++) {
		p = install(strsave(v->name));
		p->val = newvalue(T_VAR, v->vp);
		if (v->type == T_REAL)
			*v->vp = newvalue(T_REAL, atof(v->val));
		else if (v->type == T_STRING)
			*v->vp = newvalue(T_STRING, strsave(v->val));
		p->flags |= GLOBAL;
		}
	for (f = functions; f->name; f++) {
		p = install(strsave(f->name));
		p->val = newvalue(T_VOID, 0);
		p->amin = f->amin;
		p->amax = f->amax;
		p->f = f->f;
		p->flags |= GLOBAL|FUNC|BUILTIN;
		}
}
