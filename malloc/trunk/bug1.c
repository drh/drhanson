#include <stdlib.h>
void g(int n) { malloc(n); }
void f(int i) { i > 0 ? f(i - 1) : g(16); }
main(void) { f(7); }
