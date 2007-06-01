CFLAGS=-c -O
YFLAGS=-d

a.out:		y.tab.o lex.o sem.o sym.o init.o util.o code.o alloc.o \
		table.o libf.o libutil.o main.o
		cc -i y.tab.o lex.o sem.o sym.o init.c util.o code.o \
		alloc.o table.o libf.o libutil.o main.o -ly

y.tab.o:	y.tab.c s.h
		cc $(CFLAGS) y.tab.c
lex.o:		lex.c s.h y.tab.h y.tab.c
		cc $(CFLAGS) lex.c
sem.o:		sem.c s.h
		cc $(CFLAGS) sem.c
sym.o:		sym.c s.h
		cc $(CFLAGS) sym.c
code.o:		code.c s.h
		cc $(CFLAGS) code.c
y.tab.c:	s.g s.h
		yacc $(YFLAGS) s.g
alloc.o:	alloc.c s.h
		cc $(CFLAGS) alloc.c
util.o:		util.c s.h
		cc $(CFLAGS) util.c
table.o:	table.c s.h
		cc $(CFLAGS) table.c
libf.o:		libf.c s.h
		cc $(CFLAGS) libf.c
libutil.o:	libutil.c s.h
		cc $(CFLAGS) libutil.c
main.o:		main.c s.h y.tab.h y.tab.c
		cc $(CFLAGS) main.c
init.o:		init.c s.h
		cc $(CFLAGS) init.c

list:		.printdate
.printdate:	s.h \
		s.g \
		g.h \
		alloc.c \
		code.c \
		lex.c \
		libf.c \
		libutil.c \
		init.c \
		main.c \
		sem.c \
		sym.c \
		table.c \
		util.c
		pr $? | lpr &
