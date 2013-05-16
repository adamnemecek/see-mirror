/* Copyright (c) 2007, David Leonard. All rights reserved. */

/*
 * Generic trace framework for the see-shell.
 * Different options add different things to SEE's trace hook.
 * This module allows multiple traces to be added to SEE, so that
 * they are all called in sequence. (Last-added trace is added first).
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <see/see.h>
#include "shell.h"

#define MAXTRACES 32
static int ntraces;
static void (*traces[MAXTRACES])(struct SEE_interpreter *interp,
	struct SEE_throw_location *loc, struct SEE_context *context,
	enum SEE_trace_event event);

/* Runs all the traces added by add_trace */
static void
run_traces(interp, loc, context, event)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
	struct SEE_context *context;
	enum SEE_trace_event event;
{
	unsigned int i;
	for (i = 0; i < ntraces; i++)
	    (*traces[i])(interp, loc, context, event);
}

/* Appends a trace function to the system trace list.
 * Should be called before interpreters are created. */
void
shell_add_trace(trace)
	void (*trace)(struct SEE_interpreter *interp,
	    struct SEE_throw_location *loc, struct SEE_context *context,
	    enum SEE_trace_event event);
{
	traces[ntraces++] = trace;
	SEE_system.default_trace = run_traces;
}
