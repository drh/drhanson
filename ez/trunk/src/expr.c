/* ez compiler: expression parsing */

#include <stdio.h>
#include "ez.h"
#include "tokens.h"

struct node *freelist = NULL;		/* node free list */
int ncount = 0;				/* counts nodes allocated */
int oprtab[][7] = {			/* operator precedence table */
	{'|', 0, 0, 0, 0, 0, 0},
	{'&', 0, 0, 0, 0, 0, 0},
	{EQ, NE, '<', LE, GE, '>', 0},
	{CAT, 0, 0, 0, 0, 0, 0},
	{'+', '-', 0, 0, 0, 0, 0},
	{'*', '/', '%', 0, 0, 0, 0},
	0
};

int binarytab[] = {'|', '&', CAT, EQ, NE, '<', '>', LE, GE, '+', '-',
	'*', '/', '%', '=', 0};
int unarytab[] = {UNARY '~', UNARY '-', UNARY '+', 0};

int optab[] = {			/* token -> operator translation table */
	UNARY '~', O_NOT, UNARY '-', O_NEG, '*', O_MUL, '/', O_DIV, '%', O_MOD,
	'+', O_ADD, '-', O_SUB,	CAT, O_CAT,
	'<', O_LT, LE, O_LE, GE, O_GE, '>', O_GT, EQ, O_EQ, NE, O_NE,
	'=', O_ASGN, '|', BINARY '|', '&', BINARY '&',
	0};
int condtab[] = {O_NOT, O_LT, O_LE, O_GE, O_GT, O_EQ, O_NE,
	BINARY '&', BINARY '|', 0};

static int follow[] = {'\n', ')', ',', ']', IN, '!', ':', ';', '}', ELSE,
	END, EOF, 0};

int cnstp = 0;			/* virtual address of constant storage */

struct node *cvt(), *enode(), *ident(), *lvalue(), *rvalue(), *node(),
	*expr(), *expr1(), *expr2(), *term();
extern struct root *rp;

/* expr - expression */
struct node *expr()
{
	return (node(UNARY '@', T_VOID, expr1(), NULL));
}

/* expr	: expr [ binop ] = expr
 *	| term
 *
 * parse an expression
 */
struct node *expr1()
{
	int op;
	struct node *p;

	p = expr2(0);
	while (op = isop(t, binarytab)) {
		t = gtok('\n');
		if (op == O_ASGN)
			p = enode(op, p, expr1());
		else {
			mustbe('=', '\n');
			p = lvalue(p);
			p = enode(O_ASGN, p, enode(op, p, expr1()));
			}
		}
	if (inset(t, follow) == 0) {
		error("illegal expression termination", "");
		skipto(follow, 0);
		}
	return (p);
}

struct node *expr2(n)
int n;
{
	int op, c;
	struct node *p;

	if (*oprtab[n] == 0)
		return (term());
	p = expr2(n + 1);
	while (putback(ngetc()) != '=' && (op = isop(t, oprtab[n]))) {
		t = gtok('\n');
		p = enode(op, p, expr2(n + 1)); 
		}
	return (p);
}

/*
 * term	: ( - | + | ~ ) term
 *	| term `[' expr `]'
 *	| term `[' expr ( : | ! ) expr `]'
 *	| term `(' [ expr { , expr } ] `)'
 *	| `[' expr [ : expr ] { , expr [ : expr } } `]'
 *	| id
 *	| con
 *	| fcon
 *	| ccon
 *	| scon
 *	| procedure (...) ... end
 *	| `(' expr `)'
 *
 * parse a term
 */
struct node *term()
{
	int op, a, i;
	struct symbol *q;
	struct node *p, *p1;
	struct value v, *vp;
	extern struct root *rp;

	switch (t) {
		case '-': case '~': case '+':
			op = isop(UNARY t, unarytab);
			t = gtok('\n');
			p = enode(op, term(), NULL);
			break;
		case ID:
			p = ident(lexval.l_str);
			t = gtok(0);
			break;
		case CON:
			p = node(O_GVAL, T_INT,
				constant(mkint(atoi(lexval.l_str))), NULL);
			t = gtok(0);
			break;
		case FCON:
			p = node(O_GVAL, T_REAL,
				constant(mkreal(atof(lexval.l_str))), NULL);
			t = gtok(0);
			break;
		case SCON:
			p = node(O_GVAL, T_STRING,
				constant(stralloc(lexval.l_str,lexval.l_length)),
				NULL);
			t = gtok(0);
			break;
		case BCON:
			if (vp = builtin(lexval.l_str))
				v = *vp;
			else {
				error("invalid built-in constant: ",
					lexval.l_str);
				v = VOID;
				}
			p = node(O_GVAL, TYPE(v), constant(v), NULL);
			t = gtok(0);
			break;
		case PROCEDURE:
			p = node(O_GVAL, T_PROC,
				constant(mkval(T_PROC, 0, func())), NULL);
			break;
		case '(':
			t = gtok('\n');
			p = expr1();
			newline();
			mustbe(')', 0);
			break;
		case '[':
			t = gtok('\n');
			for (i = 1, p = NULL; t != ']'; i++) {
				p1 = rvalue(expr1());
				if (t == ':') {
					t = gtok('\n');
					p1 = node(BINARY '+', T_VOID, p1,
						rvalue(expr1()));
					}
				else
					p1 = node(BINARY '+', T_VOID,
						node(O_GVAL, T_INT,
							constant(mkint(i)), NULL),
						p1);
				p = node(BINARY ',', T_VOID, p, p1);
				if (t == ',')
					t = gtok('\n');
				else if (t != ']')
					break;
				}
			p = node(UNARY '[', T_TABLE, p, NULL);
			newline();
			mustbe(']', 0);
			break;
		default:
			error("illegal expression", "");
			return (node(O_GVAL, T_INT, constant(mkint(0)), NULL));
		}
	while (t == '.' || t == '[' || t == '(')
		switch (t) {
			case '.':
				t = gtok('\n');
				if (t == ID) {
					a = constant(stralloc(lexval.l_str, -1));
					t = gtok(0);
					}
				else {
					error("identifier expected", "");
					a = constant(stralloc("", 0));
					}
				p = enode(O_IDXL, p,
					node(O_GVAL, T_STRING, a, NULL));
				break;
			case '[':
				t = gtok('\n');
				p1 = expr1();
				if (t == ':' || t == '!') {
					op = BINARY t;
					t = gtok('\n');
					p = enode(O_SSTRL, p,
						enode(op, p1, expr1()));
					}
				else
					p = enode(O_IDXL, p, p1);
				mustbe(']', 0);
				break;
			case '(':
				t = gtok('\n');
				p1 = NULL;
				while (t != ')') {
					p1 = node(BINARY ',', T_VOID, p1,
						rvalue(expr1()));
					if (t == ',')
						t = gtok('\n');
					else if (t != ')')
						break;
					}
				mustbe(')', 0);
				p = enode(O_CALL, p, p1);
				break;
			}
	return (p);
}

/* isop - return operator for t if t is in set */
int isop(t, set)
int t, set[];
{
	int i, op;

	for (i = 0; set[i]; i++)
		if (t == set[i]) {
			for (i = 0; op = optab[i]; i += 2)
				if (t == op)
					return (optab[i+1]);
			return (t);
			}
	return (0);
}

/* cvt - add conversion node to convert p to type ty */
struct node *cvt(p, ty)
struct node *p;
int ty;
{
	p = rvalue(p);
	if (ty == p->ex_type)
		return (p);
	switch (ty) {
		case T_INT:
			if (p->ex_type != T_INT)
				p = node(O_CVI, T_INT, p, 0);
			break;
		case T_REAL:
			if (p->ex_type != T_INT && p->ex_type != T_REAL)
				p = node(O_CVNUM, T_INT, p, 0);
			break;
		case T_STRING:
			p = node(O_CVSTR, T_STRING, p, 0);
			break;
		}
	return (p);
}

/* enode - allocate an expression node, add conversion nodes */
struct node *enode(op, left, right)
int op;
struct node *left, *right;
{
	struct node *p;
	int n, ty;

	ty = T_VOID;
	switch (op) {
		case UNARY '+':
			return (cvt(left, T_REAL));
		case O_NEG:
			left = cvt(left, T_REAL);
			ty = T_REAL;
			break;
		case O_IDXL: case O_SSTRL:
			right = rvalue(right);
			ty = T_VAR;
			break;
		case O_CALL:
			ty = T_VAR;
		case O_LT: case O_LE: case O_EQ: case O_CAT:
		case O_NE: case O_GE: case O_GT: case BINARY ',':
		case BINARY ':':
			right = rvalue(right);
			left = rvalue(left);
			break;
		case UNARY '?':
			if (isop(left->ex_op, condtab))
				return (left);
			left = rvalue(left);
			break;
		case BINARY '&': case BINARY '|':
			right = enode(UNARY '?', right, NULL);
		case O_NOT:
			left = enode(UNARY '?', left, NULL);
			break;
		case O_SSTR:
			ty = T_VAR;
			break;
		case O_ASGN:
			left = lvalue(left);
			right = rvalue(right);
			ty = right->ex_type;
			break;
		case O_ADD: case O_SUB: case O_MUL: case O_DIV: case O_MOD:
			left = cvt(rvalue(left), T_REAL);
			right = cvt(rvalue(right), T_REAL);
			if (left->ex_type == T_INT && right->ex_type == T_INT)
				ty = T_INT;
			else
				ty = T_REAL;
			break;
		case BINARY '!':
			left = cvt(left, T_INT);
			right = cvt(right, T_INT);
			break;
		default:
			err("enode: undefined operator");
		}
	return (node(op, ty, left, right));
}

/* node - allocate a node with the given fields */
struct node *node(op, type, left, right)
int op, type;
struct node *left, *right;
{
	struct node *p;

	if (p = freelist)
		freelist = p->ex_l;
	else {
		p = (struct node *) alloc(sizeof(struct node));
		ncount++;
		}
	p->ex_op = op;
	p->ex_type = type;
	p->ex_l = left;
	p->ex_r = right;
	p->ex_result = p->ex_count = 0;
	if (op != O_GVAL && op != O_GVAR && op != O_AVAL && op != O_AVAR &&
		op != O_LVAL && op != O_LVAR) {
			if (left)
				left->ex_count++;
			if (right)
				right->ex_count++;
			}
	return (p);
}

/* ident - construct node for referencing an identifier */
struct node *ident(id)
char *id;
{
	struct value *vp;
	int a;
	struct node *p;

	vp = (struct value *) getblk(a = lookup(id));
	if (TYPE(*vp) == T_VAR && vp->v_addr > 0)
		p = node(O_LVAR, T_VAR, vp->v_addr, NULL);
	else if (TYPE(*vp) == T_VAR && vp->v_addr < 0)
		p = node(O_AVAR, T_VAR, -vp->v_addr, NULL);
	else
		p = node(O_GVAR, T_VAR, a, NULL);
	putblk(vp, 0);
	return (p);
}

/* lvalue - check that p is an lvalue */
struct node *lvalue(p)
struct node *p;
{
	if (p->ex_op == (UNARY '@'))
		p->ex_l = lvalue(p->ex_l);
	else {
		if (p->ex_type != T_VAR)
			error("variable expected", "");
		p->ex_type = T_VAR;
		}
	return (p);
}

/* rvalue - convert p to an rvalue, if necessary */
struct node *rvalue(p)
struct node *p;
{
	if (p->ex_op == (UNARY '@')) {
		p->ex_l->ex_count--;
		p->ex_l = rvalue(p->ex_l);
		}
	else if (p->ex_type == T_VAR) {
		p = node(O_DEREF, T_VOID, p, NULL);
		p->ex_type = T_VOID;
		}
	return (p);
}

/* exprgen - walk expression tree and generate code */
exprgen(p, tlab, flab)
struct node *p;
int tlab, flab;
{
	struct value *vp, v;
	struct node *p1;
	int op, n;

	if (p == NULL)
		return;
	if (p->ex_result) {
		emit1(O_DUP);
		return;
		}
	switch (op = p->ex_op) {
		case UNARY '@':
			if (tlab || flab) {
				p->ex_l->ex_count--;
				p->ex_l = enode(UNARY '?', p->ex_l, NULL);
				}
			exprgen(p->ex_l, tlab, flab);
			freenode(p->ex_l);
			freenode(p);
			return;
		case O_GVAR: case O_AVAR: case O_LVAR:
		case O_AVAL: case O_LVAL: case O_GVAL:
			emit(2, op, p->ex_l);
			p->ex_result++;
			return;
		case O_NOT:
			if (tlab || flab)
				exprgen(p->ex_l, flab, tlab);
			else {
				exprgen(p->ex_l, 0, 0);
				emit1(op);
				}
			break;
		case UNARY '?':
			exprgen(p->ex_l, 0, 0);
			if (tlab)
				jump(O_JNV, tlab);
			else if (flab)
				jump(O_JEV, flab);
			break;
		case BINARY '&':
			if (tlab) {
				exprgen(p->ex_l, 0, flab = genlabel(1));
				exprgen(p->ex_r, tlab, 0);
				deflabel(flab);
				}
			else if (flab) {
				exprgen(p->ex_l, 0, flab);
				exprgen(p->ex_r, 0, flab);
				}
			else {
				exprgen(p->ex_l, 0, 0);
				jump(O_BEV, flab = genlabel(1));
				exprgen(p->ex_r, 0, 0);
				deflabel(flab);
				}
			break;
		case BINARY '|':
			if (tlab) {
				exprgen(p->ex_l, tlab, 0);
				exprgen(p->ex_r, tlab, 0);
				}
			else if (flab) {
				exprgen(p->ex_l, tlab = genlabel(1), 0);
				exprgen(p->ex_r, 0, flab);
				deflabel(tlab);
				}
			else {
				exprgen(p->ex_l, 0, 0);
				jump(O_BNV, tlab = genlabel(1));
				exprgen(p->ex_r, 0, 0);
				deflabel(tlab);
				}
			break;
		case O_LT: case O_LE: case O_EQ:
		case O_NE: case O_GE: case O_GT:
			exprgen(p->ex_l, 0, 0);
			exprgen(p->ex_r, 0, 0);
			emit1(op);
			if (tlab)
				jump(O_JNV, tlab);
			else if (flab)
				jump(O_JEV, flab);
			break;
		case O_ASGN: case O_SSTRL: case O_CAT:
		case O_ADD: case O_SUB: case O_MUL: case O_NEG:
		case O_DIV: case O_MOD: case O_IDX: case O_IDXL:
		case O_CVI: case O_CVR: case O_CVSTR: case O_CVNUM:
			exprgen(p->ex_l, 0, 0);
			exprgen(p->ex_r, 0, 0);
			emit1(op);
			break;
		case O_DEREF:
			if (p->ex_l->ex_result)
				;
			else if (p->ex_l->ex_op == O_GVAR)
				op = O_GVAL;
			else if (p->ex_l->ex_op == O_AVAR)
				op = O_AVAL;
			else if (p->ex_l->ex_op == O_LVAR)
				op = O_LVAL;
			else if (p->ex_l->ex_op == O_IDXL) {
				p->ex_l->ex_l->ex_count--;
				p->ex_l->ex_l = rvalue(p->ex_l->ex_l);
				op = O_IDX;
				}
			else if (p->ex_l->ex_op == O_SSTRL) {
				p->ex_l->ex_l->ex_count--;
				p->ex_l->ex_l = rvalue(p->ex_l->ex_l);
				op = O_SSTR;
				}
			if (op != O_DEREF)
				p->ex_l->ex_op = op;
			exprgen(p->ex_l, 0, 0);
			if (op == O_DEREF)
				emit1(op);
			break;
		case O_SSTR:
			p->ex_l->ex_count--;
			exprgen(p->ex_l = rvalue(p->ex_l), 0, 0);
			exprgen(p->ex_r, 0, 0);
			emit1(op);
			break;
		case O_CALL:
			for (n = 0, p1 = p->ex_r; p1; n++, p1 = p1->ex_l);
			exprgen(p->ex_l, 0, 0);
			exprgen(p->ex_r, 0, 0);
			emit(2, op, n);
			break;
		case BINARY ',': case BINARY ':':
			exprgen(p->ex_l, 0, 0);
			exprgen(p->ex_r, 0, 0);
			break;
		case BINARY '!':
			exprgen(p->ex_l, 0, 0);
			emit1(O_DUP);
			if (p->ex_l->ex_type != T_INT)
				emit1(O_CVI);
			exprgen(p->ex_r, 0, 0);
			emit1(O_ADD);
			break;
		case UNARY '[':
			vp = builtin("table");
			emit(2, O_GVAL, constant(*vp));
			emit(2, O_CALL, 0);
			exprgen(p->ex_l, 0, 0);
			break;
		case BINARY '+':
			emit1(O_DUP);
			exprgen(p->ex_l, 0, 0);
			emit1(O_IDXL);
			exprgen(p->ex_r, 0, 0);
			emit1(O_ASGN);
			emit1(O_POP);
			break;
		default:
			err("exprgen: undefined operator");
		}
	freenode(p->ex_l);
	freenode(p->ex_r);
	p->ex_result++;
}

/* constant - allocate storage for constant v, return virtual address */
int constant(v)
struct value v;
{
	int a;
	struct value *vp;
	
	if (cnstp)
		vp = (struct value *) getblk(cnstp);
	else
		vp = (struct value *) ((struct data *) balloc(B_DATA))->d_vals;
	a = virtaddr(vp);
	*vp = v;
	putblk(vp, 1);
	cnstp = (vp+2 <= ((struct value *)BLKTOP(vp)+1))?virtaddr(++vp):0;
	return (a);
}

/* lookup - lookup name id, return virtual address of value */
int lookup(id)
char *id;
{
	int n, a;
	struct value v, *vp, tv;

	if (strcmp(id, "root") == 0) /* temporary hack */
		return (virtaddr(&rp->r_wglobals));
	v = stralloc(id, -1);
	if (c.proc && TYPE(c.proc->p_sym) == T_TABLE)
		tv = c.proc->p_sym;
	else
		tv = rp->r_wglobals;
	for (n = 0; TYPE(tv) == T_TABLE; n++) {
		if (vp = tindex(tv, v, 0))
			break;
		if (n < 10 && (vp = tindex(tv, rp->r_dotdot, 0))) {
			tv = *vp;
			putblk(vp, 0);
			}
		else
			tv = VOID;
		}
	if (TYPE(tv) != T_TABLE)
		vp = tindex(rp->r_wglobals, v, 1);
	a = virtaddr(vp);
	putblk(vp, 0);
	return (a);
}

/* freenode - add p to list of free nodes if --ex_count <= 0 */
freenode(p)
struct node *p;
{
	if (p && --p->ex_count <= 0) {
		p->ex_l = freelist;
		freelist = p;
		}
}
