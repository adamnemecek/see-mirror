/*
 * Copyright (c) 2005
 *      David Leonard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of David Leonard nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/* $Id$ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_TIME
# if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif
#endif

#if WITH_BOEHM_GC
# include <gc/gc.h>
#endif

#include <see/interpreter.h>
#include <see/system.h>

#include "dprint.h"
#include "platform.h"
#include "code.h"
#include "regex.h"

/* Prototypes */
static unsigned int simple_random_seed(void);
#if WITH_BOEHM_GC
static void *simple_gc_malloc(struct SEE_interpreter *, SEE_size_t,
	const char *, int);
static void *simple_gc_malloc_string(struct SEE_interpreter *, SEE_size_t,
	const char *, int);
static void *simple_gc_malloc_finalize(struct SEE_interpreter *, SEE_size_t,
        void (*)(struct SEE_interpreter *, void *, void *), void *,
		const char *, int);
static void simple_gc_free(struct SEE_interpreter *, void *,
	const char *, int);
static void simple_gc_finalizer(void *, void *);
static void simple_gc_gcollect(struct SEE_interpreter *);
#else
static void *simple_malloc(struct SEE_interpreter *, SEE_size_t,
	const char *, int);
static void *simple_malloc_finalize(struct SEE_interpreter *, SEE_size_t,
        void (*)(struct SEE_interpreter *, void *, void *), void *,
		const char *, int);
static void simple_free(struct SEE_interpreter *, void *,
	const char *, int);
static void simple_finalize_all(void);
#endif
static void simple_mem_exhausted(struct SEE_interpreter *) SEE_dead;

/*
 * System defaults. This structure should be not be modified after
 * interpreters are created.
 */
struct SEE_system SEE_system = {
	NULL,				/* default_locale */
	-1,				/* default_recursion_limit */
	NULL,				/* default_trace */

	SEE_COMPAT_262_3B, 		/* default_compat_flags */

	simple_random_seed,		/* random_seed */

	_SEE_platform_abort,		/* abort */
	NULL,				/* periodic */

#if WITH_BOEHM_GC
	simple_gc_malloc,		/* malloc */
	simple_gc_malloc_finalize,	/* malloc_finalize */
	simple_gc_malloc_string,	/* malloc_string */
	simple_gc_free,			/* free */
	simple_mem_exhausted,		/* mem_exhausted */
	simple_gc_gcollect,		/* gcollect */
#else
	simple_malloc,			/* malloc */
	simple_malloc_finalize,		/* malloc_finalize */
	simple_malloc,			/* malloc_string */
	simple_free,			/* free */
	simple_mem_exhausted,		/* mem_exhausted */
	NULL,				/* gcollect */
#endif
	NULL,				/* transit_sec_domain */
	_SEE_code1_alloc,		/* code_alloc */
	NULL,				/* object_construct */
	&_SEE_ecma_regex_engine		/* default_regex_engine */
};

/*
 * A simple random number seed generator. It is not thread safe.
 */
static unsigned int
simple_random_seed()
{
	static unsigned int counter = 0;
	unsigned int r;

	r = counter++;
#if HAVE_TIME
	r += (unsigned int)time(0);
#endif
	return r;
}

/*
 * A simple memory exhausted handler.
 */
static void
simple_mem_exhausted(interp)
	struct SEE_interpreter *interp;
{
	SEE_ABORT(interp, "memory exhausted");
}


#if WITH_BOEHM_GC

/* Redefine GC_EXTRAS as a quick, happy way to pass through the file,line */
#undef GC_EXTRAS
#ifdef GC_ADD_CALLER
#  define GC_EXTRAS GC_RETURN_ADDR, file, line
#else
#  define GC_EXTRAS file, line
#endif

/*
 * Memory allocator using Boehm GC
 */
static void *
simple_gc_malloc(interp, size, file, line)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
{
	return GC_MALLOC(size);
}

/*
 * A private structure to hold finalization info. This is appended onto
 * objects that are allocated with finalization requirements.
 */
struct finalize_info {
	struct SEE_interpreter *interp;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
};

static void
simple_gc_finalizer(p, cd)
	void *p;
	void *cd;
{
	struct finalize_info *info = 
	    (struct finalize_info *)((char *)p + (SEE_size_t)cd);
	
	(*info->finalizefn)(info->interp, (void *)p, info->closure);
}

static void *
simple_gc_malloc_finalize(interp, size, finalizefn, closure, file, line)
	struct SEE_interpreter *interp;
	SEE_size_t size;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
	const char *file;
	int line;
{
	SEE_size_t padsz;
	void *data;
	struct finalize_info *info;

	/* Round up to align the finalize_info */
	padsz = size;
	padsz += sizeof (struct finalize_info) - 1;
	padsz -= padsz % sizeof (struct finalize_info);

	/* Allocate, with space for the finalize_info */
	data = GC_MALLOC(padsz + sizeof (struct finalize_info));

	/* Fill in the finalize_info now */
	info = (struct finalize_info *)((char *)data + padsz);
	info->interp = interp;
	info->finalizefn = finalizefn;
	info->closure = closure;

	/* Ask the GC to call the finalizer when data is unreachable */
	GC_REGISTER_FINALIZER(data, simple_gc_finalizer, (void *)padsz, NULL, NULL);

	return data;
}

/*
 * Non-pointer memory allocator using Boehm GC
 */
static void *
simple_gc_malloc_string(interp, size, file, line)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
{
	return GC_MALLOC_ATOMIC(size);
}

/*
 * Non-pointer memory allocator using Boehm GC
 */
static void
simple_gc_free(interp, ptr, file, line)
	struct SEE_interpreter *interp;
	void *ptr;
	const char *file;
	int line;
{
	GC_FREE(ptr);
}

static void
simple_gc_gcollect(interp)
	struct SEE_interpreter *interp;
{
	GC_gcollect();
}

#else /* !WITH_BOEHM_GC */


/*
 * Memory allocator using system malloc().
 * Note: most mallocs do not get freed! 
 * System strongly assumes a garbage collector.
 * This is a stub function.
 */
static void *
simple_malloc(interp, size, file, line)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
{
#ifndef NDEBUG
	static int warning_printed = 0;

	if (!warning_printed) {
		warning_printed++;
		dprintf("WARNING: SEE is using non-release malloc\n");
	}

#endif
	return malloc(size);
}

/* Linked list of all finalizers to run on exit */
static struct finalize_entry {
	struct SEE_interpreter *interp;
	void *ptr;
	void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
	struct finalize_entry *next;
} *simple_finalize_list;

/* Runs all the finalizers */
static void
simple_finalize_all()
{
	struct finalize_entry *entry;
#ifndef NDEBUG
	extern int SEE_mem_debug;
#endif

#ifndef NDEBUG
	if (SEE_mem_debug)
	    dprintf("Running finalizers\n");
#endif

	while (simple_finalize_list) {
	    entry = simple_finalize_list;
	    simple_finalize_list = entry->next;
	    (*entry->finalizefn)(entry->interp, entry->ptr, entry->closure);
	    free(entry);
	}
}

static void *
simple_malloc_finalize(interp, size, finalizefn, closure, file, line)
	struct SEE_interpreter *interp;
	SEE_size_t size;
        void (*finalizefn)(struct SEE_interpreter *, void *, void *);
	void *closure;
	const char *file;
	int line;
{
	static int called = 0;
	struct finalize_entry *entry;
	void *ptr;

	ptr = malloc(size);
	if (!ptr)
	    return NULL;

	/* Record the finalization function to call at exit */
	entry = (struct finalize_entry *)malloc(sizeof *entry);
	if (entry) {
	    entry->interp = interp;
	    entry->ptr = ptr;
	    entry->finalizefn = finalizefn;
	    entry->closure = closure;
	    /* Insert at head of list */
	    entry->next = simple_finalize_list;
	    simple_finalize_list = entry;
	    /* Set up finalizers to be run during exit() */
	    if (!called) {
		called = 1;
		atexit(simple_finalize_all);
	    }
	}

	return ptr;
}

/*
 * Memory deallocator using system free().
 */
static void
simple_free(interp, ptr, file, line)
	struct SEE_interpreter *interp;
	void *ptr;
	const char *file;
	int line;
{
	free(ptr);
}

#endif /* !WITH_BOEHM_GC */


/* Reserved for future use */
void
SEE_init()
{
	static int initialised = 0;

	if (initialised)
	    return;
	initialised = 1;

	SEE_regex_init();
}
