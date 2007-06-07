/* ez: built-in functions */

#include <stdio.h>
#include <stdlib.h>
#include "ez.h"

/* builtin - return pointer to builtin value s (a C string) or NULL */
struct value *builtin(s)
char *s;
{
	char buf[TOKSIZE];
	struct proc *p;
	struct bivalue *bp;
	int i;

	for (i = 0; bivalues[i].bi_name; i++)
		if (strcmp(s, bivalues[i].bi_name) == 0)
			break;
	if (bivalues[i].bi_name == NULL)
		return (NULL);
	bp = &bivalues[i];
	if (TYPE(bp->bi_v) == T_VOID)
		switch (bp->bi_type) {
		case T_VOID: case T_TABLE:
			break;
		case T_INT:
			bp->bi_v = mkint(atoi(bp->bi_str));
			break;
		case T_REAL:
			bp->bi_v = mkreal(atof(bp->bi_str));
			break;
		case T_STRING:
			bp->bi_v = stralloc(bp->bi_str,	bp->bi_x?bp->bi_x:-1);
			break;
		case T_PROC:
			sprintf(buf, "`%s`", bp->bi_name);
			p = (struct proc *) balloc(B_PROC);
			p->p_source = stralloc(buf, -1);
			p->p_name = substr(p->p_source, 2, -1);
			bp->bi_v = mkval(T_PROC, i, virtaddr(p));
			putblk(p, 1);
			break;
		default:
			err("builtin: invalid type");
		}
	return (&bp->bi_v);
}

/* popargs - common startup for find, many, match, and upto */
static popargs(n, ap, s1v, s2v, si, ei)
int n;
struct value *ap;
int *ei, *si;
struct value *s1v, *s2v;
{
	int i;
	struct value v;

	if (n < 2)
		return(0);
	*s1v = cvstr(*ap++);
	*s2v = cvstr(*ap++);
	*si = 1;
	*ei = XFIELD((*s2v))+1;
	if (n > 2) {
		v = cvi(*ap++);
		*si = getidx(v.v_addr, XFIELD((*s2v)));
		}
	if (n > 3) {
		v = cvi(*ap++);
		*ei = getidx(v.v_addr, XFIELD((*s2v)));
		}
	if (*si < 0 || *ei < 0)
		return(0);
	if (*si > *ei) {
		i = *si; *si = *ei; *ei = i;
		}
	return (1);
}

/* tdisplay - called to display a table element on f */
static int tdisplay(ivp, vp, f, b)
struct value *ivp, *vp;
FILE *f;
int b;
{
	excvstr(*ivp, f);
	fprintf(f, " = ");
	image(*vp, f, 1);
	fprintf(f, "\n");
	return (0);
}

/* x_arg - arg(i) */
static struct value x_arg(n, ap)
int n;
struct value *ap;
{
	struct value v;
	extern struct frame *fp;

	if (n >= 1) {
		v = cvi(*ap++);
		if (v.v_addr >= 1 && &fp->f_vals[1+v.v_addr] <=
			(struct value *) &fp->f_vals[fp->f_lp])
				return (mkval(T_VAR, PHYSICAL,
					&fp->f_vals[1+v.v_addr]));
		}
	return (VOID);
}

/* x_display - display(n) */
static struct value x_display(n, ap)
int n;
struct value *ap;
{
	struct value v;
	extern struct root *rp;
	extern struct frame *fp;

	if (n >= 1) {
		v = cvi(*ap);
		strace(v.v_addr, fp, 1);
		tseq(rp->r_wglobals, tdisplay, stderr, 0);
		}
	return (VOID);
}

/* x_dump - dump({ t }) */
static struct value x_dump(n, ap)
int n;
struct value *ap;
{
	for ( ; n-- > 0; ap++)
		if (TYPE(*ap) == T_TABLE)
			tseq(*ap, tdisplay, stderr, 0);
	return (VOID);
}

/* x_find - find(s1, s2 [, i [, j ] ]) */
static struct value x_find(n, ap)
int n;
struct value *ap;
{
	struct value s1v, s2v;
	char *s1, *s2;
	int s1sav, s2sav, i, si, ei, popargs();

	if (popargs(n, ap, &s1v, &s2v, &si, &ei) == 0)
		return (VOID);
	s2 = SOPENR(s2v.v_addr);
	if (si > 1)
		s2 = sindex(s2, si);
	for ( ; ei-si >= XFIELD(s1v); si++) {
		s1 = SOPENR(s1v.v_addr);
		if ((int)s2&CHUNKMASK)
			s2sav = virtaddr(s2);
		else {		 /* handle tricky boundary condition */
			SGETC(s2);
			s2sav = virtaddr(--s2);
			}
		for (i = si; SGETC(s1) == SGETC(s2); i++)
			if (1+i-si >= XFIELD(s1v)) {
				SCLOSER(s1);
				SCLOSER(s2);
				return (mkint(si));
				}
		SCLOSER(s1);
		SCLOSER(s2);
		s2 = SOPENR(s2sav);
		SGETC(s2);
		}
	SCLOSER(s2);
	return (VOID);
}

/* x_image - image(x) */
static struct value x_image(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? image(*ap, NULL, 1) : VOID);
}

/* x_integer - integer(x) */
static struct value x_integer(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? excvi(*ap) : VOID);
}

/* x_many - many(c, s [, i [, j ] ]) */
static struct value x_many(n, ap)
int n;
struct value *ap;
{
	struct value cv, sv;
	char c, *s;
	int i, si, ei;

	if (popargs(n, ap, &cv, &sv, &si, &ei) == 0)
		return (VOID);
	s = SOPENR(sv.v_addr);
	if (si > 1)
		s = sindex(s, si);
	for (i = si; i < ei; i++)
		if (fchar(SGETC(s), cv) == 0)
			break;
	SCLOSER(s);
	return (i > si ? mkint(i) : VOID);
}

/* x_map - map(s1, s2, s3) */
static struct value x_map(n, ap)
int n;
struct value *ap;
{
	struct value s1v, s2v, s3v, v;
	char c, c1, c2, c3, *s1, *s2, *s3, *sm;
	int i, j;

	if (n < 3)
		return (VOID);
	s1v = *ap++;
	if (TYPE(s1v) == T_VOID)
		return (VOID);
	s1v = cvstr(s1v);
	s2v = cvstr(*ap++);
	s3v = cvstr(*ap++);
	if (XFIELD(s2v) != XFIELD(s3v))
		err("second and third arguments to map of unequal length");
	s1 = SOPENR(s1v.v_addr);
	s2 = SOPENR(s2v.v_addr);
	s3 = SOPENR(s3v.v_addr);
	v = mkstr(salloc(), XFIELD(s1v));
	sm = SOPENW(v.v_addr);
	for (i = 1; i <= XFIELD(s1v); i++) {
		c = c1 = SGETC(s1);
		for (j = 1; j <= XFIELD(s2v); j++) {
			c2 = SGETC(s2);
			c3 = SGETC(s3);
			if (c1 == c2)
				c = c3;
			}
		SPUTC(sm, c);
		SCLOSER(s2); s2 = SOPENR(s2v.v_addr);
		SCLOSER(s3); s3 = SOPENR(s3v.v_addr);
		}
	SCLOSER(s1);
	SCLOSER(s2);
	SCLOSER(s3);
	SCLOSEW(sm);
	return (v);
}

/* x_match - match(s1, s2 [, i [, j ] ]) */
static struct value x_match(n, ap)
int n;
struct value *ap;
{
	struct value s1v, s2v, v;
	char *s1, *s2;
	int i, si, ei;

	if (popargs(n, ap, &s1v, &s2v, &si, &ei) == 0)
		return (VOID);
	if (ei - si < XFIELD(s1v))
		return (VOID);
	s1 = SOPENR(s1v.v_addr);
	s2 = SOPENR(s2v.v_addr);
	if (si > 1)
		s2 = sindex(s2, si);
	v = VOID;
	for (i = si; SGETC(s1) == SGETC(s2); i++)
		if (1+i-si >= XFIELD(s1v)) {
			v = mkint(i+1);
			break;
			}
	SCLOSER(s1);
	SCLOSER(s2);
	return (v);
}

/* x_numeric - numeric(x) */
static struct value x_numeric(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? excvnum(*ap) : VOID);
}

/* x_proc - proc(x) (i.e. compile x) */
static struct value x_proc(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? excvproc(*ap) : VOID);
}

/* x_read - read([ n | s ]) */
static struct value x_read(n, ap)
int n;
struct value *ap;
{
	struct value v;
	char *s;
	FILE *fp;
	int c, nc, mode;

	mode = 1;
	fp = stdin;
	if (n > 0) {
		v = excvi(*ap);
		if (TYPE(v) == T_VOID) {
			if ((s = getstr(cvstr(*ap))) == NULL ||
				(fp = fopen(s, "r")) == NULL)
					return (VOID);
			free(s);
			mode = 2;
			}
		else {
			mode = 3;
			if ((nc = v.v_addr) == 0)
				return (stralloc("", 0));
			}
		}
	v = mkstr(salloc(), 0);
	s = SOPENW(v.v_addr);
	if (mode == 1)		/* read a line */
		for (n = 0; (c = getc(fp)) != EOF; n++) {
			if (c == '\n')
				break;
			SPUTC(s, c);
			}
	else if (mode == 2) {	/* read entire file */
		for (n = 0; (c = getc(fp)) != EOF; n++)
			SPUTC(s, c);
		}
	else if (mode == 3)	/* read nc characters */
		for (n = 0; nc > 0 && (c = getc(fp)) != EOF; nc--, n++)
			SPUTC(s, c);
	SCLOSEW(s);
	if (n == 0 && c == EOF)
		v = VOID;
	else
		v = mkstr(v.v_addr, n);
	if (fp != stdin)
		fclose(fp);
	return (v);
}

/* x_real - real(x) */
static struct value x_real(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? excvr(*ap) : VOID);
}

/* x_refresh - refresh(n, s) */
static struct value x_refresh(n, ap)
int n;
struct value *ap;
{
	return (VOID);
}

/* x_remove - remove({ x }) */
static struct value x_remove(n, ap)
int n;
struct value *ap;
{
	struct value t;

	for ( ; n >= 2; n -= 2) {
		t = *ap++;
		tremove(t, *ap++);
		}
	return (VOID);
}

/* x_size - size(x) */
static struct value x_size(n, ap)
int n;
struct value *ap;
{
	struct value v;
	struct table *tp;

	if (n < 1)
		return (VOID);  
	v = *ap;
	if (TYPE(v) == T_TABLE) {
		tp = (struct table *) getblk(v.v_addr);
		v = mkint(tp->t_size);
		putblk(tp, 0);
		}
	else {
		if (TYPE(v) != T_STRING)
			v = excvstr(v, NULL);
		if (TYPE(v) != T_VOID)
			v = mkint(XFIELD(v));
		}
	return (v);
}

/* x_string - string(x) */
static struct value x_string(n, ap)
int n;
struct value *ap;
{
	return (n >= 1 ? excvstr(*ap, NULL) : VOID);
}

/* x_system - system(s) */
static struct value x_system(n, ap)
int n;
struct value *ap;
{
	char *s;
	struct value v;

	if (n == 1 && (s = getstr(cvstr(*ap++)))) {
		v = mkint(system(s));
		free(s);
		return (v);
		}
	return (VOID);
}

/* x_table - table(x) */
static struct value x_table(n, ap)
int n;
struct value *ap;
{
	struct value v, *vp;
	struct array *b;

	if (n >= 1 && TYPE(*ap) == T_TABLE)
		return (*ap);
	if (n >= 1 && TYPE(*ap) == T_PROC)
		return (create(*ap));
	b = (struct array *) balloc(B_ARRAY);
	v = mkval(T_TABLE, 0, virtaddr(b));
	putblk(b, 1);
	if (n >= 1) {
		vp = tindex(v, mkint(1), 1);
		*vp = *ap;
		putblk(vp, 1);
		}
	return (v);
}

/* x_trace - trace(n) */
static struct value x_trace(n, ap)
int n;
struct value *ap;
{
	struct value v;
	extern int trace;

	if (n >= 1) {
		v = cvi(*ap++);
		trace = v.v_addr;
		}
	return (VOID);
}

/* x_type - type(x) */
static struct value x_type(n, ap)
int n;
struct value *ap;
{
	char *s;

	if (n < 1)
		return (VOID);
	switch (TYPE(*ap)) {
		case T_VOID:
			s = "";
			break;
		case T_INT:
			s = "integer";
			break;
		case T_REAL:
			s = "real";
			break;
		case T_STRING:
			s = "string";
			break;
		case T_PROC:
			s = "procedure";
			break;
		case T_TABLE:
			s = "table";
			break;
		}
	return (stralloc(s, -1));
}

/* x_upto - upto(c, s [, i [, j ] ]) */
static struct value x_upto(n, ap)
int n;
struct value *ap;
{
	struct value cv, sv, v;
	char c, *s;
	int i, si, ei;

	if (popargs(n, ap, &cv, &sv, &si, &ei) == 0)
		return (VOID);
	s = SOPENR(sv.v_addr);
	if (si > 1)
		s = sindex(s, si);
	v = VOID;
	for (i = si; i < ei; i++)
		if (fchar(SGETC(s), cv)) {
			v = mkint(i);
			break;
			}
	SCLOSER(s);
	return (v);
}

/* x_write - write({ x }) */
static struct value x_write(n, ap)
int n;
struct value *ap;
{
	FILE *fp;

	while (n-- > 0)
		excvstr(*ap++, stdout);
	return (VOID);
}

struct bivalue bivalues[] = {	/**** do not change position of functions ****/
	"void",    T_VOID, 0, NULL, NULL,       {T_VOID, 0},
	"arg",     T_PROC, 0, NULL, x_arg,      {T_VOID, 0},
	"display", T_PROC, 0, NULL, x_display,  {T_VOID, 0},
	"dump",    T_PROC, 0, NULL, x_dump,     {T_VOID, 0},
	"find",    T_PROC, 0, NULL, x_find,     {T_VOID, 0},
	"image",   T_PROC, 0, NULL, x_image,    {T_VOID, 0},
	"integer", T_PROC, 0, NULL, x_integer,  {T_VOID, 0},
	"many",    T_PROC, 0, NULL, x_many,     {T_VOID, 0},
	"map",     T_PROC, 0, NULL, x_map,      {T_VOID, 0},
	"match",   T_PROC, 0, NULL, x_match,    {T_VOID, 0},
	"numeric", T_PROC, 0, NULL, x_numeric,  {T_VOID, 0},
	"proc",    T_PROC, 0, NULL, x_proc,     {T_VOID, 0},
	"read",    T_PROC, 0, NULL, x_read,     {T_VOID, 0},
	"remove",  T_PROC, 0, NULL, x_remove,   {T_VOID, 0},
	"size",    T_PROC, 0, NULL, x_size,     {T_VOID, 0},
	"string",  T_PROC, 0, NULL, x_string,   {T_VOID, 0},
	"trace",   T_PROC, 0, NULL, x_trace,    {T_VOID, 0},
	"type",    T_PROC, 0, NULL, x_type,     {T_VOID, 0},
	"upto",    T_PROC, 0, NULL, x_upto,     {T_VOID, 0},
	"write",   T_PROC, 0, NULL, x_write,    {T_VOID, 0},
	"system",  T_PROC, 0, NULL, x_system,   {T_VOID, 0},
	"table",   T_PROC, 0, NULL, x_table,    {T_VOID, 0},
	"root",    T_TABLE,0, NULL, NULL,       {T_VOID, 0},
	"lcase",   T_STRING, 0, "abcdefghijklmnopqrstuvwxyz",
		NULL, {T_VOID, 0},
	"ucase",   T_STRING, 0, "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		NULL, {T_VOID, 0},
	"ascii",   T_STRING, 128, "\0\01\02\03\04\05\06\07\b\t\n\013\014\r\
\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\
\036\037\040!\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRS\
TUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\177",
		NULL, {T_VOID, 0},
	"version", T_STRING, 0,	VERSION,NULL,	{T_VOID, 0},
	NULL,	   0,        0,	NULL,	NULL,	{T_VOID, 0}
};
