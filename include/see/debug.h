/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: debug.h 1146 2006-08-27 07:25:55Z d $ */

#ifndef _SEE_h_debug_
#define _SEE_h_debug_

#include <stdio.h>

struct SEE_value;
struct SEE_object;
struct SEE_interpreter;
struct SEE_try_context;

/* Prints a representation of the value to the stream */
void SEE_PrintValue(struct SEE_interpreter *i, const struct SEE_value *v, 
		FILE *f);

/* Prints a representation of the object to the stream */
void SEE_PrintObject(struct SEE_interpreter *i, const struct SEE_object *o, 
		FILE *f);

/* Prints a string to the stream */
void SEE_PrintString(struct SEE_interpreter *i, const struct SEE_string *s, 
		FILE *f);

/* Prints the interpreter's current stack trace to the stream */
void SEE_PrintTraceback(struct SEE_interpreter *i, FILE *f);

/* Prints the stack trace caught from a try context to the stream */
void SEE_PrintContextTraceback(struct SEE_interpreter *i, 
		volatile struct SEE_try_context *context, FILE *f);

#endif /* _SEE_h_debug_ */
