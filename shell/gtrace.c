/* Copyright (c) 2007, David Leonard. All rights reserved. */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if WITH_BOEHM_GC
# include <gc/gc.h>
#endif

#include <see/see.h>
#include "shell.h"
#include "gtrace.h"

/*
 * gtrace() checks periodically if a SIGINFO signal has been sent 
 * to the process (usually generated with Control-T).
 * If so, it dumps some debugging information to standard error.
 */

static volatile int gtrace_pending;

/* Raises the gtrace request flag */
static void
gtrace_raise_request()
{
    gtrace_pending = 1;
}

/* Returns true if gtrace_raise_request() was recently called. 
 * Resets the flag. */
static int
gtrace_check_requested()
{
	if (gtrace_pending)  {
	    gtrace_pending = 0;
	    return 1;
	} else
	    return 0;
}

/* Dumps (hopefully) useful information */
static void
gtrace_dump(interp, loc, context)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
	struct SEE_context *context;
{
	if (loc) {
	    SEE_string_fputs(SEE_location_string(interp, loc), stderr);
	    fprintf(stderr, "gtrace\n");
	} 
#if WITH_BOEHM_GC
	{ void GC_dump(void); 
	  fprintf(stderr, "gtrace: GC_dump() follows:\n");
	  GC_dump(); }
#endif
	if (interp) {
	    SEE_string_fputs(SEE_location_string(interp, loc), stderr);
	    fprintf(stderr, "traceback follows:\n");
	    SEE_PrintTraceback(interp, stderr);
	    fprintf(stderr, "end of traceback\n");
	} else
	    fprintf(stderr, "gtrace: no interpreter, no traceback\n");
}

/* Called periodically by SEE only after gtrace_enable() is called */
static void
gtrace(interp, loc, context, event)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
	struct SEE_context *context;
	enum SEE_trace_event event;
{
	if (gtrace_check_requested())
	    gtrace_dump(interp, loc, context);
}

/* Called when a SIGINFO signal is delivered to the process.
 * Converts to a gtrace request */
static RETSIGTYPE
sig()
{
	gtrace_raise_request();
}

void
gtrace_enable()
{
	static int gtrace_enabled = 0;

	if (!gtrace_enabled) {
	    gtrace_enabled = 1;

	    /* Install triggers for gtrace */
#ifdef SIGINFO
	    signal(SIGINFO, sig);
#endif
	    shell_add_trace(gtrace);
	}
}
