CC=cc
CFLAGS=-g
AR=ar
OBJS=tree.o

libtree.a:	$(OBJS) makefile
		$(AR) -r $@ $(OBJS)

clean:
		-rm $(OBJS)

clobber:	clean
		-rm libtree.a

		