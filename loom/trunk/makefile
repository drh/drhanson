CFLAGS=-g -DFAST

loom:		loom.o
		cc $(CFLAGS) -o loom loom.o

common:		common.o
		cc $(CFLAGS) -o common common.o
		
common.dvi:	common.tex index.tex
		tex common

index.tex:	common.tex
		rm -f index.dat index.tex
		tex common
		makeindex index.dat >index.tex
		rm common.dvi

common.tex:	common.c
		cc -E -C -DTeXinput common.c | sed '/^#/d' | \
		  loom -I . -f "prog|xref +buf +k -t" >common.tex

clean:
		rm -f common common.dvi a.out loom *.o index.dat *.log
