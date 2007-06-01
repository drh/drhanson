CC=lcc -A
MEMMON=
CFLAGS=-g -DMEMMON='"$(MEMMON)"'
LDFLAGS=-g
OBJS=malloc.o flush.o
FILES=makefile malloc.c trace.c flush.s memmon.c memmon.h sym.h sym.c

libmalloc.a:	$(OBJS)
		ar r $@ $?
		ranlib $@

malloc.o:	memmon.h trace.c

flush.o:	flush.s
		$(CC) -c flush.s

lint:		malloc.c trace.c memmon.c sym.c sym.h
		-lint -I. -u -w2 -e534 malloc.c
		-lint -I. -w2 '-esym(526,read,sbrk,unlink)' memmon.c sym.c

memmon:		memmon.o sym.o
		$(CC) -o $@ $(LDFLAGS) memmon.o sym.o

sym.o:		sym.h
memmon.o:	sym.h memmon.h

malloc.tar.gz:
		tar cf - README $(FILES) memmon.1 index.ps | gzip -c >$@

clean:
		rm -f *.o *.out core a.out *.sym

clobber:	clean
		rm -f libmalloc.a memmon
