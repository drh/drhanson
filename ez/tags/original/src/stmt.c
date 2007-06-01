/* ez compiler: statement parsing */

#include <stdio.h>
#include "ez.h"
#include "tokens.h"

int first[] = {IF, WHILE, FOR, RETURN, BREAK, CONTINUE, '{', ';', 0};
int efirst[] = {'-', '+', ID, CON, BCON, FCON, SCON, '(', '[', PROCEDURE, 0};
static int follow[] = {EOF, '\n', END, '{', ';', '}', ELSE, 0};

extern struct node *expr();
extern char tok[];

/* stmt	: expr
 *	| if `(' expr `)' stmt [ else stmt ]
 *	| while `(' expr `)' stmt
 *	| for `(' [ expr ] ; [ expr ] ; [ expr ] `)' stmt
 *	| for ( expr in expr ) stmt
 *	| return [ expr ]
 *	| break
 *	| continue
 *	| `{' { stmt } `}'
 *	| ;
 *
 * parse a single statement
 */
stmt(n)
int n;
{
	int loopsav, pt, pt1;
	char *s;
	struct value *vp;
	struct proc *f;
	struct node *p, *expr(), *term(), *rvalue(), *enode(), *ident();

	loopsav = c.loop;
	pt = 0;
	if (t != ';' && t != '{')
		pt = begpoint();
	switch (t) {
		case PROCEDURE:
			p = term();
			if (p->ex_op == O_GVAL) {
				vp = (struct value *) getblk(p->ex_l);
				f = (struct proc *) getblk(vp->v_addr);
				if (s = getstr(f->p_name)) {
					p = enode(O_ASGN, ident(s), p);
					free(s);
					}
				putblk(f, 0);
				putblk(vp, 0);
				}
			exprgen(p, 0, 0);
			freenode(p);
			if (n)
				emit1(O_BASE);
			break;
		case '-': case '+': case ID: case CON: case BCON:
		case FCON: case SCON: case '(': case '[':
			exprgen(rvalue(expr()), 0, 0);
			if (n)
				emit1(O_BASE);
			break;
		case IF:
			ifstat(genlabel(2));
			break;
		case WHILE:
			whilestat(c.loop = genlabel(3));
			break;
		case FOR:
			forstat(c.loop = genlabel(3));
			break;
		case RETURN:
			t = gtok(0);
			pt1 = begpoint();
			if (inset(t, efirst))
				exprgen(rvalue(expr()), 0, 0);
			else
				emit(2, O_GVAL, const(VOID));
			endpoint(pt1);
			if (c.forlevel)
				emit(2, O_QUIT, c.forlevel + 1);
			else
				emit1(O_RET);
			break;
		case BREAK:
		case CONTINUE:
			if (c.loop)
				jump(O_JUMP, c.loop + (t == BREAK ? 2 : 1));
			else
				error("illegal ", tok);
			t = gtok(0);
			break;
		case '{':
			t = gtok('\n');
			stmtlist(1);
			mustbe('}', 0);
			break;
		case ';':
			t = gtok(0);
			return;		/* avoid follow check */
		default:
			error("unrecognized statement", "");
			t = gtok(0);
			skipto(follow, 0);
		}
	if (inset(t, follow) == 0) {
		error("illegal statement termination", "");
		skipto(follow, 0);
		}
	if (pt)
		endpoint(pt);
	c.loop = loopsav;
}

/* ifstat - if `(' expr `)' stmt [ else stmt ] */
ifstat(lab)
int lab;
{
	int pt;

	t = gtok('\n');
	mustbe('(', '\n');
	pt = begpoint();
	exprgen(expr(), 0, lab);
	endpoint(pt);
	mustbe(')', '\n');
	stmt(1);
	if (t == '\n') {
		t = gtok('\n');
		if (t != ELSE && t != EOF) {
			pbstr(tok);
			t = '\n';
			}
		}
	if (t == ELSE) {
		t = gtok('\n');
		jump(O_JUMP, lab + 1);
		deflabel(lab);
		stmt(1);
		}
	else
		deflabel(lab);
	deflabel(lab + 1);
}

/* whilestat - while `(' expr `)' stmt */
whilestat(lab)
int lab;
{
	int pt;

	t = gtok('\n');
	mustbe('(', '\n');
	deflabel(lab);
	deflabel(lab + 1);
	pt = begpoint();
	exprgen(expr(), 0, lab + 2);
	endpoint(pt);
	mustbe(')', '\n');
	stmt(1);
	jump(O_JUMP, lab);
	deflabel(lab + 2);
}

/* forstat - for `(' [ expr ] ; [ expr ] ; [ expr ] `)' stmt
 *	     for `(' id in expr `)' stmt	*/
forstat(lab)
int lab;
{
	int pt;
	struct node *p, *expr1(), *node(), *lvalue();

	t = gtok('\n');
	mustbe('(', '\n');
	pt = begpoint();
	if (t != ';') {
		p = expr();
		if (t == IN) {
			endpoint(pt);
			c.forlevel++;
			t = gtok('\n');
			pt = begpoint();
			exprgen(rvalue(expr()), 0, 0);
			exprgen(lvalue(p), 0, 0);
			jump(O_ITER, lab);
			endpoint(pt);
			mustbe(')', '\n');
			stmt(1);
			deflabel(lab + 1);
			emit(2, O_QUIT, 0);
			deflabel(lab + 2);
			emit(2, O_QUIT, 1);
			deflabel(lab);
			c.forlevel--;
			return;
			}
		exprgen(p, 0, 0);
		emit1(O_POP);
		}
	endpoint(pt);
	mustbe(';', '\n');
	deflabel(lab);
	pt = begpoint();
	if (t != ';')
		exprgen(expr(), 0, lab + 2);
	endpoint(pt);
	mustbe(';', '\n');
	pt = begpoint();
	if (t != ')')
		p = expr();
	else
		p = NULL;
	endpoint(pt);
	mustbe(')', '\n');
	stmt(1);
	deflabel(lab + 1);
	if (p) {
		exprgen(p, 0, 0);
		emit1(O_POP);
		}
	jump(O_JUMP, lab);
	deflabel(lab + 2);
}

/* stmtlist - { stmt } */
stmtlist(lev)
int lev;
{
	int t1;

	for (t1 = 0; inset(t, first) || inset(t, efirst); ) {
		if (inset(t1, efirst))
			emit1(O_BASE);
		t1 = t;
		stmt(0);
		newline();
		}
	if (lev > 0 && inset(t1, efirst))
		emit1(O_BASE);
}
