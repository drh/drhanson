/* ez: utility functions */

#include <stdio.h>
#include "ez.h"

int strp;		/* virtual address of string constants */

/* ldump - print local variable or argument typ */
static int ldump(typ, vp, a, b)
struct value *typ, *vp;
int a, b;
{
	if (TYPE(*vp) == T_VAR) {
		fprintf(stderr, "	");
		excvstr(*typ, stderr);
		if (vp->v_addr < 0)
			fprintf(stderr, " argument %d\n", -vp->v_addr);
		else
			fprintf(stderr, " local %d\n", vp->v_addr);
		}
	return (0);
}

/* bdump - print the block containing virtual address a on f or stderr */
bdump(a, f)
int a;
FILE *f;
{
	extern char *opname[];
	int i, *q, *q1, ldump();
	char *s;
	struct block *bp;
	struct value *vp;

	if (f == NULL)
		f = stderr;
	bp = getblk(BLKTOP(a));
	switch (bp->b_type) {
	case B_PROC: { struct proc *p = (struct proc *) bp;
		fprintf(f, "%08o ", virtaddr(p));
		excvstr(p->p_name, f);
		fprintf(f, " %d arguments, %d locals\n", p->p_nargs,
			p->p_nlocals);
		excvstr(p->p_source, f);
		tseq(p->p_sym, ldump, 0, 0);
		bdump(p->p_code, f);
		for (i = 0; i < sizeof(p->p_points)/sizeof(int); i++)
			if (p->p_points[i])
				bdump(p->p_points[i], f);
		break; }
	case B_CODE: { struct code *p = (struct code *) bp;
		q = p->c_code;
		while (*q) {
			fprintf(f, "%08o ", virtaddr(q));
			if (*q >= 0 && *q <= 0377 && opname[*q])
				fprintf(f, "%s\n", opname[*q]);
			else
				fprintf(f, "%08o\n", *q);
			switch (*q) {
			case O_GVAR:
				vp = (struct value *)
					getblk((struct value *)(*++q) - 1);
				fprintf(f, "%08o ", virtaddr(q));
				image(*vp, f, 0);
				fprintf(f, "\n");
				putblk(vp, 0);
				break;
			case O_GVAL:
				vp = (struct value *) getblk(*++q);
				fprintf(f, "%08o ", virtaddr(q));
				image(*vp, f, 0);
				fprintf(f, "\n");
				putblk(vp, 0);
				break;
			case O_JUMP: case O_ITER: case O_JEV:
			case O_JNV: case O_BEV: case O_BNV:
				q++;
				fprintf(f, "%08o %08o\n", virtaddr(q), *q);
				break;
			case O_LVAR: case O_LVAL: case O_AVAR:
			case O_AVAL: case O_CALL: case O_QUIT: case O_ERR:
				q++;
				fprintf(f, "%08o %d\n", virtaddr(q), *q);
				break;
			case O_LINK:
				q1 = (int *) getblk(((struct code *) BLKTOP(q))->c_next);
				putblk(q, 0);
				q = ((struct code *) q1)->c_code;
				q--;
				}
			q++;
			if (((int)q&CHUNKMASK) == 0) {
				putblk(q-1, 0);
				break;
				}
			}
		return; }
	case B_STRING: { struct string *p = (struct string *) bp;
		fprintf(f, "%08o string\n", virtaddr(p));
		fprintf(f, "%08o next=%o", virtaddr(&p->s_next), p->s_next);
		for (s = p->s_chars; (struct string *) s < p + 1; s++) {
			if (((int)s&03) == 0)
				fprintf(f, "\n%08o", s);
			if (*s <= ' ' || *s == 0177)
				fprintf(f, " %03o", *s);
			else
				fprintf(f, " %3c", *s);
			}
		break; }
	case B_ARRAY: { struct array *p = (struct array *) bp;
		fprintf(f, "%08o array\n", virtaddr(p));
		fprintf(f, "%08o size=%d\n", virtaddr(&p->a_size), p->a_size);
		for (q = p->a_indir; (struct array *) q < p + 1; q++)
			fprintf(f, "%08o %o\n", virtaddr(q), *q);
		break; }
	case B_INDIR: { struct indir  *p = (struct indir *) bp;
		fprintf(f, "%08o indirect\n", virtaddr(p));
		for (q = p->i_data; (struct indir *) q < p + 1; q++)
			fprintf(f, "%08o %o\n", virtaddr(q), *q);
		break; }
	case B_TABLE: { struct table *p = (struct table *) bp;
		fprintf(f, "%08o table\n", virtaddr(p));
		fprintf(f, "%08o size=%d\n", virtaddr(&p->t_size), p->t_size);
		for (q = p->t_elem; (struct table *) q < p + 1; q++)
			fprintf(f, "%08o %o\n", virtaddr(q), *q);
		break; }
	case B_TELEM: { struct elem *p = (struct elem *) bp;
		fprintf(f, "%08o table element\n", virtaddr(p));
		fprintf(f, "%08o next=%d\n", virtaddr(&p->e_next), p->e_next);
		for (vp = p->e_vals; (struct elem *) (vp+1) <= p + 1; vp++) {
			fprintf(f, "%08o ", virtaddr(vp));
			vdump(*vp, f);
			putc('\n', f);
			}
		break; }
	case B_DATA: { struct data *p = (struct data *) bp;
		fprintf(f, "%08o data\n", virtaddr(p));
		for (vp = p->d_vals; (struct data *) (vp+1) <= p + 1; vp++) {
			fprintf(f, "%08o ", virtaddr(vp));
			vdump(*vp, f);
			putc('\n', f);
			}
		break; }
	case B_POINTS: { struct points *p = (struct points *) bp;
		fprintf(f, "%08o points\n", virtaddr(p));
		for (i = 0; i < sizeof(p->n_points)/sizeof(struct point); i++)
			fprintf(f, "%d..%d %08o\n", p->n_points[i].pos[0],
				p->n_points[i].pos[1], p->n_points[i].pc);
		break; }
	case B_FRAME: { struct frame *p = (struct frame *) bp;
		fprintf(f, "%08o frame\n", virtaddr(p));
		fprintf(f, "%08o sp=%d\n", virtaddr(&p->f_sp), p->f_sp);
		fprintf(f, "%08o prev=0%o\n", virtaddr(&p->f_prev), p->f_prev);
		fprintf(f, "%08o table=", virtaddr(&p->f_table));
		vdump(p->f_table, f);
		fprintf(f, "%08o lp=%d\n", virtaddr(&p->f_lp), p->f_lp);
		for (vp = p->f_vals; (struct frame *) (vp+1) <= p + 1; vp++) {
			fprintf(f, "%08o ", virtaddr(vp));
			vdump(*vp, f);
			putc('\n', f);
			}
		break; }
	default:
		fprintf(f, "%08o unknown type %d\n", virtaddr(bp), bp->b_type);
		for (q = bp->b_ints; (struct block *) q < bp + 1; q++)
			fprintf(f, "%08o %o\n", virtaddr(q), *q);
		}
	putblk(bp, 0);
}

/* compare - compare two arbitrary values, no conversions */
int compare(av, bv)
struct value av, bv;
{
	char ca, cb, *sa, *sb;
	int lena, lenb;

	if (TYPE(av) == T_STRING && TYPE(bv) == T_STRING) {
		sa = SOPENR(av.v_addr);
		sb = SOPENR(bv.v_addr);
		lena = XFIELD(av);
		lenb = XFIELD(bv);
		do {
			ca = lena>0?(lena--,SGETC(sa)):-1;
			cb = lenb>0?(lenb--,SGETC(sb)):-1;
			} while (ca == cb && ca >= 0);
		SCLOSER(sa);
		SCLOSER(sb);
		return (ca - cb);
		}
	if ((TYPE(av) == T_INT || TYPE(av) == T_REAL) &&
		(TYPE(bv) == T_INT || TYPE(bv) == T_INT)) {
			av = cvnum(av);
			bv = cvnum(bv);
			if (TYPE(av) == T_REAL || TYPE(bv) == T_REAL)
				return (getr(cvr(av)) - getr(cvr(bv)));
			else
				return (av.v_addr - bv.v_addr);
			}
	if (TYPE(av) == TYPE(bv))
		return (av.v_addr - bv.v_addr);
	return (TYPE(av) - TYPE(bv));
}

/* fchar - find character c in string s */
fchar(c, s)
char c;
struct value s;
{
	char cs, *ps;
	int i;

	ps = SOPENR(s.v_addr);
	for (i = 1; i <= XFIELD(s); i++)
		if (SGETC(ps) == c) {
			SCLOSER(ps);
			return (i);
			}
	SCLOSER(ps);
	return (0);
}

/* getidx - convert arbitrary position in l-char string to a positive one */
int getidx(i, l)
int i, l;
{
	if (i < -l || i > l+1)
		return (-1);
	if (i <= 0)
		i = l + i + 1;
	return (i);
}

/* getstr - get string v into C string, converting if necessary */
char *getstr(v)
struct value v;
{
	int i;
	char *s, *s1, *s2, *malloc();

	if (TYPE(v) != T_STRING)
		v = excvstr(v, NULL);
	if (TYPE(v) == T_VOID)
		return (NULL);
	if ((s = malloc(XFIELD(v)+1)) == NULL)
		return (NULL);
	s1 = SOPENR(v.v_addr);
	for (s2 = s, i = 0; i < XFIELD(v); i++)
		*s2++ = SGETC(s1);
	*s2 = 0;
	SCLOSER(s1);
	return (s);
}

/* sindex - adjust (opened) s to point to ith character */
char *sindex(s, i)
char *s;
int i;
{
	int n, q;
	struct string *s1;

	n = sizeof(struct block) - ((int)s&CHUNKMASK);
	for (i--; i >= n; ) {
		q = ((struct string *) BLKTOP(s))->s_next;
		putblk(s, 0);
		s = ((struct string *) getblk(q))->s_chars;
		i -= n;
		n = sizeof(s1->s_chars);
		}
	s += i;
	return (s);
}

/* snextr - simulates c = *p++ at string block boundaries */
int snextr(p)
char **p;
{
	int q;

	q = ((struct string *)BLKTOP(*p-1))->s_next;
	putblk(*p-1, 0);
	*p = (char *) ((struct string *) getblk(q))->s_chars;
	return (*(*p)++);
}

/* snextw - simulates *p++ = c at string block boundaries */
snextw(p, c)
char **p, c;
{
	struct string *q;

	q = (struct string *) balloc(B_STRING);
	((struct string *) BLKTOP(*p-1))->s_next = virtaddr(q);
	putblk(*p-1, 1);
	*p = (char *) q->s_chars;
	*(*p)++ = c;
}

/* stralloc - allocate a string and initialize it with C string s */
struct value stralloc(s, len)
char *s;
int len;
{
	struct value v;
	char *p;

	if (len < 0)
		len = strlen(s);
	if (strp == 0)
		strp = salloc();
	v = mkstr(strp, len);
	p = SOPENW(v.v_addr);
	for ( ; len > 0; len--)
		SPUTC(p, *s++);
	strp = (((int)p)&CHUNKMASK) == 0?0:virtaddr(p);
	SCLOSEW(p);
	return (v);
}

/* substr - compute substring of string v */
struct value substr(v, i1, i2)
struct value v;
int i1, i2;
{
	int i, n, q;
	struct string *s1;
	char *s;

	i1 = getidx(i1, XFIELD(v));
	i2 = getidx(i2, XFIELD(v));
	if (i1 < 0 || i2 < 0)
		return (VOID);
	if (i1 == i2)
		return (mkstr(v.v_addr, 0));
	if (i2 < i1) {
		i = i1; i1 = i2; i2 = i;
		}
	s = SOPENR(v.v_addr);
	n = sizeof(struct block) - ((int)s&CHUNKMASK);
	for (i = i1 - 1; i >= n; ) {
		q = ((struct string *) BLKTOP(s))->s_next;
		putblk(s, 0);
		s = ((struct string *) getblk(q))->s_chars;
		i -= n;
		n = sizeof(s1->s_chars);
		}
	s += i;
	v = mkstr(virtaddr(s), i2 - i1);
	SCLOSER(s);
	return (v);
}

/* vdump - print value v on f or stderr if f is NULL */
vdump(v, f)
struct value v;
FILE *f;
{
	if (f == NULL)
		f = stderr;
	switch (TYPE(v)) {
		case T_VOID:
			fprintf(f, "void");
			break;
		case T_STRING:
			fprintf(f, "string: length=%d, addr=0%o", XFIELD(v),
				v.v_addr);
			break;
		case T_PROC:
			fprintf(f, "procedure:");
			if (XFIELD(v))
				fprintf(f, " builtin #%d,", XFIELD(v));
			fprintf(f, " addr=0%o", v.v_addr);
			break;
		case T_INT:
			fprintf(f, "integer: %d", v.v_addr);
			break;
		case T_REAL:
			fprintf(f, "real: %g", getr(v));
			break;
		case T_TABLE:
			fprintf(f, "table: addr=0%o", v.v_addr);
			break;
		case T_VAR:
			fprintf(f, "variable:");
			if (XFIELD(v) == PHYSICAL)
				fprintf(f, " physical");
			if (XFIELD(v) == VIRTUAL)
				fprintf(f, " virtual");
			if (XFIELD(v) == SUBSTRING)
				fprintf(f, " substring");
			if (XFIELD(v) == SUBPROC)
				fprintf(f, " subprocedure");
			fprintf(f, " addr=0%o", v.v_addr);
			break;
		default:
			fprintf(f, "unknown: type=0%0, xfield=0%0, addr=0%o",
				TYPE(v), XFIELD(v), v.v_addr);
			break;
		}
}
