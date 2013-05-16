/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: eval.h 1265 2007-07-24 14:33:04Z d $ */

#ifndef _SEE_h_eval
#define _SEE_h_eval

struct SEE_string;
struct SEE_value;
struct SEE_interpreter;
struct SEE_input;

/* Execution scope chain */
struct SEE_scope {
	struct SEE_scope *next;
	struct SEE_object *obj;
};

/* Parses and evaluates the program text from input in the Global scope */
void SEE_Global_eval(struct SEE_interpreter *i, struct SEE_input *input, 
	struct SEE_value *res);

/* Parses and evaluates the program text from input */
void SEE_eval(struct SEE_interpreter *i, struct SEE_input *input,
	struct SEE_object *thisobj, struct SEE_object *variable, 
	struct SEE_scope *scope, struct SEE_value *res);

/* Constructs a new function object from inputs */
struct SEE_object *SEE_Function_new(struct SEE_interpreter *i, 
	struct SEE_string *name, struct SEE_input *param_input, 
	struct SEE_input *body_input);

#endif /* _SEE_h_eval */
