#include <stdlib.h>
main() {
	void *p;

	p = realloc(NULL, 100);
	p = realloc(p, 200);
	free(p);
	p = realloc(p, 50);
	p = realloc(&p, 200);
	p = malloc(0);
	p = realloc(NULL, 0);
	p = malloc(-1);
	p = calloc(2, 2147483648);
}
