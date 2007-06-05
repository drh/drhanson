#ifndef MEMMON_INCLUDED
#define MEMMON_INCLUDED

#define T Memmon_T

typedef struct T {	/* memmon messages: */
	enum {
		Memmon_free,
		Memmon_malloc,
		Memmon_calloc,
		Memmon_realloc
	} opcode;		/* allocation function code */
	void *ptr[2];		/* input address, output address */
	unsigned size[2];	/* input sizes in bytes */
	void *calls[10];	/* associated call stack */
} T;

/*
The fields hold the following values. Omitted values must be transmitted,
but memmon ignores them.

client's call		opcode		ptr[0]	ptr[1]	size[0]	size[1]
free(ptr)		Memmon_free	ptr
ptr=malloc(n)		Memmon_malloc		ptr	n
ptr=calloc(m,n)		Memmon_calloc		ptr	n	m
new=realloc(ptr,n)	Memmon_realloc	ptr	new	n

calls[] always holds up to 10 return addresses from the point
of call to the allocation function.
*/
#undef T
#endif
