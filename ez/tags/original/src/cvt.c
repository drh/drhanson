/* ez: conversions and type constructors */

#include <stdio.h>
#include <ctype.h>
#include "ez.h"

/* cvnum - implicit conversion to numeric */
struct value cvnum(v)
struct value v;
{
	struct value v1;

	v1 = excvnum(v);
	if (TYPE(v1) == T_VOID)
		err("numeric expected", v);
	return (v1);
}

/* cvr - implicit conversion to real */
struct value cvr(v)
struct value v;
{
	struct value v1;

	v1 = excvr(v);
	if (TYPE(v1) == T_VOID)
		err("real expected", v);
	return (v1);
}

/* cvi - implicit conversion to integer */
struct value cvi(v)
struct value v;
{
	struct value v1;

	v1 = excvi(v);
	if (TYPE(v1) == T_VOID)
		err("integer expected", v);
	return (v1);
}

/* cvstr - implicit conversion to string */
struct value cvstr(v)
struct value v;
{
	struct value v1, cvstr();

	v1 = excvstr(v, NULL);
	if (TYPE(v1) == T_VOID)
		err("string expected", v);
	return (v1);
}

/* deref - dereference a variable and return its value */
struct value deref(v)
struct value v;
{
	struct value *vp;

	while (TYPE(v) == T_VAR)
		if (XFIELD(v)) {
			vp = (struct value *) getblk(v.v_addr);
			v = *vp;
			putblk(vp, 0);
			}
		else
			v = *(struct value *) v.v_addr;
	return (v);
}

/* excvproc - explicit conversion to procedure */
struct value excvproc(v)
struct value v;
{
	struct value v1;

	if (TYPE(v) == T_PROC)
		return (v);
	v = excvstr(v, NULL);
	if (TYPE(v) == T_STRING)
		v = ezparse(v);
	return (v);
}

/* excvi - explicit conversion to integer */
struct value excvi(v)
struct value v;
{
	float getr();

	v = excvnum(v);
	if (TYPE(v) == T_VOID)
		return (VOID);
	if (TYPE(v) == T_INT)
		return (v);
	return (mkint((int) getr(v)));
}

/* excvr - explicit conversion to real */
struct value excvr(v)
struct value v;
{
	v = excvnum(v);
	if (TYPE(v) == T_VOID)
		return (VOID);
	if (TYPE(v) == T_REAL)
		return (v);
	return (mkreal((float) v.v_addr));
}

/* excvnum - explicit conversion to numeric */
struct value excvnum(v)
struct value v;
{
	int n, len;
	float d;
	char c, *s;

	if (TYPE(v) == T_INT || TYPE(v) == T_REAL)
		return (v);
	if (TYPE(v) != T_STRING)
		return (VOID);
	len = XFIELD(v);
	s = SOPENR(v.v_addr);
	c = len>0?(len--,SGETC(s)):0;
	while (isspace(c))
		c = len>0?(len--,SGETC(s)):0;
	for (n = 0; isdigit(c); c = len>0?(len--,SGETC(s)):0)
		n = 10*n + c - '0';
	if (c == '.') {
		c = len>0?(len--,SGETC(s)):0;
		for (d = 1.0; isdigit(c); c = len>0?(len--,SGETC(s)):0) {
			n = 10*n + c - '0';
			d *= 10.0;
			}
		v = mkreal(n/d);
		}
	else
		v = mkint(n);
	while (isspace(c))
		c = len>0?(len--,SGETC(s)):0;
	if (c != 0)
		v = VOID;
	SCLOSER(s);
	return (v);
}

/* excvstr - explicit conversion to string, or output of v to f */
struct value excvstr(v, f)
struct value v;
FILE *f;
{
	char *s, *cvs1, str[TOKSIZE];
	struct proc *p;
	int a, i, len1, xtos();
	static char *cvs = NULL;
	static int len = 0;

	switch (TYPE(v)) {
		case T_STRING:
			if (f || cvs) {
				s = SOPENR(v.v_addr);
				if (f)
					for (i = XFIELD(v); i > 0; i--)
						putc(SGETC(s), f);
				else
					for (i = XFIELD(v); i > 0; i--,len++)
						SPUTC(cvs, SGETC(s));
				SCLOSER(s);
				}
			break;
		case T_PROC:
			p = (struct proc *) getblk(v.v_addr);
			v = p->p_source;
			putblk(p, 0);
			v = excvstr(v, f);
			break;
		case T_INT:
			if (f)
				fprintf(f, "%d", v.v_addr);
			else {
				sprintf(str, "%d", v.v_addr);
				if (cvs) {
					for (s = str; *s; s++, len++)
						SPUTC(cvs, *s);
					}
				else
					v = stralloc(str, -1);
				}
			break;
		case T_REAL:
			if (f)
				fprintf(f, "%g", getr(v));
			else {
				sprintf(str, "%g", getr(v));
				if (cvs) {
					for (s = str; *s; s++, len++)
						SPUTC(cvs, *s);
					}
				else
					v = stralloc(str, -1);
				}
			break;
		case T_TABLE:
			if (f)
				tseq(v, xtos, excvstr, f);
			else if (cvs == NULL) {
				cvs1 = cvs;
				len1 = len;
				cvs = SOPENW(a = salloc());
				len = 0;
				tseq(v, xtos, excvstr, NULL);
				SCLOSEW(cvs);
				v = mkstr(a, len);
				cvs = cvs1;
				len = len1;
				}
			break;
		default:
			v = VOID;
		}
	if (f)
		v = VOID;
	return (v);
}

/* getr - extract and return the real number stored in v */
float getr(v)
struct value v;
{
	float *p;

	p = (float *) &v.v_addr;
	return (*p);
}

/* image - return short/long `image' of v, or write image of v to f */
struct value image(v, f, longform)
struct value v;
FILE *f;
int longform;
{
	char c, *s, *s1, *cvssav, *index();
	struct proc *p;
	int a, i, n, lensav, xtoi();
	static char *cvs = NULL;
	static int len = 0;
#define iputc(c) if (f) putc(c, f); else {SPUTC(cvs, c); len++;} n++

	cvssav = cvs;
	lensav = len;
	switch (TYPE(v)) {
		case T_STRING:
			if (f == NULL && cvs == NULL) {
				cvs = SOPENW(a = salloc());
				len = 0;
				}
			s = SOPENR(v.v_addr);
			iputc('"');
			for (n = 0, i = XFIELD(v); i > 0; i--) {
				if ((c = SGETC(s)) >= ' ' && c < 0177) {
					iputc(c);
					}
				else if (c && (s1 = index("\bb\tt\nn", c))) {
					iputc('\\');
					iputc(*++s1);
					}
				else {
					iputc('\\');
					iputc('0'+((c>>6)&07));
					iputc('0'+((c>>3)&07));
					iputc('0'+(c&07));
					}
				if (longform == 0 && n >= 60) {
					iputc('.');
					iputc('.');
					iputc('.');
					break;
					}
				}
			if (i <= 0)
				iputc('"');
 			if (f == NULL) {
				SCLOSEW(cvs);
				v = mkstr(a, len);
				}
			SCLOSER(s);
			break;
		case T_PROC:
			p = (struct proc *) getblk(v.v_addr);
			v = image(p->p_source, f, longform);
			putblk(p, 0);
			break;
		case T_INT: case T_REAL:
			v = excvstr(v, f);
			break;
		case T_TABLE:
			if (f)
				fprintf(f, "table");
			else
				v = stralloc("table", -1);
			break;
		case T_VOID:
			if (f)
				fprintf(f, "void");
			else
				v = stralloc("void", -1);
			break;
		default:
			v = VOID;
		}
	if (f)
		v = VOID;
	cvs = cvssav;
	len = lensav;
	return (v);
}

/* mkint - construct and return an integer value */
struct value mkint(i)
int i;
{
	struct value v;

	v.v_type = T_INT;
	v.v_x = 0;
	v.v_addr = i;
	return (v);
}

/* mkreal - construct and return a real value */
struct value mkreal(r)
float r;
{
	struct value v;
	int *p;

	p = (int *) &r;
	v.v_type = T_REAL;
	v.v_x = 0;
	v.v_addr = *p;
	return (v);
}

/* mkstr - construct and return a string value; l is the length */
struct value mkstr(s, l)
char *s;
int l;
{
	struct value v;

	v.v_type = T_STRING;
	v.v_x = l;
	v.v_addr = (int) s;
	return (v);
}

/* mkval - construct and return a value of type t, with x and a fields */
struct value mkval(t, x, a)
int t, x, a;
{
	struct value v;

	v.v_type = t;
	v.v_x = x;
	v.v_addr = a;
	return (v);
}

/* xtos - convert arbitrary table element to string by calling f */
static int xtos(typ, vp, f, fp)
struct value *typ, *vp;
int (*f)();
FILE *fp;
{
	if (TYPE(*vp) != T_TABLE)
		(*f)(*vp, fp);
	return (0);
}
