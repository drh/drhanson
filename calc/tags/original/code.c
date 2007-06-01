/* calc: interpreter */

#include <stdio.h>
#include <math.h>
#include <setjmp.h>
#include "calc.h"
#include "y.tab.h"

struct frame {			/* function call stack frame */
	struct symbol *sym;	/* symbol table entry for function */
	union inst *retpc;	/* return point */
	struct value *argp;	/* pointer to first argument on stack */
	int nargs;		/* number of arguments */
} frame[100], *fp;		/* call stack & frame pointer */
int level = 0;			/* call level */
union inst *cp;			/* code pointer */
struct value stack[100];	/* evaluation stack */
struct value *sp = stack;	/* evaluation stack pointer */
extern jmp_buf restart;

/* compare - compare a and b, return <0, 0, or >0 */
int compare(a, b)
struct value a, b;
{
	switch (a.type<<8|b.type) {
		case T_STRING<<8|T_STRING:
			return (strcmp(a.string, b.string));
		case T_REAL<<8|T_REAL:
			if (a.real < b.real) 
				return (-1);
			if (a.real > b.real)
				return (1);
			return (0);
		case T_STRING<<8|T_REAL:
			return (compare(cvr(a.string), b));
		case T_REAL<<8|T_STRING:
			return (compare(a, cvr(b.string)));
		case T_TABLE<<8|T_TABLE:
			return (a.tbl - b.tbl);
		default:
			return (a.type - b.type); 
		}
}

/* cvr - convert v to real */
struct value cvr(v)
struct value v;
{
	v = deref(v);
	if (v.type == T_STRING)
		v = newvalue(T_REAL, atof(v.string));
	else if (v.type != T_REAL)
		runtimeError("real", " expected", &v);
	return (v);
}

/* cvs - convert v to string */
struct value cvs(v)
struct value v;
{
	char buf[100];

	v = deref(v);
	if (v.type == T_REAL) {
		sprintf(buf, "%g", v.real);
		v = newvalue(T_STRING, strsave(buf));
		}
	else if (v.type != T_STRING)
		runtimeError("string", " expected", &v);
	return (v);
}

/* deref - dereference variable v */
struct value deref(v)
struct value v;
{
	while (v.type == T_VAR)
		v = *v.var;
	return (v);
}

/* display - display frame f on file fp (default stderr) */
display(f, fp)
struct frame *f;
FILE *fp;
{
	int i;

	if (f == NULL)
		return;
	if (fp == NULL)
		fp = stderr;
	fprintf(fp, "%s(", f->sym->name);
	for (i = 0; i < f->nargs; i++) {
		image(f->argp[i], "", fp);
		if (i < f->nargs-1)
			fprintf(fp, ", ");
		}
	fprintf(fp, ")");
}

/* execute - execute code at address pc */
execute(pc)
union inst *pc;
{
	extern struct value TraceExecute;

	sp = stack;
	fp = frame;
	level = 0;
	cp = pc;
	while (cp) {
		if (TraceExecute.real) {
			dumpinst(cp, NULL, stderr);
			TraceExecute.real--;
			}
		(*cp++->op)();
		}
}

/* hash - return hash number for v */
int hash(v)
struct value v;
{
	struct table *tp;
	char *s;
	unsigned h;

	switch (v.type) {
		case T_VOID:
			return (0);
		case T_VAR:
		case T_REAL:
		case T_TABLE:
			h = v.other&077777;
			break;
		case T_STRING:
			for (h = 0, s = v.string; s && *s; s++)
				h += *s;
			break;
		default:
			error("system error", " in hash: unknown value");
		}
	return (h%(sizeof(tp->htab)/sizeof(struct tnode *)));
}

/* newvalue - construct a new value of type t with associated information x */
struct value newvalue(t, x)
int t;
union u_info x;
{
	int i;
	struct value v;

	switch (v.type = t) {
		case T_VOID:
			v.other = 0;
			break;
		case T_REAL:
		case T_STRING:
		case T_VAR:
			v.u_info = x;
			break;
		case T_TABLE:
			v.tbl = (struct table *) alloc(sizeof(struct table));
			v.tbl->size = i = 0;
			while (i < sizeof(v.tbl->htab)/sizeof(struct tnode *))
				v.tbl->htab[i++] = NULL;
			break;
		default:
			error("system error", " in newvalue: unknown type");
		}
	return (v);
}

/* print - print v on fp, default stderr */
print(v, fp)
struct value v;
FILE *fp;
{
	struct tnode *tp;
	int i, n;

	if (fp == NULL)
		fp = stderr;
	switch (v.type) {
		case T_VOID:
			break;
		case T_STRING:
			fprintf(fp, "%s", v.string);
			break;
		case T_REAL:
			fprintf(fp, "%g", v.real);
			break;
		case T_TABLE:
			fprintf(fp, "[");
			n = i = 0;
			while (i < sizeof(v.tbl->htab)/sizeof(struct tnode *))
				for (tp = v.tbl->htab[i++]; tp; tp = tp->link) {
					if (n)
						fprintf(fp, ", ");
					if (tp->index.type == T_TABLE)
						fprintf(fp, "[...]");
					else
						print(tp->index, fp);
					fprintf(fp, ":");
					if (tp->value.type == T_TABLE)
						fprintf(fp, "[...]");
					else
						print(tp->value, fp);
					n++;
					}
			fprintf(fp, "]");
			break;
		default:
			error("system error", " in print: unknown value");
		}
}

/* push - push v onto the stack */
push(v)
struct value v;
{
	if (sp >= &stack[sizeof(stack)/sizeof(struct value)])
		error("system error", " in push: stack overflow");
	*++sp = v;
}

/* rnd - round r to n decimal places, where  0 <= n <= 6 */
double rnd(r, n)
double r;
int n;
{
	static double tens[] = { 1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6};
	static double eps = 0.0002;
	double r1;

	if (n < 0 || n > 6)
		error("system error", " in rnd: bad decimal places");
	r1 = fabs(r) + (0.5 + eps)/tens[n];
	r1 = floor(r1) + ((int)(tens[n]*(r1 - floor(r1))))/tens[n];
	return (r < 0.0 ? -r1 : r1);
}

/* runtimeError - issue runtime error message msg, str, offending value *vp */
runtimeError(msg, str, vp)
char *msg, *str;
struct value *vp;
{
	extern struct dnode *tlist;
	extern struct symbol *current;
	extern int errno;

	fprintf(stderr, "%s%s", msg, str);
	if (fp > frame) {
		fprintf(stderr, " in ");
		display(fp, stderr);
		}
	if (current) {
		fprintf(stderr, " while computing %s", current->name);
		current = NULL;
		}
	fprintf(stderr, "\n");
	if (vp) {
		fprintf(stderr, "offending value: ");
		image(*vp, "\n", stderr);
		}
	errno = 0;
	release(tlist);
	tlist = NULL;
	longjmp(restart, 1);
}

/* tindex - index table t with x, install if flag!=0, return tnode pointer */
struct tnode *tindex(t, x, flag)
struct value t, x;
int flag;
{
	struct tnode *tp;
	int i;

	for (tp = t.tbl->htab[i=hash(x)]; tp; tp = tp->link)
		if (compare(tp->index, x) == 0)
			break;
	if (tp == NULL && flag) {
		tp = (struct tnode *) alloc(sizeof(struct tnode));
		tp->index = x;
		tp->link = t.tbl->htab[i];
		t.tbl->htab[i] = tp;
		}
	return (tp);
}

/* Add - [T_REAL, x] [T_REAL, y] | [T_REAL, x + y] */
Add()
{
	*(sp-1) = cvr(*(sp-1));
	*sp = cvr(*sp);
	(sp-1)->real += sp->real;
	sp--;
}

/* Call - ... a b ... | ... a b ... (new frame) */
Call()
{
	struct value v;
	int i, n;

	if ((cp->sym->flags&FUNC) == 0)
		runtimeError("undefined function: ", cp->sym->name, NULL);
	if ((n = cp[1].offset) < cp->sym->amin || n > cp->sym->amax)
		runtimeError(n > cp->sym->amax ? "too many arguments for " :
			"too few arguments for ", cp->sym->name, NULL);
	v = newvalue(T_VOID, 0);
	if (fp+1 >= &frame[sizeof(frame)/sizeof(struct frame)])
		runtimeError("call stack overflow", "", NULL);
	(++fp)->sym = cp->sym;
	level++;
	fp->nargs = n;
	fp->argp = sp - n + 1;
	while (n < fp->sym->offset)
		push(v), n++;
	for (n = 0; n < fp->nargs; n++)
		fp->argp[n] = deref(fp->argp[n]);
	if (trace) {
		for (i = level; i > 0; i--)
			fprintf(stderr, ".");
		display(fp, stderr);
		fprintf(stderr, "\n");
		trace--;
		}
	if (cp->sym->flags&BUILTIN) {
		v = (*cp->sym->f)(fp->nargs, fp->argp);
		sp -= n;
		if (trace) {
			for (i = level; i > 0; i--)
				fprintf(stderr, ".");
			fprintf(stderr, "%s returned ",	fp->sym->name);
			image(v, "\n", stderr);
			trace--;
			}
		push(v);
		cp += 2;
		fp--;
		level--;
		}
	else {
		fp->retpc = cp + 2;
		cp = fp->sym->fcode;
		}
}

/* Concatenate - [T_STRING, x] [T_STRING, y] | [T_STRING, x||y] */
Concatenate()
{
	int len1, len2;
	char *strcpy();

	*(sp-1) = cvs(*(sp-1));
	*sp = cvs(*sp);
	if ((len1 = strlen((sp-1)->string)) == 0)
		(sp-1)->string = sp->string;
	else if (len2 = strlen(sp->string)) {
		(sp-1)->string = strcpy(alloc(len1+len2+1), (sp-1)->string);
		strcat((sp-1)->string, sp->string);
		}
	sp--;
}

/* Div - [T_REAL, x] [T_REAL, y] | [T_REAL, x/y] */
Div()
{
	*(sp-1) = cvr(*(sp-1));
	*sp = cvr(*sp);
	if (sp->real == 0.0)
		runtimeError("division by zero", "", sp);
	(sp-1)->real /= sp->real;
	sp--;
}

/* Dup - ... a | ... a a */
Dup()
{
	push(*sp);
}

/* End - terminate execution */
End()
{
	cp = NULL;
}

/* Equal - a b | if a == b then b else [T_VOID, 0] */
Equal()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) == 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* GlobalAddress - | [T_VAR, adr] */
GlobalAddress()
{
	push(newvalue(T_VAR, &cp++->sym->val));
}

/* GtrOrEqual - a b | if a >= b then b else [T_VOID, 0] */
GtrOrEqual()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) >= 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* GtrThan - a b | if a > b then b else [T_VOID, 0] */
GtrThan()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) > 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* Jump - ... | ... */
Jump()
{
	cp += cp->offset;
}

/* JumpIfVoid - ... a | ... */
JumpIfVoid()
{
	*sp = deref(*sp);
	if ((sp--)->type == T_VOID)
		cp += cp->offset;
	else
		cp++;
}

/* JumpNotVoid - ... a | ... */
JumpNotVoid()
{
	*sp = deref(*sp);
	if ((sp--)->type != T_VOID)
		cp += cp->offset;
	else
		cp++;
}

/* LessOrEqual - a b | if a <= b then b else [T_VOID, 0] */
LessOrEqual()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) <= 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* LessThan - a b | if a < b then b else [T_VOID, 0] */
LessThan()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) < 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* Lindex - [T_TABLE, t] a | [T_VAR, adr t[a]] */
Lindex()
{
	struct value v;
	struct tnode *tp;

	*sp = deref(*sp);
	v = deref(*(sp-1));
	if (v.type != T_TABLE) {
		v = newvalue(T_TABLE, 0);
		if ((sp-1)->type != T_VAR)
			runtimeError("variable", " expected for [", sp - 1);
		while ((sp-1)->var->type == T_VAR)
			*(sp-1) = *(sp-1)->var;
		*(sp-1)->var = v;
		}
	tp = tindex(v, *sp, 1);
	*--sp = newvalue(T_VAR, &tp->value);
}

/* LocalAddress - | [T_VAR, adr] */
LocalAddress()
{
	push(newvalue(T_VAR, &fp->argp[cp++->sym->offset-1]));
}

/* Mul - [T_REAL, x] [T_REAL, y] | [T_REAL, x*y] */
Mul()
{
	*(sp-1) = cvr(*(sp-1));
	*sp = cvr(*sp);
	(sp-1)->real *= sp->real;
	sp--;
}

/* Negate - [T_REAL, x] | [T_REAL, -x] */
Negate()
{
	*sp = cvr(*sp);
	sp->real = -sp->real;
}

/* Not - [t, -] | if t is T_VOID then [T_STRING, ""] else [T_VOID, 0] */
Not()
{
	*sp = deref(*sp);
	if (sp->type != T_VOID)
		*sp = newvalue(T_VOID, 0);
	else	
		*sp = newvalue(T_STRING, "");
}

/* NotEqual - a b | if a ~= b then b else [T_VOID, 0] */
NotEqual()
{
	*(sp-1) = deref(*(sp-1));
	*sp = deref(*sp);
	if (sp->type != T_VOID && (sp-1)->type != T_VOID &&
		compare(*(sp-1), *sp) != 0)
			*(sp-1) = *sp;
	else
		*(sp-1) = newvalue(T_VOID, 0);
	sp--;
}

/* Pop - ... a | ... */
Pop()
{
	sp--;
}

/* Pow - [T_REAL, x] [T_REAL, y] | [T_REAL, x^y] */
Pow()
{
	*sp = cvr(*sp);
	*(sp-1) = cvr(*(sp-1));
	(sp-1)->real = pow((double) ((sp-1)->real), (double) sp->real);
	errcheck(NULL, "pow");
	sp--;
}

/* Print - ... a | ... */
Print()
{
	if (sp->type != T_VOID) {
		printf("\t");
		print(deref(*sp--), stdout);
		printf("\n");
		}
}

/* Push - | [any, -] */
Push()
{
	push(cp++->sym->val);
}

/* Return - ... a | a (in previous frame) */
Return()
{
	int i;

	if (level == 0 || fp == frame)
		error("system error", " in Return: extraneous return");
	*fp->argp = deref(*sp);
	if (trace) {
		for (i = level; i > 0; i--)
			fprintf(stderr, ".");
		fprintf(stderr, "%s returned ", fp->sym->name);
		image(*fp->argp, "\n", stderr);
		trace--;
		}
	sp = fp->argp;
	cp = fp->retpc;
	fp--;
	level--;
}

/* Rindex - [T_TABLE, t] a | t[a] */
Rindex()
{
	struct value v;
	struct tnode *tp;

	v = deref(*sp--);
	*sp = deref(*sp);
	if (sp->type == T_TABLE && (tp = tindex(*sp, v, 0)))
		*sp = tp->value;
	else
		*sp = newvalue(T_VOID, 0);
}

/* Store - [T_VAR, adr] a | [T_VAR, adr] */
Store()
{
	extern struct value Precision;

	*sp = deref(*sp);
	if (sp->type == T_REAL && Precision.type == T_REAL &&
		Precision.real >= 0.0 && Precision.real <= 6.0)
			sp->real = rnd(sp->real, (int) Precision.real);
	if ((sp-1)->type != T_VAR)
		runtimeError("variable", " expected for =", sp - 1);
	while ((sp-1)->var->type == T_VAR)
		*(sp-1) = *(sp-1)->var;
	*(sp-1)->var = *sp;
	sp--;
}

/* Sub - [T_REAL, x] [T_REAL, y] | [T_REAL, x - y] */
Sub()
{
	*(sp-1) = cvr(*(sp-1));
	*sp = cvr(*sp);
	(sp-1)->real -= sp->real;
	sp--;
}

/* Swap - ... a b | ... b a */
Swap()
{
	struct value t;

	t = *sp;
	*sp = *(sp-1);
	*(sp-1) = t;
}

struct operator opnames[] = {
	1, GlobalAddress,"address of",
	1, LocalAddress,"address of local",
	1, Push,	"push",
	0, Add,		"add",
	0, Sub,		"subtract",
	0, Mul,		"multiply",
	0, Div,		"divide",
	0, Pow,		"power",
	0, Negate,	"negate",
	-1, Jump,	"jump",
	-1, JumpIfVoid,	"jump if void",
	-1, JumpNotVoid,"jump if not void",
	0, Not,		"not",	
	0, LessThan,	"less than",
	0, LessOrEqual,	"less than or equal",
	0, Equal,	"equal",
	0, NotEqual,	"not equal",
	0, GtrOrEqual,	"greater than or equal",
	0, GtrThan,	"greater than",
	0, Store,	"store",
	0, Concatenate,	"concatenate",
	0, End,		"end",
	2, Call,	"call",
	0, Lindex,	"lindex",
	0, Rindex,	"rindex",
	0, Return,	"return",
	0, Swap,	"swap",
	0, Pop,		"pop",
	0, Dup,		"dup",
	0, Print,	"print",
	0, NULL,	"?"
};
