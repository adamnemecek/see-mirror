/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: array.h 1215 2007-04-25 07:29:41Z d $ */

#ifndef _SEE_h_array_
#define _SEE_h_array_

#include <see/type.h>

struct SEE_object;
struct SEE_interpreter;
struct SEE_value;
struct SEE_string;

int	SEE_is_Array(struct SEE_object *a);
void	SEE_Array_push(struct SEE_interpreter *i, struct SEE_object *a,
		struct SEE_value *val);
SEE_uint32_t SEE_Array_length(struct SEE_interpreter *i, struct SEE_object *a);
int	SEE_to_array_index(struct SEE_string *, SEE_uint32_t *);


#endif /* _SEE_h_array_ */
