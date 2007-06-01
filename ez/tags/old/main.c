#include "s.h"
#include "y.tab.h"
#include "g.h"

/* main program - parse and execute statements */
main(argc, argv)
int argc;
char *argv[];
{
   FILE *fopen(), *f;
   int i;

   gcoff = debug = trace = ntrace = 0;
   f = stdin;
   for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"-D") == 0)
         debug = 1;
      else if (strcmp(argv[i],"-T") == 0)
         trace = 1;
      else if (argv[i][0] == '-' && argv[i][1] == 't')
         if (argv[i][2])
            ntrace = atoi(&argv[i][2]);
         else
            ntrace = -1;
      else if (strcmp(argv[i], "-G") == 0)
         gcoff = 1;
      else if (strcmp(argv[i],"-") == 0)
         break;
      else if ((f=fopen(argv[i],"r")) == NULL) {
         fprintf(stderr, "s : can't open file %s\n", argv[i]);
         exit(1);
         }
      }
   initalloc();
   initsym();
   initlex(f);
   for (eofflag = 0; eofflag == 0;) {
      startlex();
      if (yyparse() == 0)
         if ((i = setjmp(env)) == 0) {
            if (debug)
               cdump(fstblk);
            xinit();
            interp(fstblk->c_code);
            valprnt();
            }
         else if (i == 2)
            valprnt();
      }
}
