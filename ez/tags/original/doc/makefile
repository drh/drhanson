TROFF=ditroff
TFLAGS=
Refman:
	bib -p /u/drh/refs/INDEX refman* | tbl | $(TROFF) -mtr $(TFLAGS)

TR84-2:
	bib -p /u/drh/refs/INDEX tr84-2 | tbl | $(TROFF) -mtr $(TFLAGS)

Popl:
	bib -p /u/drh/refs/INDEX popl | $(TROFF) -mtr $(TFLAGS)

ez:
	$(TROFF) -man ez.1
