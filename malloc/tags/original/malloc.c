#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "memmon.h"

static struct block {	/* block descriptor: */
	struct block *free;	/* next block on the free list */
	struct block *link;	/* next block on the hash chain */
	void *ptr;		/* pointer to the allocated/free block */
	size_t size;		/* size of the block in bytes */
} *htab[1024];
#define hash(a) ((unsigned)a>>3)
#define NELEMS(a) ((int)(sizeof (a)/sizeof ((a)[0])))
#define NALLOC	4096		/* minimum #bytes to request from system */

static struct block freelist = { &freelist };	/* list of free blocks */
static union align { void (*f)(void); double d; long l; } align; /*lint -e551 */

extern void *sbrk(int);
#include "trace.c"

/* find - find the block descriptor for the block at address ptr */
static struct block *find(void *ptr) {
	struct block *bp = htab[hash(ptr)%NELEMS(htab)];

	while (bp && bp->ptr != ptr)
		bp = bp->link;
	return bp;
}

/* _free - internal version of free */
static void _free(void *ptr) {
	struct block *bp;

	if (((unsigned)ptr&(sizeof align - 1)) == 0
	&& (bp = find(ptr)) != NULL	/* free'ing unallocated memory? */
	&&  bp->free == NULL) {		/* free'ing free memory? */
		bp->free = freelist.free;	/* free a valid block */
		freelist.free = bp;
	}
}

/* free - free block at address ptr */
void free(void *ptr) {
	Memmon_T msg = { Memmon_free };

	if ((msg.ptr[0] = ptr) != NULL)
		_free(ptr);
	msg.opcode = Memmon_free;
	send(&msg, _flush());
}

/* block - allocate and initialize a block descriptor */
static struct block *block(void *ptr, unsigned size) {
	static struct block *avail;
	static int nleft = 0;

	if (nleft <= 0) {
		if ((avail = sbrk(512*sizeof *avail)) == (void *)-1)
			return NULL;
		nleft = 512;
	}
	avail->ptr = ptr;
	avail->size = size;
	avail->free = avail->link = NULL;
	nleft--;
	return avail++;
}

/* _malloc - internal version of malloc */
void *_malloc(size_t size) {
	struct block *bp, *new;
	void *ptr;

	if (size > INT_MAX - NALLOC)
		return NULL;
	if (size == 0)
		size = 1;
	if (size%sizeof align)
		size += sizeof align - size%sizeof align;
	for (bp = freelist.free; bp; bp = bp->free) {
		if (bp->size > size) {	/* big enough? */
			bp->size -= size;	/* allocate tail end */
			ptr = (char *)bp->ptr + bp->size;
			if ((bp = block(ptr, size)) != NULL) {
				unsigned h = hash(ptr)%NELEMS(htab);
				bp->link = htab[h];
				htab[h] = bp;
				return ptr;
			} else
				return NULL;
		}
		if (bp == &freelist) {
			if ((ptr = sbrk(size + NALLOC)) == (void *)-1)
				return NULL;
			if ((new = block(ptr, size + NALLOC)) == NULL)
				return NULL;
			new->free = freelist.free;
			freelist.free = new;
		}
	}
	/*lint -e506 */ assert(0);
	return NULL;
}

void *malloc(size_t size) {
	Memmon_T msg = { Memmon_malloc };

	msg.size[0] = size;
	msg.ptr[1] = _malloc(size);
	send(&msg, _flush());
	return msg.ptr[1];
}

void *calloc(size_t nobj, size_t size) {
	Memmon_T msg = { Memmon_calloc };

	if (nobj > 0 && size > UINT_MAX/nobj)
		msg.ptr[1] = NULL;
	else if ((msg.ptr[1] = _malloc(nobj*size)) != NULL)
		memset(msg.ptr[1], 0, nobj*size);
	msg.size[1] = nobj;
	msg.size[0] = size;
	send(&msg, _flush());
	return msg.ptr[1];
}

/* realloc - reallocate the block at ptr to be size bytes */
void *realloc(void *ptr, size_t size) {
	struct block *bp;
	void *new = NULL;
	Memmon_T msg = { Memmon_realloc };

	msg.ptr[0] = ptr;
	msg.size[0] = size;
	if (ptr == NULL)
		new = _malloc(size);
	else if (ptr && size == 0)
		_free(ptr);
	else if ((new = _malloc(size)) != NULL
	&& (bp = find(ptr)) != NULL && bp->free == NULL) {
		memcpy(new, ptr, size < bp->size ? size : bp->size);
		_free(ptr);
	}
	msg.ptr[1] = new;
	send(&msg, _flush());
	return new;	
}
