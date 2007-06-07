/* ez: interpreter */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ez.h"
#include <setjmp.h>
#define mkvar(p,flag) (v.v_type = T_VAR, v.v_x = flag, v.v_addr = (int) (p), v)
#define xsp fp->f_sp
#define top fp->f_vals[xsp]
#define xpush(v) if (++xsp < sizeof(fp->f_vals)/sizeof(struct value))\
	fp->f_vals[xsp] = v; else err("stack overflow")
#define xpop(x) fp->f_vals[xsp--]

int level;			/* call level */
int trace;			/* trace counter */
int *cp = NULL;			/* code pointer */
struct frame *fp = NULL;	/* current frame */
struct frame *freeframes = NULL;/* list of free frames */
struct value VOID;		/* void value */
static struct value returned;	/* returned value from interp */ 
extern jmp_buf restart;

static char *errmsgs[] = {
	"interp: bad error number",
	"ran off end of a procedure"
};

static call();
static int checkpc();
static int interp();
static struct value subasgn();
static int tloop();

/* apply - execute function f */
struct value apply(f)
struct value f;
{
	level = 0;
	cp = NULL;
	fp = NULL;
	returned = VOID;
	if (TYPE(f) == T_PROC) {
		call(f, 0, NULL);
		interp(0);
		}
	return (returned);
}

/* asgn - assign av to variable v if av is not void, return av */
struct value asgn(v, av)
struct value v, av;
{
	struct value v1, v2, *vp;

	if (TYPE(av) == T_VOID)
		return (av);
	for (v2 = v; TYPE(v) == T_VAR; v = v1)
		if (XFIELD(v)) {
			vp = (struct value *) getblk(v.v_addr);
			v1 = *vp;
			if (TYPE(v1) != T_VAR) {
				*vp = av;
				putblk(vp, 1);
				return (av);
				}
			putblk(vp, 0);
			}
		else {
			v1 = *(struct value *) v.v_addr;
			if (TYPE(v1) != T_VAR) {
				*(struct value *) v.v_addr = av;
				return (av);
				}
			}
	err("variable expected", v2);
}

/* call - call procedure or table f with n arguments at ap[0..n-1] */
static call(f, n, ap)
struct value f;
int n;
struct value *ap;
{
	struct value v, t, *vp, rpt;
	struct frame *newfp;
	struct proc *p;
	struct point *pt;
	int i;

	switch (TYPE(f)) {
		case T_TABLE:
			t = f;
			newfp = (struct frame *) getblk(t.v_addr);
			if (newfp->f_type != B_FRAME)
				err("call: invalid frame");
			if ((vp = tindex(t, Procedure, 0)) == NULL)
				err("Procedure entry missing");
			f = *vp;
			if (TYPE(f) != T_PROC) {
				v = excvproc(f);
				if (TYPE(v) != T_PROC)
					err("procedure expected", f);
				*vp = f = v;
				touch(vp);
				}
			putblk(vp, 0);
			p = (struct proc *) getblk(f.v_addr);
			if (p->p_code == 0)
				if (recompile(p))
					err("procedure expected", f);
			if (n > p->p_nargs)
				n = p->p_nargs;
			for (i = 0; i < n; i++)
				asgn(newfp->f_vals[i+1], ap[i]);
			newfp->f_sp = newfp->f_spbase;
			if ((vp = tindex(t, Resumption, 0)) == NULL)
				err("Resumption entry missing");
			rpt = *vp;
			putblk(vp, 0);
			rpt = cvi(rpt);
			if ((pt = pindex(p, rpt.v_addr, NULL)) && pt->pc)
				newfp->f_pc = pt->pc;
			else {
				if (pt)
					putblk(pt, 0);
				err("invalid resumption point");
				}
			putblk(pt, 0);
			break;
		default:	
			f = excvproc(f);
			if (TYPE(f) != T_PROC)
				err("procedure expected", f);
		case T_PROC:
			p = (struct proc *) getblk(f.v_addr);
			if (i = XFIELD(f)) {
				if (trace)
					ptcall(f, n, ap);
				xpush((*bivalues[i].bi_addr)(n, ap));
				if (trace)
					ptret(f, fp->f_vals[xsp]);
				putblk(p, 0);
				return;
				}
			if (p->p_code == 0)
				if (recompile(p))
					err("procedure expected", f);
			newfp = falloc();
			newfp->f_sp = p->p_nargs + p->p_nlocals;
			newfp->f_spbase = newfp->f_sp;
			newfp->f_lp = p->p_nargs;
			newfp->f_vals[0] = f;
			if (n > p->p_nargs)
				n = p->p_nargs;
			for (i = 0; i < n; i++)
				newfp->f_vals[i+1] = ap[i];
			for (i = n; i < p->p_nargs; i++)
				newfp->f_vals[i+1] = VOID;
			for (i = p->p_nlocals; i > 0; i--)
				newfp->f_vals[newfp->f_lp+i] = VOID;
			newfp->f_pc = p->p_code;
			break;
		}
	if (fp)
		fp->f_pc = cp ? virtaddr(cp) : 0;
	if (cp)
		putblk(cp, 0);
	newfp->f_prev = fp;
	fp = newfp;
	cp = (int *) getblk(fp->f_pc);
	if (trace)
		ptcall(f, n, ap);
	level++;
	putblk(p, 0);
}

/* checkpc - get current resumption point for frame f */
static int checkpc(f)
struct frame *f;
{
	struct points *p;
	struct proc *bp;
	int i, j, k, n, min;

	bp = (struct proc *) getblk(f->f_vals[0].v_addr);
	min = 10000;
	k = 1;
	for (n = i = 0; i < sizeof(bp->p_points)/sizeof(int); i++) {
		if (bp->p_points[i] == 0)
			continue;
		p = (struct points *) getblk(bp->p_points[i]);
		for (j = 0; j < sizeof(p->n_points)/sizeof(struct point); j++, k++)
			if (BLKTOP(p->n_points[j].pc) == BLKTOP(f->f_pc) &&
				f->f_pc >= p->n_points[j].pc &&
				f->f_pc - p->n_points[j].pc <= min) {
					min = f->f_pc - p->n_points[j].pc;
					n = k;
					}
		putblk(p, 0);
		}
	putblk(bp, 0);
	return (n);
}

/* err - issue runtime error message s, offending value v */
err(s, v)
char *s;
struct value v;
{
	if (strchr(s, ':')) {
		fprintf(stderr, "system error in %s\n", s);
		savecache();
		exit(1);
		}
	fprintf(stderr, "%s\n", s);
	if (s+strlen(s)-8 > s && strcmp(s+strlen(s)-8, "expected") == 0) {
		fprintf(stderr, "offending value: ");
		image(v, stderr, 1);
		fprintf(stderr, "\n");
		}
	strace(-1, fp, 0);
	longjmp(restart, 1);
}

/* falloc - allocate a frame block */
struct frame *falloc()
{
	struct frame *p;

	if (p = freeframes)
		freeframes = p->f_prev;
	else if ((p = (struct frame *) malloc(sizeof(struct frame))) == NULL)
		err("falloc: stack overflow");
	p->f_type = B_BLOCK;
	return (p);
}

/* ffree - free frame block p */
ffree(p)
struct frame *p;
{
	if (p->f_type == B_BLOCK) {
		p->f_prev = freeframes;
		freeframes = p;
		}
}

/* interp - begin interpreting code at virtual address pc */
static int interp(pc)
int pc;
{
	struct value v, av, bv, *vp, subasgn();
	char *s1, *s2;
	struct proc *p;
	struct block *b;
	struct point *pt;
	struct frame *oldfp;
	int *cp1, i, op, cond, tloop();
	float r;

	if (pc)
		cp = (int *) getblk(pc);
	while (cp)
		switch (op = *cp++) {
		case O_ADD:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
				xpush(mkint(av.v_addr + bv.v_addr));
			else
				xpush(mkreal(getr(cvr(av)) + getr(cvr(bv))));
			break;
		case O_SUB:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
				xpush(mkint(av.v_addr - bv.v_addr));
			else
				xpush(mkreal(getr(cvr(av)) - getr(cvr(bv))));
			break;
		case O_MUL:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_INT && TYPE(bv) == T_INT)
				xpush(mkint(av.v_addr * bv.v_addr));
			else
				xpush(mkreal(getr(cvr(av)) * getr(cvr(bv))));
			break;
		case O_DIV:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_INT && TYPE(bv) == T_INT) {
				if (bv.v_addr == 0)
					err("division by zero");
				xpush(mkint(av.v_addr / bv.v_addr));
				}
			else {
				if ((r=getr(cvr(bv))) == 0.0)
					err("division by zero");
				xpush(mkreal(getr(cvr(av)) / r));
				}
			break;
		case O_MOD:
			bv = xpop();
			av = xpop();
			if (bv.v_addr == 0)
				err("residue by zero");
			xpush(mkint(av.v_addr % bv.v_addr));
			break;
		case O_NEG:
			if (TYPE(top) == T_INT)
				top = mkint(-top.v_addr);
			else
				top = mkreal(-getr(top));
			break;
		case O_IDX:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_PROC)
				xpush(pidx(av, bv));
			else if (vp = tindex(av, bv, 0)) {
				xpush(*vp);
				putblk(vp, 0);
				}
			else
				xpush(VOID);
			break;
		case O_GVAR:
			xpush(mkvar(*cp++, VIRTUAL));
			break;
		case O_GVAL:
			vp = (struct value *) getblk(*cp++);
			xpush(*vp);
			putblk(vp, 0);
			break;
		case O_LVAR:
			if (fp->f_type == B_FRAME)
				xpush(fp->f_vals[fp->f_lp+(*cp++)]);
			else
				xpush(mkvar(&fp->f_vals[fp->f_lp+(*cp++)], PHYSICAL));
			break;
		case O_LVAL:
			if (fp->f_type == B_FRAME)
				xpush(deref(fp->f_vals[fp->f_lp+(*cp++)]));
			else
				xpush(fp->f_vals[fp->f_lp+(*cp++)]);
			break;
		case O_AVAR:
			if (fp->f_type == B_FRAME)
				xpush(fp->f_vals[*cp++]);
			else
				xpush(mkvar(&fp->f_vals[*cp++], PHYSICAL));
			break;
		case O_AVAL:
			if (fp->f_type == B_FRAME)
				xpush(deref(fp->f_vals[*cp++]));
			else
				xpush(fp->f_vals[*cp++]);
			break;
		case O_JUMP:
			cp1 = (int *) *cp;
			putblk(cp, 0);
			cp = (int *) getblk(cp1);
			break;
		case O_LINK:
			cp1 = (int *) ((struct code *) BLKTOP(--cp))->c_next;
			putblk(cp, 0);
			cp = ((struct code *) getblk(cp1))->c_code;
			break;
		case O_NOT:
			if (TYPE(top) == T_VOID)
				top = stralloc("", 0);
			else
				top = VOID;
			break;
		case O_LT: case O_LE: case O_EQ:
		case O_NE: case O_GE: case O_GT:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID) {
				xpush(VOID);
				break;
				}
			if (TYPE(av) == T_INT || TYPE(av) == T_REAL)
				bv = cvnum(bv);
			else if (TYPE(bv) == T_INT || TYPE(bv) == T_REAL)
				av = cvnum(av);
			cond = compare(av, bv);
			switch (op) {
				case O_LT: if (cond >= 0) bv = VOID; break;
				case O_LE: if (cond >  0) bv = VOID; break;
				case O_EQ: if (cond != 0) bv = VOID; break;
				case O_NE: if (cond == 0) bv = VOID; break;
				case O_GE: if (cond <  0) bv = VOID; break;
				case O_GT: if (cond <= 0) bv = VOID; break;
				}
			xpush(bv);
			break;
		case O_BEV:
			if (TYPE(top) == T_VOID) {
				cp1 = (int *) *cp;
				putblk(cp, 0);
				cp = (int *) getblk(cp1);
				}
			else {
				xsp--;
				cp++;
				}
			break;
		case O_JEV:
			v = xpop();
			if (TYPE(v) == T_VOID) {
				cp1 = (int *) *cp;
				putblk(cp, 0);
				cp = (int *) getblk(cp1);
				}
			else
				cp++;
			break;
		case O_BNV:
			if (TYPE(top) != T_VOID) {
				cp1 = (int *) *cp;
				putblk(cp, 0);
				cp = (int *) getblk(cp1);
				}
			else {
				xsp--;
				cp++;
				}
			break;
		case O_JNV:
			v = xpop();
			if (TYPE(v) != T_VOID) {
				cp1 = (int *) *cp;
				putblk(cp, 0);
				cp = (int *) getblk(cp1);
				}
			else
				cp++;
			break;
		case O_ITER:
			pc = virtaddr(cp+1);
			cp1 = (int *) *cp;
			putblk(cp, 0);
			v = xpop();
			av = xpop();
			if (TYPE(av) == T_PROC)
				i = pseq(av, tloop, &v, pc);
			else
				i = tseq(av, tloop, &v, pc);
			if (i <= 1) {
				cp = (int *) getblk(cp1);
				break;
				}
			if (i > 2)	/* unwind nested for (x in e) loops */
				return (--i);
			cp = 0;
		case O_RET:
			v = xpop();
			level--;
			if (trace)
				ptret(fp->f_vals[0], v);
			oldfp = fp;
			fp = oldfp->f_prev;
			if (oldfp->f_type == B_FRAME) {
				oldfp->f_pc = cp ? virtaddr(cp) : 0;
				vp = tindex(mkval(T_TABLE, 0, virtaddr(oldfp)),
					Resumption, 1);
				*vp = mkint(checkpc(oldfp));
				putblk(vp, 1);
				if (fp->f_type == B_BLOCK)
					oldfp->f_prev = NULL;
				putblk(oldfp, 1);
				}
			else
				ffree(oldfp);
			if (cp)
				putblk(cp, 0);
			if (fp == NULL) {
				returned = v;
				return (0);
				}
			xpush(v);
			cp = (int *) getblk(fp->f_pc);
			break;
		case O_IDXL:
			bv = xpop();
			v = xpop();
			av = deref(v);
			switch (TYPE(av)) {
				case T_PROC:
					if (XFIELD(av)) {
						xpush(VOID);
						break;
						}
					bv = cvi(bv);
					p = (struct proc *) getblk(av.v_addr);
					if (p->p_code == 0 && recompile(p)) {
						putblk(p, 0);
						xpush(VOID);
						break;
						}
					pt = pindex(p, bv.v_addr, NULL);
					if (pt && pt->pc) {
						xpush(v);
						xpush(mkint(pt->pos[0]));
						xpush(mkint(pt->pos[1]));
						xpush(mkvar(0, SUBPROC));
						}
					else
						xpush(VOID);
					if (pt)
						putblk(pt, 0);
					putblk(p, 0);
					break;
				default:
					if (TYPE(bv) == T_INT)
						b = balloc(B_ARRAY);
					else
						b = balloc(B_TABLE);
					av = mkval(T_TABLE, 0, virtaddr(b));
					putblk(b, 0);
					asgn(v, av);	/* fall thru */
				case T_TABLE:
					vp = tindex(av, bv, 1);
					xpush(mkvar(virtaddr(vp), VIRTUAL));
					putblk(vp, 0);
				}
			break;
		case O_BASE:
			xsp = fp->f_spbase;
			break;
		case O_DEREF:
			while (TYPE(top) == T_VAR)
				if (XFIELD(top)) {
					vp = (struct value *) getblk(top.v_addr);
					top = *vp;
					putblk(vp, 0);
					}
				else
					top = *(struct value *) top.v_addr;
			break;
		case O_ASGN:
			av = xpop();
			if (TYPE(top) != T_VAR) {
				top = asgn(top, av);
				break;
				}
			switch (XFIELD(top)) {
				case PHYSICAL: case VIRTUAL:
					top = asgn(top, av);
					break;
				case SUBSTRING: case SUBPROC:
					xpop();
					top = subasgn(av);
					break;
				default:
					err("interp: illegal variable");
				}
			break;
		case O_CVI:
			if (TYPE(top) == T_INT)
				break;
			top = excvi(v = top);
			if (TYPE(top) == T_VOID)
				err("integer expected", v);
			break;
		case O_CVR:
			if (TYPE(top) == T_REAL)
				break;
			top = excvr(v = top);
			if (TYPE(top) == T_VOID)
				err("real expected", v);
			break;
		case O_CVNUM:
			if (TYPE(top) == T_INT || TYPE(top) == T_REAL)
				break;
			top = excvnum(v = top);
			if (TYPE(top) == T_VOID)
				err("numeric expected", v);
			break;
		case O_CVSTR:
			if (TYPE(top) == T_STRING)
				break;
			top = excvstr(v = top, NULL);
			if (TYPE(top) == T_VOID)
				err("string expected", v);
			break;
		case O_DUP:
			v = top;
			xpush(v);
			break;
		case O_QUIT:
			cp1 = (int *) *cp;
			putblk(cp, 0);
			return ((int) cp1);
		case O_POP:
			xpop();
			break;
		case O_SSTR:
			bv = xpop();
			av = xpop();
			if (TYPE(top) == T_TABLE) {
				top = tsub(top, av, bv);
				break;
				}
			av = cvi(av);
			bv = cvi(bv);
			if (TYPE(top) == T_PROC)
				top = psubstr(top, &av.v_addr, &bv.v_addr);
			else
				top = substr(cvstr(top), av.v_addr, bv.v_addr);
			break;
		case O_SSTRL:
			v = deref(fp->f_vals[xsp-2]);
			if (TYPE(v) == T_PROC) {
				fp->f_vals[xsp-1] = cvi(fp->f_vals[xsp-1]);
				top = cvi(top);
				v = psubstr(v, &fp->f_vals[xsp-1].v_addr,
					&top.v_addr);
				if (TYPE(v) == T_VOID) {
					xsp -= 2;
					top = VOID;
					}
				else
					xpush(mkvar(0, SUBPROC));
				}
			else
				xpush(mkvar(0, SUBSTRING));
			break;
		case O_CAT:
			bv = xpop();
			av = xpop();
			if (TYPE(av) == T_TABLE && TYPE(bv) == T_TABLE) {
				xpush(tcat(av, bv));
				break;
				}
			av = cvstr(av);
			bv = cvstr(bv);
			if (XFIELD(av) == 0)
				xpush(bv);
			else if (XFIELD(bv) == 0)
				xpush(av);
			else {
				xpush(mkstr(salloc(), XFIELD(av)+XFIELD(bv)));
				s1 = SOPENW(top.v_addr);
				s2 = SOPENR(av.v_addr);
				for (i = XFIELD(av); i > 0; i--)
					SPUTC(s1, SGETC(s2));
				SCLOSER(s2);
				s2 = SOPENR(bv.v_addr);
				for (i = XFIELD(bv); i > 0; i--)
					SPUTC(s1, SGETC(s2));
				SCLOSER(s2);
				SCLOSEW(s1);
				}
			break;
		case O_CALL:
			i = (int) *cp++;
			xsp -= i+1;
			call(fp->f_vals[xsp+1], i, &fp->f_vals[xsp+2]);
			break;
		case O_NOOP: break;
		case O_ERR:
			i = (int) *cp++;
			putblk(cp, 0);
			err(errmsgs[i], VOID);
		default:
			err("interp: illegal operator");
		}
}

/* subasgn - substring assignment */
static struct value subasgn(v)
struct value v;
{
	struct value var, s, av, bv, v1;
	struct proc *p;
	char *s1, *s2;
	int i1, i2, i;

	bv = xpop();
	av = xpop();
	var = xpop();
	s = deref(var);
	if (TYPE(s) == T_TABLE)
		return (asgn(var, tasgn(s, av, bv, v)));
	p = NULL;
	if (TYPE(s) == T_PROC) {	/* modify source code */
		if (XFIELD(s))
			return (VOID);
		p = (struct proc *) getblk(s.v_addr);
		s = p->p_source;
		}
	s = cvstr(s);
	v = cvstr(v);
	av = cvi(av);
	bv = cvi(bv);
	i1 = getidx(av.v_addr, XFIELD(s));
	i2 = getidx(bv.v_addr, XFIELD(s));
	if (i1 < 0 || i2 < 0)
		return (VOID);
	if (i2 < i1) {
		i = i1; i1 = i2; i2 = i;
		}
	av = substr(s, 1, i1);
	bv = substr(s, i2, 0);
	if (TYPE(av) == T_VOID || TYPE(bv) == T_VOID)
		return (VOID);
	v1 = mkstr(salloc(), XFIELD(av) + XFIELD(v) + XFIELD(bv));
	s1 = SOPENW(v1.v_addr);
	s2 = SOPENR(av.v_addr);
	for (i = XFIELD(av); i > 0; i--)
		SPUTC(s1, SGETC(s2));
	SCLOSER(s2);
	s2 = SOPENR(v.v_addr);
	for (i = XFIELD(v); i > 0; i--)
		SPUTC(s1, SGETC(s2));
	SCLOSER(s2);
	s2 = SOPENR(bv.v_addr);
	for (i = XFIELD(bv); i > 0; i--)
		SPUTC(s1, SGETC(s2));
	SCLOSER(s2);
	SCLOSEW(s1);
	if (p) {	/* set new source code, zap compiled code */
		p->p_source = v1;
		p->p_code = 0;
		putblk(p, 1);
		return (v1);
		}
	return (asgn(var, v1));
}

/* tloop - called at each iteration of a for (id in e) loop */
static int tloop(iv, vp, var, pc)
struct value *iv, *vp, *var;
int pc;
{
	asgn(*var, *iv);	/* assign index value to the loop variable */
	return (interp(pc));	/* execute the loop body */
}

#ifndef xpush
/* xpush - push v onto the current stack */
static xpush(v)
struct value v;
{
	if (++xsp < sizeof(fp->f_vals)/sizeof(struct value))
		fp->f_vals[xsp] = v;
	else
		err("stack overflow");
}
#endif

#ifndef xpop
/* xpop - pop and return top value from the stack */
static struct value xpop()
{
	if (xsp >= 0)
		return(fp->f_vals[xsp--]);
	err("xpop: stack underflow");
}
#endif
