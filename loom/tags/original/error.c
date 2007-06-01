#include <stdio.h>

error(s1, s2, s3)	/* print error message and die */
	char *s1, *s2, *s3;
{
	warn(s1, s2, s3);
	exit(1);
}

warn(s1, s2, s3)	/* print error message */
	char *s1, *s2, *s3;
{
	extern int errno, sys_nerr;
	extern char *sys_errlist[], *progname;

	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, s1, s2, s3);
	if (errno > 0 && errno < sys_nerr)
		fprintf(stderr, "; %s", sys_errlist[errno]);
	fprintf(stderr, "\n");
}
