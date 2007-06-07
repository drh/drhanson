/* ez compiler: top-level parsing */

#include <stdio.h>
#include "ez.h"
#include "tokens.h"

static int follow[] = {END, 0};		/* follow set for procedures */
struct value errors = {T_VOID, 0};	/* current value of `errors' */
static struct value err = {T_VOID, 0};	/* string "errors" */
static int errp = 0;			/* virtual address of error string */
extern char *errs;
extern int elength;

/* prog	: { stmt }
 *
 * ezparse - parse ez program in string v
 */
struct value ezparse(v)
struct value v;
{
	struct proc *p;

	initlex(NULL, &v, NULL);
	errs = SOPENW(errp = salloc());
	t = gtok('\n');
	p = procbeg(NULL);
	endpoint(begpoint());
	stmtlist(0);
	endpoint(begpoint());
	emit1(O_RET);
	emit(2, O_ERR, 1);
	p->p_source = v;
	v = mkval(T_PROC, 0, procend(p));
	newline();
	if (t != EOF)
		error("syntax error", "");
	geterrors();
	if (nerrors)
		v = VOID;
	return (v);
}

/* func	: procedure [ id ] `(' [ id { , id } ] `)' { dcl }  { stmt } end
 *
 * parse procedure definition
 */
int func()
{
	struct compile csav;
	extern int cursor, tcursor;
	extern struct value src;
	int n, a, bpos, epos;
	struct proc *p;

	csav = c;
	bpos = tcursor;
	t = gtok('\n');
	if (t == ID) {
		p = procbeg(lexval.l_str);
		t = gtok('\n');
		}
	else
		p = procbeg(NULL);
	c.offset = bpos - 1;
	endpoint(begpoint());
	newline();
	mustbe('(', '\n');
	while (t == ID) {
		dcl(lexval.l_str, p, 0);
		t = gtok('\n');
		if (t != ',')
			break;
		t = gtok('\n');
		}
	mustbe(')', '\n');
	while (t == LOCAL) {
		t = gtok('\n');
		while (t == ID) {
			dcl(lexval.l_str, p, LOCAL);
			t = gtok(0);
			if (t != ',')
				break;
			t = gtok('\n');
			}
		newline();
		}
	stmtlist(0);
	endpoint(begpoint());
	emit(2, O_GVAL, constant(VOID));
	emit1(O_RET);
	emit(2, O_ERR, 1);
	epos = cursor - 1;
	if (t == END)
		t = gtok(0);
	else {
		error("missing end", "");
		skipto(follow, 0);
		}
	if (t == EOF)
		epos++;
	p->p_source = substr(src, bpos, epos);
	a = procend(p);
	c = csav;
	return (a);
}

/* dcl - declaration of formal parameter or local in p */
dcl(id, p, lflag)
char *id;
struct proc *p;
int lflag;
{
	struct value *vp;

	vp = tindex(p->p_sym, stralloc(id, -1), 1);
	if (TYPE(*vp) != T_VOID)
		error("parameter or local previously declared: ", id);
	else if (lflag == LOCAL) 
		*vp = mkval(T_VAR, 0, ++p->p_nlocals);
	else
		*vp = mkval(T_VAR, 0, -++p->p_nargs);
	putblk(vp, 1);
	bfree(id);
}

/* geterrors - assign compilation error string to `errors' */
geterrors()
{
	struct value *vp;
	extern struct root *rp;

	if (errs)
		SCLOSEW(errs);
	if (nerrors) {
		if (TYPE(err) != T_STRING)
			err = stralloc("errors", -1);
		vp = tindex(rp->r_wglobals, err, 1);
		*vp = errors = mkstr(errp, elength);
		putblk(vp, 1);
		}
}

/* inset - return t if it is in set, return 0 otherwise */
int inset(t, set)
int t, *set;
{
	while (*set)
		if (t == *set++)
			return (t);
	return (0);
}

/* recompile - recompile source for procedure p */
int recompile(p)
struct proc *p;
{
	int a;
	struct proc *p1;

	initlex(NULL, &p->p_source, NULL);
	errs = SOPENW(errp = salloc());
	t = gtok('\n');
	if (t == PROCEDURE)
		a = func();
	else {
		p1 = procbeg(NULL);
		endpoint(begpoint());
		stmtlist(0);
		endpoint(begpoint());
		emit1(O_RET);
		emit(2, O_ERR, 1);
		p1->p_source = p->p_source;
		a = procend(p1);
		newline();
		if (t != EOF)
			error("syntax error", "");
		}
	geterrors();
	if (nerrors == 0) {	/* overwrite previous procedure block */
		p1 = (struct proc *) getblk(a);
		*p = *p1;
		putblk(p1, 0);
		touch(p);
		}
	return (nerrors);
}

/* skipto - discard input upto next occurrence of character in set or EOF */
skipto(set, ic)
int *set, ic;
{
	while (t != EOF && inset(t, set) == 0)
		t = gtok(ic);
}
