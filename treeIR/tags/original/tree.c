/* Tree IR node constructors & misc. functions */

#include "tree.h"

struct nodeinfo nodeops[] = {
	"", 0,
#define xx(a,b,c) #a,c,
#include "tree.h"
};

Temp newTemp(int size) {
	Temp t = (Temp) malloc(sizeof *t);
	assert(t);
	assert(size > 0);
	t->size = size;
	t->number = 0;
	return t;
}

Tree newNode(Opcode op) {
	static struct tree z;
	Tree t = (Tree) malloc(sizeof *t);
	assert(t);
	*t = z;
	t->op = op;
	return t;
}

Tree tSEQ(Tree stm1, Tree stm2) {
	assert(stm1 || stm2);
	if (stm1 && stm2) {
		Tree t = newNode(oSEQ);
		assert(HasAttr(stm1,ATTstm|ATTexp));
		assert(HasAttr(stm2,ATTstm|ATTexp));
		t->u.child[0] = stm1;
		t->u.child[1] = stm2;
		return t;
	}
	if (stm1) {
		assert(HasAttr(stm1,ATTstm|ATTexp));
		return stm1;
	}
	assert(HasAttr(stm2,ATTstm|ATTexp));
	return stm2;
}

Tree tLABEL(Label label) {
	Tree t = newNode(oLABEL);
	t->u.label = label;
	return t;
}

Tree tJUMP(Tree exp) {
	Tree t = newNode(oJUMP);
	assert(HasAttr(exp,ATTexp));
	t->u.child[0] = exp;
	return t;
}

Tree tCJUMP(Tree test, Tree exp) {
	Tree t = newNode(oCJUMP);
	assert(HasAttr(test,ATTtest));
	assert(HasAttr(exp,ATTexp));
	t->u.child[0] = test;
	t->u.child[1] = exp;
	return t;
}

Tree tOP(int size, Opcode binop, Tree exp1, Tree exp2) {
	Tree t = newNode(binop);
	assert(Attr(binop)&ATTbinop);
	assert(HasAttr(exp1,ATTexp));
	assert(HasAttr(exp2,ATTexp));
	assert(size == exp1->size && size == exp2->size);
	t->size = size;
	t->u.child[0] = exp1;
	t->u.child[1] = exp2;
	return t;
}

Tree tUNOP(int size, Opcode unop, Tree exp) {
	Tree t = newNode(unop);
	assert(Attr(unop)&ATTunop);
	assert(HasAttr(exp,ATTexp));
	assert(size == exp->size);
	t->size = size;
	t->u.child[0] = exp;
	return t;
}

Tree tCONVERT(int size, Opcode cvtop, Tree exp) {
	Tree t = newNode(cvtop);
	assert(Attr(cvtop)&ATTcvtop);
	assert(HasAttr(exp,ATTexp));
	assert(size > 0);
	t->size = size;
	t->u.child[0] = exp;
	return t;
}

Tree tREL(Opcode relop, Tree exp1, Tree exp2) {
	Tree t = newNode(relop);
	assert(Attr(relop)&ATTrelop);
	assert(HasAttr(exp1,ATTexp));
	assert(HasAttr(exp2,ATTexp));
	t->size = 1;
	t->u.child[0] = exp1;
	t->u.child[1] = exp2;
	return t;
}

Tree tMEM(int size, Tree exp) {
	Tree t = newNode(oMEM);
	assert(HasAttr(exp,ATTexp));
	assert(size >= 0);
	t->size = size;
	t->u.child[0] = exp;
	return t;
}

Tree tMOVE(Tree exp1, Tree exp2) {
	Tree t = newNode(oMOVE);
	assert(HasAttr(exp1,ATTexp));
	assert(exp1->op == oMEM || exp1->op == oTEMP);
	assert(HasAttr(exp2,ATTexp));
	assert(exp1->size == exp2->size);
	t->size = exp2->size;
	t->u.child[0] = exp1;
	t->u.child[1] = exp2;
	return t;
}

Tree tESEQ(Tree stm, Tree exp) {
	Tree t = newNode(oESEQ);
	assert(HasAttr(stm,ATTstm|ATTexp));
	assert(HasAttr(exp,ATTexp));
	t->size = exp->size;
	t->u.child[0] = stm;
	t->u.child[1] = exp;
	return t;
}

Tree tNAME(int size, Label label) {
	Tree t = newNode(oNAME);
	assert(size > 0);
	t->size = size;
	t->u.label = label;
	return t;
}

Tree tCONST(int size, int val) {
	Tree t = newNode(oCONST);
	assert(size == 1 || size == 2 || size == 4);
	t->size = size;
	t->u.ivalue = val;
	return t;
}

Tree tCONSTF(int size, double val) {
	Tree t = newNode(oCONSTF);
	assert(size == 4 || size == 8);
	t->size = size;
	t->u.fvalue = val;
	return t;
}

Tree tALLOC(Temp temp, Tree exp) {
	Tree t = newNode(oALLOC);
	assert(HasAttr(exp,ATTexp));
	t->size = exp->size;
	t->u.child[0] = tTEMP(temp);
	t->u.child[1] = exp;
	return t;
}

Tree tTEMP(Temp temp) {
	Tree t = newNode(oTEMP);
	t->size = temp->size;
	t->u.temp = temp;
	return t;
}

Tree tCALL(int size, Tree args, Tree exp) {
	Tree t = newNode(oCALL);
	assert(HasAttr(args,ATTargs));
	assert(HasAttr(exp,ATTexp));
	assert(size >= 0);
	t->size = size;
	t->u.child[0] = args;
	t->u.child[1] = exp;
	return t;
}

Tree tARG(Tree exp, Tree args) {
	Tree t = newNode(oARG);
	assert(HasAttr(exp,ATTexp));
	assert(HasAttr(args,ATTargs));
	t->size = exp->size + args->size;
	t->u.child[0] = exp;
	t->u.child[1] = args;
	return t;
}

Tree tNOARGS(void) {
	return newNode(oNOARGS);
}

Label newLabel(void) {
	static int labelcount = 0;
	char *s, buf[8];

	sprintf(buf, "L%d", ++labelcount);
	s = malloc(strlen(buf) + 1);
	assert(s);
	return strcpy(s, buf);
}

static void printnode(Tree t, int depth, FILE *fp) {
	for ( ; depth > 0; depth--)
		putc(' ', fp);
	fprintf(fp, "%s", nodeops[t->op].s);
	if (HasAttr(t,ATTsize))
		fprintf(fp, " size=%d", t->size);
	if (HasAttr(t,ATTlabel))
		fprintf(fp, " label=%s", t->u.label ? t->u.label : "(null)");
	if (HasAttr(t,ATTivalue))
		fprintf(fp, " ivalue=%d", t->u.ivalue);
 	if (HasAttr(t,ATTfvalue))
		fprintf(fp, " fvalue=%g", t->u.fvalue);
	if (HasAttr(t,ATTtemp))
		fprintf(fp, " temp=[number=%d size=%d]",
			t->u.temp->number, t->u.temp->size);
	putc('\n', fp);
}

static void treeprint(Tree t, int depth, FILE *fp) {
	if (t) {
		int i;
		if (t->op == oSEQ)
			depth = 0;
		printnode(t, depth, fp);
		if (t->op == oESEQ)
			depth--;
		if (HasAttr(t,ATTchild))
			for(i = 0; i < sizeof t->u.child/sizeof t->u.child[0] && t->u.child[i]; i++)
				treeprint(t->u.child[i], depth + 1, fp);
	}
}

void freeTree(Tree t) {
	if (t) {
		int i;
		if (HasAttr(t,ATTchild))
			for(i = 0; i < sizeof t->u.child/sizeof t->u.child[0]; i++)
				freeTree(t->u.child[i]);
		free(t);
	}
}

void printTree(Tree t, FILE *fp) {
	treeprint(t, 0, fp ? fp : stderr);
}
