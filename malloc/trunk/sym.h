#ifndef SYM_INCLUDED
#define SYM_INCLUDED

#define T Sym_T
typedef struct T *T;

/*
 * Sym maintains a map of addresses to function names.
 */

extern T Sym_init(const char *file);
/*
 * Sym_init specifies the name of the file from which to extract the symbol
 * table of an executable program. The file must be created by a command like
 *
 *	nm -n a.out | grep '[tT] _' >file
 *
 * where a.out is executable file of interest.
 * 
 * Sym_init returns a handle to the symbol table if the file can be read and
 * it's in the proper format. Otherwise, Sym_init returns NULL. Sym_init
 * allocates space for the symbol table by making system calls; it does not
 * call malloc. It is a checked runtime error to pass a NULL file to Sym_init.
 */
	
extern char *Sym_find(T stab, void *addr);
/*
 * Sym_find searches the symbol table stab and returns a pointer to the name of
 * the function whose body includes the location at addr, or NULL if no
 * function's body in stab includes addr. The name is at most 16 characters long
 * including the terminating null byte. Clients must make a copy of the name
 * before modifying it.
 */

/*
 * It is a checked runtime error to pass a NULL T to any function in this
 * interface.
 */

#undef T
#endif
