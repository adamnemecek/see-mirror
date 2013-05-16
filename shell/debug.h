/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _h_shell_debug
#define _h_shell_debug

struct debug;

struct debug *debug_new(struct SEE_interpreter *interp);
void debug_eval(struct SEE_interpreter *interp, struct debug *debug,
	struct SEE_input *input, struct SEE_value *res);
void debug_add_bp(struct SEE_interpreter *interp, struct debug *debug,
	const char *filename, int lineno);

#endif
