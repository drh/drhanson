/* ez: tables */

#include <stdio.h>
#include "ez.h"
#define	LOWER 0			/* lower address for array layout */
#define	UPPER ((int) ((CHUNKSIZE-2)*(CHUNKSIZE-1)*((CHUNKSIZE-1)/2) - 1))
				/* upper address for array layout */
#define	MAPFACTOR1 (CHUNKSIZE-1)/* B_ARRAY page 1 factor */
#define	MAPFACTOR2 ((int) (CHUNKSIZE-1)/2)	/* B_ARRAY page 2 factor */

/* hash - return hash code for arbitrary value v */
static unsigned hash(v)
struct value v;
{
	char *s;
	register unsigned h;
	register int i;

	if (TYPE(v) == T_STRING) {
		s = SOPENR(v.v_addr);
		h = 0;
		for (i = XFIELD(v)>5?5:XFIELD(v); i > 0; i--)
			h = (h<<5) + SGETC(s);
		SCLOSER(s);
		}
	else
		h = (unsigned) v.v_addr;
	return(h);
}

/* tadd - called to add an element in table concatentation */
static struct value index, newt;
static int tadd(iv, v, avp, bvp)
struct value *iv, *v, *avp, *bvp;
{
	struct value *vp;

	if (avp) {	/* adding element from the first table */
		vp = tindex(*avp, *iv, 1);
		if (TYPE(*iv) == T_INT && (TYPE(index) == T_VOID ||
			iv->v_addr > index.v_addr))
				index = *iv;
		}
	else {		/* adding element from the second table */
		if (TYPE(*iv) == T_INT && TYPE(index) == T_INT) {
			index.v_addr++;
			vp = tindex(*bvp, index, 1);
			}
		else
			vp = tindex(*bvp, *iv, 1);
		}
	*vp = *v;
	putblk(vp, 1);
	return (0);
}

/* tadd1 - called to add an element in subtable assignment */
static int tadd1(iv, v, avp, bvp)
struct value *iv, *v, *avp, *bvp;
{
	struct value *vp;

	vp = NULL;
	if (avp) {	/* add elements < *avp from the first table */
		if (compare(*iv, *avp) < 0) {
			vp = tindex(newt, *iv, 1);
			if (TYPE(*iv) == T_INT && (TYPE(index) == T_VOID ||
				iv->v_addr > index.v_addr))
					index = *iv;
			}
		}
	else {		/* add elements > *bvp from the third table */
		if (compare(*iv, *bvp) > 0)
			if (TYPE(*iv) == T_INT && TYPE(index) == T_INT) {
				index.v_addr++;
				vp = tindex(newt, index, 1);
				}
			else
				vp = tindex(newt, *iv, 1);
		}
	if (vp) {
		*vp = *v;
		putblk(vp, 1);
		}
	return (0);
}

/* tasgn - return the table for the subtable assignment t[av:bv] = v */
struct value tasgn(t, av, bv, v)
struct value t, av, bv, v;
{
	struct table *tp;
	struct block *b;
	struct value *vp;

	b = balloc(B_TABLE);
	newt = mkval(T_TABLE, 0, virtaddr(b));
	putblk(b, 0);
	index = VOID;
	tseq(t, tadd1, &av, 0);
	if (TYPE(v) == T_TABLE)
		if (TYPE(index) == T_VOID)
			tseq(v, tadd, &newt, 0);
		else
			tseq(v, tadd, 0, &newt);
	else {		/* add v as if it were in a table with index 1 */
		if (TYPE(index) == T_VOID) 
			index = mkint(1);
		else
			index.v_addr++;
		vp = tindex(newt, index, 1);
		*vp = v;
		putblk(vp, 1);
		}
	tseq(t, tadd1, 0, &bv);
	return (newt);
}

/* tcat - concatenate tables av and bv */
struct value tcat(av, bv)
struct value av, bv;
{
	struct value v;
	struct table *atp, *btp;
	struct block *b;

	atp = (struct table *) getblk(av.v_addr);
	btp = (struct table *) getblk(bv.v_addr);
	if (atp->t_type == B_ARRAY && btp->t_type == B_ARRAY)
		b = balloc(B_ARRAY);
	else
		b = balloc(B_TABLE);
	putblk(atp, 0);
	putblk(btp, 0);
	v = mkval(T_TABLE, 0, virtaddr(b));
	putblk(b, 0);
	index = VOID;
	tseq(av, tadd, &v, 0);
	tseq(bv, tadd, 0, &v);
	return (v);
}

/* tcopy - called to copy a table element */
static int tcopy(iv, v, tv, x)
struct value *iv, *v, *tv, *x;
{
	struct value *vp;

	vp = tindex(*tv, *iv, 1);
	*vp = *v;
	putblk(vp, 1);
	return (0);
}

/* tcvt - convert table t from array layout to hashed layout */
struct value tcvt(t)
struct value t;
{
	struct array *ap, *b;
	struct value v;
	int i;

	ap = (struct array *) getblk(t.v_addr);
	if (ap->a_type == B_ARRAY) {
		b = (struct array *) balloc(B_ARRAY);
		b->a_size = ap->a_size;
		for (i = 0; i < sizeof(b->a_indir)/sizeof(int); i++) {
			b->a_indir[i] = ap->a_indir[i];
			ap->a_indir[i] = 0;
			}
		ap->a_type = B_TABLE;
		ap->a_size = 0;
		touch(ap);
		v = mkval(T_TABLE, 0, virtaddr(b));
		putblk(b, 0);
		tseq(v, tcopy, &t, 0);
		}
	putblk(ap, 0);
	return (t);
}

/* tindex - subscript a table, t[v].  Returns a pointer
 * (i.e. physical address) to the value part of an <index,value> pair or NULL
 * if v was not found; after it is used, it should passed to putblk.
 * if lval is non-zero, an <index,value> for v is created, the index is
 * initialized to v, the value is initialized to VOID, and
 * a pointer to the value part of the new pair is returned. */
struct value *tindex(t, v, lval)
struct value t, v;
int lval;
{
	extern int tcopy();
	int h, q, va, p1, p2, offset;
	struct table *tp;
	struct array *ap, *tap;
	struct elem *ep;
	struct value *vp, td;
	struct block *b;
	struct indir *ip;
	struct data *dp;

top:	if (TYPE(t) != T_TABLE)
		return (NULL);
	tp = (struct table *) getblk(t.v_addr);
	if (tp->t_type == B_FRAME) {struct frame *fp = (struct frame *) tp;
		t = fp->f_table;
		putblk(fp, 0);
		goto top;
		}
	if (tp->t_type == B_TABLE) {
		h = hash(v)%(sizeof(tp->t_elem)/sizeof(int));
		va = 0;
		for (q = tp->t_elem[h]; q; ) {
			ep = (struct elem *) getblk(q);
			for (vp = ep->e_vals; vp+2 <= (struct value *) (ep+1); vp += 2)
				if (TYPE(*vp) == T_VOID) { /* see tremove */
					if (!va)
						va = virtaddr(vp);
					}
				else if (compare(v, *vp) == 0) {
					putblk(tp, 0);
					return (++vp);
					}
			q = ep->e_next;
			putblk(ep, 0);
			}
		if (lval) {
			if (va)
				vp = (struct value *) getblk(va);
			else {
				ep = (struct elem *) balloc(B_TELEM);
				ep->e_next = tp->t_elem[h];
				tp->t_elem[h] = virtaddr(ep);
				vp = ep->e_vals;
				}
			*vp++ = v;
			*vp = VOID;
			touch(vp);
			tp->t_size++;
			touch(tp);
			}
		else
			vp = NULL;
		putblk(tp, 0);
		return (vp);
		}
	/* table is type B_ARRAY */
	ap = (struct array *) tp;
	if ((TYPE(v) != T_INT) || v.v_addr < LOWER  || v.v_addr > UPPER) {
		putblk(ap, 0);
		return (tindex(tcvt(t), v, lval));
		}
	offset = v.v_addr % MAPFACTOR2;
	q = v.v_addr / MAPFACTOR2;
	p2 = q % MAPFACTOR1;
	p1 = q / MAPFACTOR1;
	if (lval) {
		if (q = ap->a_indir[p1])
			ip = (struct indir *) getblk(q);
		else {
			ip = (struct indir *) balloc(B_INDIR);
			q = ap->a_indir[p1] = virtaddr(ip);
			touch(ap);
			}
		if (q = ip->i_data[p2])
			dp = (struct data *) getblk(q);
		else {
			dp = (struct data *) balloc(B_DATA);
			q = ip->i_data[p2] = virtaddr(dp);
			touch(ip);
			}
		putblk(ip, 0);
		vp = dp->d_vals + offset;
		if (TYPE(*vp) == T_VOID)
			ap->a_size++;
		touch(ap);
		}
	else {
		vp = NULL;
		if (q = ap->a_indir[p1]) {
			ip = (struct indir *) getblk(q);
			if (q = ip->i_data[p2]) {
				dp = (struct data *) getblk(q);
				vp = dp->d_vals + offset;
				if (TYPE(*vp) == T_VOID) {
					putblk(dp, 0);
					vp = NULL;
					}
				}
			putblk(ip, 0);
			}
		}
	putblk(ap, 0);
	return (vp);
}

/* tremove - remove t[v] */
tremove(t, v)
struct value t, v;
{
	 struct value *vp;
	 struct table *tp;

top:	if (TYPE(t) != T_TABLE)
		return;
	tp = (struct table *) getblk(t.v_addr);
	if (tp->t_type == B_FRAME) {struct frame *fp = (struct frame *) tp;
		t = fp->f_table;
		putblk(fp, 0);
		goto top;
		}
	if (vp = tindex(t, v, 0)) {
		tp->t_size--;
		if (tp->t_type == B_ARRAY)
			*vp = VOID;
		else if (TYPE(*--vp) == T_STRING)
			*vp = mkval(T_VAR, VIRTUAL, 0);	/* can't reuse */
						/* possible identifiers */
		else
			*vp = VOID;
		putblk(vp, 1);
		putblk(tp, 1);
		}
}

/* tseq - sequence through a table calling
 * f(struct value *index, struct value *value, a, b) for each
 * entry (index,value pair).  If f returns a non-zero value,
 * the loop is broken and tseq returns that value, otherwise tseq returns 0. */
int tseq(t, f, a, b)
struct value t;
int (*f)(), *a, *b;
{
	int q, i, j, n, n1, n2, x;
	struct value v;
	struct table *tp;
	struct elem *ep;
	struct array *ap;
	struct indir *ip;
	struct data *dp;
	register struct value *vp;

top:	if (TYPE(t) != T_TABLE)
		return (0);
	tp = (struct table *) getblk(t.v_addr);
	if (tp->t_type == B_FRAME) {struct frame *fp = (struct frame *) tp;
		t = fp->f_table;
		putblk(fp, 0);
		goto top;
		}
	if (tp->t_type == B_TABLE) {
		for (i = 0; i < sizeof(tp->t_elem)/sizeof(int); i++)
			for (q = tp->t_elem[i]; q; ) {
				ep = (struct elem *) getblk(q);
				for (vp = ep->e_vals; vp+2 <= (struct value *) (ep+1); vp += 2)
					if (TYPE(*vp) != T_VOID &&
						TYPE(*vp) != T_VAR &&
						(x = (*f)(vp, vp+1, a, b))) {
							putblk(ep, 0);
							putblk(tp, 0);
							return (x);
							}
				q = ep->e_next;
				putblk(ep, 0);
				}
		putblk(tp, 0);
		return (0);
		}
	/* type is B_ARRAY */
	ap = (struct array *) tp;
	v = mkint(0);
	n = -(MAPFACTOR1 * MAPFACTOR2);
	for (i = 0; i < sizeof(ap->a_indir)/sizeof(int); i++) {
		n += (MAPFACTOR1 * MAPFACTOR2);
		if ((q = ap->a_indir[i]) == NULL)
			continue;
		ip = (struct indir *) getblk(q);
		n1 = -MAPFACTOR2;
		for (j = 0; j < sizeof(ip->i_data)/sizeof(int); j++) {
			n1 += MAPFACTOR2;
			if ((q = ip->i_data[j]) != NULL) {
				dp = (struct data *) getblk(q);
				n2 = -1;
				for (vp = dp->d_vals; vp < (struct value *) (dp+1); vp += 1) {
					v.v_addr = n + n1 + ++n2;
					if (TYPE((*vp)) != T_VOID &&
						(x = (*f)(&v, vp, a, b))) {
							putblk(dp, 0);
							putblk(ip, 0);
							putblk(ap, 0);
							return (x);
							}
					}
				putblk(dp, 0);
				}
			}
		putblk(ip, 0);
		}
	putblk(ap, 0);
	return (0);
}

/* tsstr - add an entry to subtable if *ip >= *av && *ip <= bv */
static int tsstr(ip, v, av, bv)
struct value *ip, *v, *av, *bv;
{
	struct value *vp;

	if (compare(*ip, *av) >= 0 && compare(*ip, *bv) <= 0) {
		vp = tindex(newt, *ip, 1);
		*vp = *v;
		putblk(vp, 1);
		}
	return (0);
}

/* tsub - compute a subtable of t from av to bv */
struct value tsub(t, av, bv)
struct value t, av, bv;
{
	struct table *tp;
	struct block *b;
	struct value v;

	tp = (struct table *) getblk(t.v_addr);
	if (tp->t_type == B_ARRAY && TYPE(av) == T_INT && TYPE(bv) == T_INT)
		b = balloc(B_ARRAY);
	else
		b = balloc(B_TABLE);
	newt = mkval(T_TABLE, 0, virtaddr(b));
	putblk(b, 0);
	putblk(tp, 0);
	if (compare(bv, av) < 0) {
		v = av; av = bv; bv = v;
		}
	tseq(t, tsstr, &av, &bv);
	return (newt);
}
