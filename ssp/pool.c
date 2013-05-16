#include <stdlib.h>
#include "pool.h"

/*
 * Simple pool memory allocator.
 * Memory is allocated from large blocks of store
 * and released in one hit, at the very end.
 * Allocations grow exponentially.
 */

#define INITIAL_BLOCK_SIZE	8192

struct block {
	struct block *next;
	char *p;	/* Pointer into block storage */
	int avail;	/* bytes available after p */
};

struct pool {
	struct block *blocks;
	int next_alloc;	/* size of next block */
};

/* Creates a new memory pool */
struct pool *
pool_new()
{
	struct pool *pool;

	pool = (struct pool *)malloc(sizeof (struct pool));
	if (pool) {
		pool->blocks = NULL;
		pool->next_alloc = INITIAL_BLOCK_SIZE;
	}
	return pool;
}

/* Destroys a memory pool */
void
pool_destroy(pool)
	struct pool *pool;
{
	struct block *block;

	while (pool->blocks) {
		block = pool->blocks;
		pool->blocks = block->next;
		free(block);
	}
	free(pool);
}

/* Allocates from a memory pool */
void *
pool_malloc(pool, size)
	struct pool *pool;
	size_t size;
{
	int spc;
	struct block *block;
	char *ptr;

	/* Round size up to align to nearest ptr */
	spc = (size - 1 + sizeof (void *)) & ~(sizeof (void *) - 1);

  	for (;;) {
		/* Search blocks for enough space */
		for (block = pool->blocks; block; block = block->next)
		    if (block->avail >= spc) {
			ptr = block->p;
			block->p += spc;
			block->avail -= spc;
			return ptr;
		    }

		/* No block had enough space. Make more blocks! */
		while (pool->next_alloc < spc + sizeof (struct block))
			pool->next_alloc *= 2;

		block = (struct block *)malloc(pool->next_alloc);
		if (!block)
			return NULL;
		block->p = (char *)(block + 1);
		block->avail = pool->next_alloc - sizeof (struct block);
		block->next = pool->blocks;
		pool->blocks = block;
		pool->next_alloc *= 2;
	}
}
