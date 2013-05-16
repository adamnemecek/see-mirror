/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_system_
#define _SEE_h_system_

struct SEE_interpreter;
struct SEE_throw_location;
struct SEE_context;
struct SEE_code;
struct SEE_value;
struct SEE_regex_engine;

#include <see/interpreter.h>		/* for enum SEE_trace_event */
#include <see/type.h>

struct SEE_system {

	/* Interpreter field defaults */
	const char *default_locale;		/* default: NULL */
	int default_recursion_limit;		/* default: -1 (no limit) */
	void (*default_trace)(struct SEE_interpreter *, 
		struct SEE_throw_location *,
		struct SEE_context *,
		enum SEE_trace_event);		/* default: NULL */
	int default_compat_flags;

	unsigned int (*random_seed)(void);

	/* Fatal error handler */
	void (*abort)(struct SEE_interpreter *, 
		const char *) SEE_dead;

	/* Periodic execution callback */
	void (*periodic)(struct SEE_interpreter *);

	/* Memory allocator */
	void *(*malloc)(struct SEE_interpreter *, SEE_size_t,
		const char *, int);
	void *(*malloc_finalize)(struct SEE_interpreter *, SEE_size_t,
		void (*)(struct SEE_interpreter *, void *, void *), void *,
		const char *, int);
	void *(*malloc_string)(struct SEE_interpreter *, SEE_size_t,
		const char *, int);

	void (*free)(struct SEE_interpreter *, void *,
		const char *, int);
	void (*mem_exhausted)(struct SEE_interpreter *) SEE_dead;
	void (*gcollect)(struct SEE_interpreter *);

	/* Security domain tracking */
	void *(*transit_sec_domain)(struct SEE_interpreter *, void *);

	/* Bytecode backend */
	struct SEE_code *(*code_alloc)(struct SEE_interpreter *);

	/* Host object constructor hook. Called for 'new Object(o)'. */
	void (*object_construct)(struct SEE_interpreter *interp,
		struct SEE_object *self, struct SEE_object *thisobj,
		int argc, struct SEE_value **argv, struct SEE_value *res);

	/* Default regex engine to use (experimental) */
	const struct SEE_regex_engine *default_regex_engine;
};

extern struct SEE_system SEE_system;

void SEE_init(void);	    /* no-op; reserved for API 3.0 */

#define SEE_ABORT(interp, msg) (*SEE_system.abort)(interp, msg)

/* The following two functions are experimental and may change */
const char **SEE_regex_engine_list(void);
const struct SEE_regex_engine *SEE_regex_engine(const char *name);

#endif /* _SEE_h_system_ */
