/* Tree IR definitions */

#ifndef _TREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef enum Opcode {
	oZERO,
#define xx(a,b,c) o##a,
#endif

/* name	arity	attributes */	
xx(SEQ,		2, ATTstm|ATTchild)
xx(LABEL,	0, ATTstm|ATTlabel)
xx(JUMP,	1, ATTstm|ATTchild)
xx(CJUMP,	2, ATTstm|ATTchild)
xx(MEM,		1, ATTexp|ATTchild|ATTsize)
xx(MOVE,	2, ATTexp|ATTchild|ATTsize)
xx(ESEQ,	2, ATTexp|ATTchild)
xx(NAME,	0, ATTexp|ATTlabel|ATTsize)
xx(CONST,	0, ATTexp|ATTivalue|ATTsize)
xx(CONSTF,	0, ATTexp|ATTfvalue|ATTsize)
xx(ALLOC,	2, ATTexp|ATTchild|ATTsize)
xx(TEMP,	0, ATTexp|ATTtemp|ATTsize)
xx(CALL,	2, ATTexp|ATTchild|ATTsize)
xx(ARG,		2, ATTargs|ATTchild|ATTsize)
xx(NOARGS,	0, ATTargs)
xx(FPLUS,	2, ATTbinop|ATTsize|ATTchild)
xx(FMINUS,	2, ATTbinop|ATTsize|ATTchild)
xx(FMUL,	2, ATTbinop|ATTsize|ATTchild)
xx(FDIV,	2, ATTbinop|ATTsize|ATTchild)
xx(FNEG,	1, ATTunop|ATTsize|ATTchild)
xx(CVTSU,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTSS,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTSF,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTUU,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTUS,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTFS,	1, ATTcvtop|ATTsize|ATTchild)
xx(CVTFF,	1, ATTcvtop|ATTsize|ATTchild)
xx(PLUS,	2, ATTbinop|ATTsize|ATTchild)
xx(MINUS,	2, ATTbinop|ATTsize|ATTchild)
xx(MUL,		2, ATTbinop|ATTsize|ATTchild)
xx(DIV,		2, ATTbinop|ATTsize|ATTchild)
xx(MOD,		2, ATTbinop|ATTsize|ATTchild)
xx(AND,		2, ATTbinop|ATTsize|ATTchild)
xx(OR,		2, ATTbinop|ATTsize|ATTchild)
xx(LSHIFT,	2, ATTbinop|ATTsize|ATTchild)
xx(RSHIFT,	2, ATTbinop|ATTsize|ATTchild)
xx(XOR,		2, ATTbinop|ATTsize|ATTchild)
xx(NEG,		1, ATTunop|ATTsize|ATTchild)
xx(COMP,	1, ATTunop|ATTsize|ATTchild)
xx(EQ,		2, ATTrelop|ATTchild)
xx(NE,		2, ATTrelop|ATTchild)
xx(LT,		2, ATTrelop|ATTchild)
xx(GE,		2, ATTrelop|ATTchild)
xx(GT,		2, ATTrelop|ATTchild)
xx(LE,		2, ATTrelop|ATTchild)
xx(ULT,		2, ATTrelop|ATTchild)
xx(UGE,		2, ATTrelop|ATTchild)
xx(UGT,		2, ATTrelop|ATTchild)
xx(ULE,		2, ATTrelop|ATTchild)
xx(FEQ,		2, ATTrelop|ATTchild)
xx(FNE,		2, ATTrelop|ATTchild)
xx(FLT,		2, ATTrelop|ATTchild)
xx(FLE,		2, ATTrelop|ATTchild)
xx(FGT,		2, ATTrelop|ATTchild)
xx(FGE,		2, ATTrelop|ATTchild)
#undef xx

#ifndef _TREE_H
#define _TREE_H
	oLAST
} Opcode;

#define bit(n) (1<<(n))
typedef enum Attribute {
	ATTsize=bit(0),
	ATTchild=bit(1),
	ATTlabel=bit(2),
	ATTivalue=bit(3),
	ATTfvalue=bit(4),
	ATTtemp=bit(5),
	ATTexp=bit(6),
	ATTtest=bit(7),
	ATTstm=bit(8),
	ATTargs=bit(9),
	ATTbinop=bit(10)|ATTexp,
	ATTrelop=bit(11)|ATTtest,
	ATTunop=bit(12)|ATTexp,
	ATTcvtop=bit(13)|ATTexp
} Attribute;
#undef bit

#define Attr(op) ((op)>0 && (op)<oLAST?nodeops[op].attributes : 0)
#define HasAttr(n,a) ((n) && Attr((n)->op)&(a))

extern struct nodeinfo {
	char *s;
	int attributes;
} nodeops[];

typedef char *Label;
typedef struct temp {
	int number;
	int size;
} *Temp;

typedef struct tree *Tree;
struct tree {
	Opcode op;
	int size;
	union {
		Tree child[2];
		Label label;
		int ivalue;
		double fvalue;
		Temp temp;
	} u;
#ifdef YYTREE
	YYTREE x;
#endif
};

void	freeTree(Tree t),
	printTree(Tree t, FILE *fp);
Label	newLabel(void);
Temp	newTemp(int size);
Tree	tSEQ(Tree stm1, Tree stm2),
	tLABEL(Label label),
	tJUMP(Tree exp),
	tCJUMP(Tree test, Tree exp),
	tOP(int size, Opcode binop, Tree exp1, Tree exp2),
	tUNOP(int size, Opcode unop, Tree exp),
	tCONVERT(int size, Opcode cvtop, Tree exp),
	tREL(Opcode relop, Tree exp1, Tree exp2),
	tMEM(int size, Tree exp),
	tMOVE(Tree exp1, Tree exp2),
	tESEQ(Tree stm, Tree exp),
	tNAME(int size, Label label),
	tCONST(int size, int val),
	tCONSTF(int size, double val),
	tALLOC(Temp temp, Tree exp),
	tTEMP(Temp temp),
	tCALL(int size, Tree exp, Tree args),
	tARG(Tree exp, Tree args),
	tNOARGS(void);

#endif
