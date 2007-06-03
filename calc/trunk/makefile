CC=cc
CFLAGS=-g
YFLAGS=-d
FILES=calc.h calc.y code.c depend.c func.c gen.c main.c sym.c
OBJECTS=calc.o code.o func.o depend.o gen.o main.o sym.o

a.out:		$(OBJECTS) makefile
		$(CC) -g $(OBJECTS) -lm

$(OBJECTS):	calc.h

code.o gen.o:	x.tab.h

x.tab.h:	y.tab.h
		-cmp -s x.tab.h y.tab.h || cp y.tab.h x.tab.h
clean:
		rm -f $(OBJECTS) a.out *.CKP y.output [xy].tab.*
