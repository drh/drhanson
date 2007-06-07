/* loom  [ -f cmd | -I dir | file ]... */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAXLINE 200		/* line length */

char *directories[20];		/* directories */
char *filter;			/* default filter */
int filesize;			/* size of the current file */
char *progname;

void filt(char *, char *, char *);
void find(char *, char *, char *);
char *getfile(char *name);
int openfile(char *);
void process(FILE *, char *);
char *strind(char *, char *);
void usage(void);

extern int close(int);
extern int open(char *, int);
extern int read(int ,char *, int);
extern int fstat(int, struct stat *);

main(int argc, char *argv[]) {
	int i, nf = 0, nd = 0;
	FILE *fp = stdin;

	progname = argv[0];
	for (i = 1; i < argc; i++)
		if (strncmp(argv[i], "-I", 2) == 0) {
			if (argv[i][2])
				directories[nd++] = &argv[i][2];
			else if (i + 1 < argc)
				directories[nd++] = argv[++i];
			else
				usage();
			if (nd >= sizeof(directories)/sizeof(char *)) {
				fprintf(stderr, "%s: too many directories\n",
					progname);
				directories[--nd] = 0;
				}
		} else if (strcmp(argv[i], "-f") == 0)
			if (i + 1 < argc)
				filter = argv[++i];
			else
				usage();
		else if (strcmp(argv[i], "-") == 0) {
			process(stdin, filter);
			nf++;
		} else if (fp = fopen(argv[i], "r")) {
			process(fp, filter);
			fclose(fp);
			nf++;
		} else
			fprintf(stderr, "%s: can't open %s\n", progname,
				argv[i]);
	if (nf == 0)
		process(stdin, filter);
	exit(0);
}

/* filt - filter text from *s thru *t using cmd, write result to stdout */
void filt(char *s, char *t, char *cmd) {
	FILE *pfp;

	if (cmd == 0 || *cmd == '\0')
		while (s < t)
			putchar(*s++);
	else if (pfp = popen(cmd, "w")) {
		fflush(stdout);
		while (s < t)
			putc(*s++, pfp);
		pclose(pfp);
	} else
		fprintf(stderr, "%s: cannot execute %s\n", progname, cmd);
}

/* getfile - read file name, if necessary, return pointer to buffer */
char *getfile(char *name) {
	static char *buffer = 0, file[MAXLINE] = "";
	static int bufsize = -1;
	struct stat statbuf;
	int n, fd;

	if (strcmp(name, file) == 0 && buffer)
		return buffer;
	/* need a new file */
	if ((fd = openfile(strcpy(file, name))) < 0) {
		fprintf(stderr, "%s: can't find %s\n", progname,name);
		file[0] = '\0';
		return 0;
	}
	if (fstat(fd, &statbuf) < 0) {
		fprintf(stderr, "%s: can't find size of %s\n",
			progname, file);
		close(fd);
		file[0] = '\0';
		return 0;
	}
	filesize = statbuf.st_size;
	if (statbuf.st_size > bufsize) {
		if (buffer)
			buffer = realloc(buffer, filesize + 2);
		else
			buffer = malloc(filesize + 2);
		buffer[0] = '\n';
		bufsize = filesize + 1;
	}
	if (buffer == 0) {
		fprintf(stderr, "%s: can't allocate space for %s\n",
			progname, file);
		close(fd);
		file[0] = '\0';
		return 0;
	}
	n = read(fd, buffer + 1, filesize);
	close(fd);
	if (n < filesize) {
		fprintf(stderr, "%s: can't read %s\n", progname, file);
		return 0;
	}
	buffer[n+1] = '\0';
	return buffer;
}

/* find - find section sec in file name, filter it through cmd */
void find(char *sec, char *name, char *cmd) {
	int i;
	char *buffer, buf[MAXLINE], *s, *t;

	if ((buffer = getfile(name)) == 0)
		return;
	if (sec == 0 || strcmp(sec, "-") == 0) {	/* filter whole file */
		filt(buffer + 1, &buffer[filesize+1], cmd);
		return;
	}
	i = strlen(sec);
	s = buffer + 1;
again:
	for (; s = strind(s, sec); s += i)
		if (s[i] == ' ' && strncmp(s - 11, "/* include ", 11) == 0) {
			sprintf(buf, "/* end %s */", sec);
			break;
		} else if (s[i] == ' ' && strncmp(s - 4, "\n/* ", 4) == 0
		&& s[i+1] == '-') {
			strcpy(buf, "\n}\n");
			break;
		} else if (s[i] == '}' && strncmp(s - 9, "{include ", 9) == 0) {
			sprintf(buf, "{end %s}", sec);
			break;
		}
	if (s) {
		while (*s != '\n')
			s++;
		if (t = strind(s, buf)) {
			if (buf[0] == '\n')
				t += strlen(buf);
			filt(++s, t, cmd);
			s = t;
			goto again;
		} else
			fprintf(stderr, "%s: section %s does not terminate\n",
				progname, sec);
	}
}

/* openfile - open file name, looking in all possible directories */
int openfile(char *name) {
	int fd;
	char buf[MAXLINE], **p;

	if (*name != '/')
		for (p = directories; *p; p++) {
			sprintf(buf, "%s/%s", *p, name);
			if ((fd = open(buf, 0)) >= 0)
				return fd;
		}
	return open(name, 0);
}

/* process - process file open on fp, using default filter cmd */
void process(FILE *fp, char *cmd) {
	char buf[MAXLINE], *s, *sec, *file;

	while (fgets(buf, MAXLINE, fp)) 
		if (*buf == '%' && strncmp(buf, "%include", 8) == 0) {
			s = buf + sizeof("%include") - 1;
			while (*s && isspace(*s))
				s++;
			for (file = s; *s && !isspace(*s); s++)
				;
			*s++ = '\0';
			while (*s && isspace(*s))
				s++;
			if (*s && *s != '|') {	/* section name */
				for (sec = s; *s && !isspace(*s); s++)
					;
				*s++ = '\0';
			} else
				sec = 0;
			while (*s && isspace(*s))
				s++;
			if (*s == '|')
				s++;
			find(sec, file, *s ? s : cmd);
		} else
			fputs(buf, stdout);
}

/* strind - return pointer to leftmost t in s, or 0 if none */
char *strind(char s[], char t[]) {
	int i, j, k;

	for (i = 0; s[i] != '\0'; i++) {
		for (j=i, k=0; t[k] && s[j] == t[k]; j++, k++)
			;
		if (t[k] == '\0')
			return &s[i];
	}
	return 0;
}

/* usage - print usage message and quit */
void usage() {
	fprintf(stderr, "usage: %s [ -f cmd | -I dir | file ]...\n",
		progname);
	exit(1);
}
