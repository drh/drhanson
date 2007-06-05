CFLAGS=-g
LDFLAGS=-g
OBJS=malloc.o

libmalloc.a:	$(OBJS)
		ar r $@ $?
		ranlib $@

malloc.o:	memmon.h trace.c

memmon:		memmon.o sym.o
		$(CC) -o $@ $(LDFLAGS) memmon.o sym.o

sym.o:		sym.h
memmon.o:	sym.h memmon.h

clean:
		-rm -f *.o *.out core a.out *.sym

clobber:	clean
		-rm -f libmalloc.a memmon
