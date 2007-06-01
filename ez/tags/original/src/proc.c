/* ez: procedures/activations */

#include <stdio.h>
#include "ez.h"

extern int level;
extern int trace;
struct value Procedure = {T_VOID, 0, 0};	/* entry `Procedure' */
struct value Resumption = {T_VOID, 0, 0};	/* entry `Resumption' */

/* cloop - called by create for each element of a procedure's symbol table */
static int cloop(iv, v, tv, fp)
struct value *iv, *v, *tv;
struct frame *fp;
{
	struct value *vp;

	vp = tindex(*tv, *iv, 1);
	if (TYPE(*v) == T_VAR) {
		*vp = VOID;	/* initialize argument or local */
		if (v->v_addr > 0)
			fp->f_vals[fp->f_lp+v->v_addr] = mkval(T_VAR, VIRTUAL,
				virtaddr(vp));
		else
			fp->f_vals[-v->v_addr] = mkval(T_VAR, VIRTUAL,
				virtaddr(vp));
		}
	else
		*vp = *v;
	putblk(vp, 1);
	return (0);
}

/* create - create and return a table (activation record) for v */
struct value create(v)
struct value v;
{
	int i;
	struct value t, *vp;
	struct frame *fp;
	struct table *bp;
	struct proc *p;

	if (TYPE(v) != T_PROC)
		return (VOID);
	p = (struct proc *) getblk(v.v_addr);
	t = mkval(T_TABLE, 0, virtaddr(bp = (struct table *) balloc(B_TABLE)));
	fp = (struct frame *) balloc(B_FRAME);
	fp->f_prev = NULL;
	fp->f_table = t;
	fp->f_lp = p->p_nargs;
	tseq(p->p_sym, cloop, &t, fp);
	fp->f_pc = p->p_code;
	fp->f_spbase = fp->f_sp = p->p_nargs + p->p_nlocals;
	fp->f_vals[0] = v;
	vp = tindex(t, Procedure, 1);
	*vp = v;
	putblk(vp, 1);
	vp = tindex(t, Resumption, 1);
	*vp = mkint(1);
	putblk(vp, 1);
	putblk(p, 0);
	putblk(bp, 1);
	t.v_addr = virtaddr(fp);
	putblk(fp, 1);
	return (t);
}

/* pidx - compute f[v], i.e. index a procedure */
struct value pidx(f, v)
struct value f, v;
{
	struct value v1, *vp;
	struct point *pt;
	struct proc *p;

	if (XFIELD(f))
		return (VOID);
	p = (struct proc *) getblk(f.v_addr);
	if (p->p_code == 0 && recompile(p)) {
		putblk(p, 0);
		return (VOID);
		}
	v1 = excvi(v);
	if (TYPE(v1) == T_INT) {
		pt = pindex(p, v1.v_addr, NULL);
		if (pt && pt->pc)
			v = substr(excvstr(p->p_source, NULL), pt->pos[0],
				pt->pos[1]);
		else
			v = VOID;
		if (pt)
			putblk(pt, 0);
		}	
	else if (TYPE(v) == T_STRING)
		if (vp = tindex(p->p_sym, v, 0)) {
			v = *vp;
			if (TYPE(v) == T_VAR)
				v = VOID;
			putblk(vp, 0);
			}
		else
			v = VOID;
	else
		v = VOID;
	putblk(p, 0);
	return (v);
}

/* pindex - find/install the ith resumption point in procedure bp */
struct point *pindex(bp, i, pt)
struct proc *bp;
int i;
struct point *pt;
{
	struct points *p;
	int k;

	k = i/(sizeof(p->n_points)/sizeof(struct point));
	i %= (sizeof(p->n_points)/sizeof(struct point));
	if (bp->p_points[k] == NULL) {
		if (pt == NULL)
			return (NULL);
		p = (struct points *) balloc(B_POINTS);
		bp->p_points[k] = virtaddr(p);
		touch(bp);
		}
	else
		p = (struct points *) getblk(bp->p_points[k]);
	if (pt) {
		p->n_points[i] = *pt;
		touch(p);
		}
	return (&p->n_points[i]);
}

/* prproc - print call of procedure f with n arguments ap[0..n-1] */
static prproc(f, n, ap)
struct value f, *ap;
int n;
{
	int i;
	struct proc *p;

	p = (struct proc *) getblk(f.v_addr);
	if (TYPE(p->p_name) == T_STRING)
		excvstr(p->p_name, stderr);
	else
		fprintf(stderr, "?");
	fprintf(stderr, "(");
	for (i = 1; i <= n; i++) {
		image(deref(*ap++), stderr, 0);
		if (i < n)
			fprintf(stderr, ", ");
		}
	fprintf(stderr, ")");
	putblk(p, 0);
}

/* pseq - sequence through the resumption points of procedure v calling
 * f(struct value *index, struct value *value, a, b) for each
 * resumption point index; value is the corresponding substring.
 * If returns a non-zero value, the loop is broken and pseq returns
 * that value, otherwise pseq returns 0. */
int pseq(v, f, a, b)
struct value v;
int (*f)(), *a, *b;
{
	struct value index, value;
	int i, j, x;
	struct points *p;
	struct proc *bp;

	bp = (struct proc *) getblk(v.v_addr);
	index = mkint(0);
	for (i = 0; i < sizeof(bp->p_points)/sizeof(int); i++) {
		if (bp->p_points[i] == 0)
			break;
		p = (struct points *) getblk(bp->p_points[i]);
		for (j = 0; j < sizeof(p->n_points)/sizeof(struct point); j++)
			if (p->n_points[j].pc) {
				index.v_addr++;
				value = substr(bp->p_source,
						p->n_points[j].pos[0],
						p->n_points[j].pos[1]);
				if (x = (*f)(&index, &value, a, b)) {
					putblk(p, 0);
					putblk(bp, 0);
					return (x);
					}
				}
		putblk(p, 0);
		}
	putblk(bp, 0);
	return (0);
}

/* psubstr - compute substring of procedure v, widening *i1, *i2 as necessary.
 * Returns widened values of *i1 and *i2 */
struct value psubstr(v, i1, i2)
struct value v;
int *i1, *i2;
{
	int min, i, j;
	struct point pt;
	struct points *p;
	struct proc *bp;

	if (XFIELD(v))
		return (VOID);
	bp = (struct proc *) getblk(v.v_addr);
	if (bp->p_code == 0 && recompile(bp)) {
		putblk(bp, 0);
		return (VOID);
		}
	*i1 = getidx(*i1, min = XFIELD(bp->p_source));
	*i2 = getidx(*i2, min);
	pt.pc = 0;
	for (i = 0; i < sizeof(bp->p_points)/sizeof(int); i++) {
		if (bp->p_points[i] == 0)
			continue;
		p = (struct points *) getblk(bp->p_points[i]);
		for (j = 0; j < sizeof(p->n_points)/sizeof(struct point); j++)
			if (p->n_points[j].pc && p->n_points[j].pos[0] <= *i1 &&
				p->n_points[j].pos[1] >= *i2 &&
				*i1 - p->n_points[j].pos[0] +
				p->n_points[j].pos[1] - *i2 < min) {
					pt = p->n_points[j];
					min = *i1 - pt.pos[0] + pt.pos[1] - *i2;
					}
		putblk(p, 0);
		}
	if (pt.pc)
		v = substr(bp->p_source, *i1 = pt.pos[0], *i2 = pt.pos[1]);
	else
		v = VOID;
	putblk(bp, 0);
	return (v);
}

/* ptcall - print trace of call to procedure f of n arguments ap[0..n-1] */
ptcall(f, n, ap)
struct value f, *ap;
int n;
{
	int i;

	if (trace) {
		for (i = 1; i <= level; i++)
			putc('.', stderr);
		prproc(f, n, ap);
		fprintf(stderr, " called\n");
		trace--;
		}
}

/* ptlocal - print local in a frame */
static int ptlocal(ip, vp, fp, f)
struct value *ip, *vp;
struct frame *fp;
FILE *f;
{
	if (TYPE(*vp) == T_VAR && vp->v_addr > 0) {
		fprintf(f, "	 ");
		excvstr(*ip, f);
		fprintf(f, " = ");
		image(deref(fp->f_vals[fp->f_lp+vp->v_addr]), f, 0);
		fprintf(f, "\n");
		}
	return (0);
}

/* ptret - print trace of procedure f returning v */
ptret(f, v)
struct value f, v;
{
	int i;
	struct proc *p;

	if (trace) {
		for (i = 1; i <= level; i++)
			putc('.', stderr);
		p = (struct proc *) getblk(f.v_addr);
		if (TYPE(p->p_name) == T_STRING)
			excvstr(p->p_name, stderr);
		else
			fprintf(stderr, "?");
		putblk(p, 0);
		fprintf(stderr, " returned ");
		image(deref(v), stderr, 0);
		fprintf(stderr, "\n");
		trace--;
		}
}

/* strace - print stack trace for n levels beginning with frame pointers
   fp; prints locals if lflag is non-zero */
strace(n, fp, lflag)
int n, lflag;
struct frame *fp;
{
	struct proc *p;

	for ( ; n && fp; n--) {
		prproc(fp->f_vals[0], fp->f_lp, &fp->f_vals[1]);
		putc('\n', stderr);
		if (lflag) {
			p = (struct proc *) getblk(fp->f_vals[0].v_addr);
			tseq(p->p_sym, ptlocal, fp, stderr);
			putblk(p, 0);
			}
		fp = fp->f_prev;
		}
}
