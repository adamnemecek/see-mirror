/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: mem.h 1350 2008-02-08 09:53:55Z d $ */

#ifndef _SEE_h_mem_
#define _SEE_h_mem_

#include <see/type.h>

struct SEE_interpreter;

void *	SEE_malloc(struct SEE_interpreter *i, SEE_size_t sz) 
		_SEE__malloc;
void *	SEE_malloc_string(struct SEE_interpreter *i, SEE_size_t sz) 
		_SEE__malloc;
void *	SEE_malloc_finalize(struct SEE_interpreter *i, SEE_size_t sz,
		void (*finalizefn)(struct SEE_interpreter *i, void *p,
			void *closure), void *closure) 
		_SEE__malloc;
void  	SEE_free(struct SEE_interpreter *i, void **memp);
void  	SEE_gcollect(struct SEE_interpreter *i);

/* Debugging variants */
void *	_SEE_malloc_debug(struct SEE_interpreter *i, SEE_size_t sz, 
		const char *file, int line);
void *	_SEE_malloc_string_debug(struct SEE_interpreter *i, SEE_size_t sz, 
		const char *file, int line);
void *	_SEE_malloc_finalize_debug(struct SEE_interpreter *i, SEE_size_t sz,
		void (*finalizefn)(struct SEE_interpreter *i, void *p,
			void *closure), void *closure, 
			const char *file, int line);
void 	_SEE_free_debug(struct SEE_interpreter *i, void **memp,
		const char *file, int line);
#ifndef NDEBUG
#define SEE_malloc(i,s) \
		_SEE_malloc_debug(i,s,__FILE__,__LINE__)
#define SEE_malloc_string(i,s) \
		_SEE_malloc_string_debug(i,s,__FILE__,__LINE__)
#define SEE_malloc_finalize(i,s,f,c) \
		_SEE_malloc_finalize_debug(i,s,f,c,__FILE__,__LINE__)
#define SEE_free(i,p) \
		_SEE_free_debug(i,p,__FILE__,__LINE__)
#endif


struct SEE_growable {
    void **data_ptr;		/* Reference to base pointer */
    unsigned int *length_ptr;	/* Reference to element use count */
    SEE_size_t element_size;	/* Size of an element */
    SEE_size_t allocated;	/* Bytes of storage addressed by *data_ptr */
    unsigned int is_string : 1;	/* Use SEE_malloc_string */
};

/* Sets the new length of a growable array */
void	SEE_grow_to(struct SEE_interpreter *i, struct SEE_growable *grow,
		    unsigned int new_len);
#define SEE_GROW_INIT(i,g,ptr,len) do {			\
	(ptr) = 0; (len) = 0;				\
	(g)->data_ptr = (void **)&(ptr);		\
	(g)->length_ptr = &(len);			\
	(g)->element_size = sizeof (ptr)[0];		\
	(g)->allocated = 0;				\
	(g)->is_string = 0;				\
    } while (0)
void	_SEE_grow_to_debug(struct SEE_interpreter *i, 
		    struct SEE_growable *grow,
		    unsigned int new_len, const char *file, int line);
#ifndef NDEBUG
# define SEE_grow_to(i, g, n) \
		_SEE_grow_to_debug(i,g,n,__FILE__,__LINE__)
#endif

/* A slightly faster variant of SEE_grow_to() */
#define SEE_GROW_TO(i,g,l) do {				\
	if ((l) > (g)->allocated / (g)->element_size)	\
	    SEE_grow_to(i, g, l);			\
	else						\
	    *(g)->length_ptr = (l);			\
    } while (0)

/* Convenience macros */
#define SEE_NEW(i, t)		(t *)SEE_malloc(i, sizeof (t))
#define SEE_NEW_FINALIZE(i, t, f, c) \
				(t *)SEE_malloc_finalize(i, sizeof (t), f, c)
#define SEE_NEW_ARRAY(i, t, n)	(t *)SEE_malloc(i, (n) * sizeof (t))
#define SEE_NEW_STRING_ARRAY(i, t, n) \
				(t *)SEE_malloc_string(i, (n) * sizeof (t))


#endif /* _SEE_h_mem_ */
