CFLAGS=-g
FILES=ez.h tokens.h cache.c code.c cvt.c expr.c func.c gen.c lex.c \
main.c parse.c proc.c stmt.c table.c util.c collect.c
OBJECTS=cache.o code.o cvt.o expr.o func.o gen.o \
lex.o main.o parse.o proc.o stmt.o table.o util.o

a.out:		$(OBJECTS) makefile
		cc -g $(OBJECTS)

$(OBJECTS) collect.o:	ez.h

main.o lex.o gen.o parse.o expr.o stmt.o:	tokens.h

ez:		a.out
		cp a.out ez

collect:	collect.o cache.o
		cc -w -g -o collect collect.o cache.o

install:	ez collect
		cp ez collect $(DESTDIR)

clean:
		-rm -f $(OBJECTS) a.out collect.o
		
clobber:	clean
		-rm -f ez collect

