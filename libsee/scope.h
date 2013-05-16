/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_scope_
#define _SEE_h_scope_

struct SEE_interpreter;
struct SEE_string;
struct SEE_value;
struct SEE_scope;

void SEE_scope_lookup(struct SEE_interpreter *interp, struct SEE_scope *scope,
	struct SEE_string *name, struct SEE_value *res);
int SEE_scope_eq(struct SEE_scope *scope1, struct SEE_scope *scope2);


#endif /* _SEE_h_scope_ */
