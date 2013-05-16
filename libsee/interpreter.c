/*
 * Copyright (c) 2003
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
/* $Id: interpreter.c 1320 2008-01-15 11:47:21Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#include <see/cfunction.h>
#include <see/context.h>
#include <see/intern.h>
#include <see/interpreter.h>
#include <see/mem.h>
#include <see/native.h>
#include <see/system.h>
#include <see/error.h>

#include "init.h"

/**
 * Initialises/reinitializes an interpreter structure
 * using the default compatibility flags.
 * The interpreter structure must be initialized at least once
 * before it can be used.
 */
void
SEE_interpreter_init(interp)
	struct SEE_interpreter *interp;
{
	SEE_interpreter_init_compat(interp, 
		SEE_system.default_compat_flags);
}

/**
 * Initialises/reinitializes an interpreter structure
 * using the given compatibility flags.
 * The interpreter structure must be initialized at least once
 * before it can be used.
 */
void
SEE_interpreter_init_compat(interp, compat_flags)
	struct SEE_interpreter *interp;
	int compat_flags;
{
	interp->try_context = NULL;
	interp->try_location = NULL;

	interp->compatibility = compat_flags;
	interp->random_seed = (*SEE_system.random_seed)();
	interp->trace = SEE_system.default_trace;
	interp->traceback = NULL;
	interp->locale = SEE_system.default_locale;
	interp->recursion_limit = SEE_system.default_recursion_limit;
	interp->sec_domain = NULL;
	interp->regex_engine = SEE_system.default_regex_engine;

	/* Allocate object storage first, since dependencies are complex */
	SEE_Array_alloc(interp);
	SEE_Boolean_alloc(interp);
	SEE_Date_alloc(interp);
	SEE_Error_alloc(interp);
	SEE_Function_alloc(interp);
	SEE_Global_alloc(interp);
	SEE_Math_alloc(interp);
	SEE_Number_alloc(interp);
	SEE_Object_alloc(interp);
	SEE_RegExp_alloc(interp);
	SEE_String_alloc(interp);
	_SEE_module_alloc(interp);

	/* Initialise the per-interpreter intern table now */
	_SEE_intern_init(interp);

	/* Initialise the objects; order *shouldn't* matter */
	SEE_Array_init(interp);
	SEE_Boolean_init(interp);
	SEE_Date_init(interp);
	SEE_Error_init(interp);
	SEE_Global_init(interp);
	SEE_Math_init(interp);
	SEE_Number_init(interp);
	SEE_Object_init(interp);
	SEE_RegExp_init(interp);
	SEE_String_init(interp);
	SEE_Function_init(interp);	/* Call late because of parser use */
	_SEE_module_init(interp);
}

struct SEE_interpreter_state {
	struct SEE_interpreter *interp;
	volatile struct SEE_try_context * try_context;
	struct SEE_throw_location * try_location;
	struct SEE_traceback *traceback;
};

/**
 * Saves sufficient interpreter state that allows another thread to
 * make a call into it.
 */
struct SEE_interpreter_state *
SEE_interpreter_save_state(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_interpreter_state *state;
	
	state = SEE_NEW(interp, struct SEE_interpreter_state);
	state->interp = interp;
	state->try_context = interp->try_context;
	state->try_location = interp->try_location;
	state->traceback = interp->traceback;
	return state;
}

/**
 * Restores saved interpreter state
 */
void
SEE_interpreter_restore_state(interp, state)
	struct SEE_interpreter *interp;
	struct SEE_interpreter_state *state;
{
	SEE_ASSERT(interp, state->interp == interp);
	interp->try_context = state->try_context;
	interp->try_location = state->try_location;
	interp->traceback = state->traceback;
}
