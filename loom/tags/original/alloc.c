
/* alloc - allocate space of n objects of size bytes or issue error message */
char *alloc(n, size)
int n, size;
{
	char *p, *calloc();

	if (p = calloc(n, size))
		return p;
	error("storage overflow", (char *)0);
	return 0;
}

