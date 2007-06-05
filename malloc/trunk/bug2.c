#include <stdlib.h>
void *f(int i) { return i > 0 ? f(i - 1) : malloc(10); }
main() {
	void *p = f(3);
	double d;

	free(p);
	free(p);
	free(0);
	free((void*)1);
	free(&d);
	f(2);
}
