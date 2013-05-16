/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: no.h 1246 2007-06-18 05:15:06Z d $ */

#ifndef _SEE_h_no_
#define _SEE_h_no_

struct SEE_object;
struct SEE_string;
struct SEE_value;

/* Object class methods that do nothing or return empty or failure */
void	SEE_no_get(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_string *, struct SEE_value *val);
void	SEE_no_put(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_string *, struct SEE_value *val, int);
int	SEE_no_canput(struct SEE_interpreter *, struct SEE_object *,
		struct SEE_string *);
int	SEE_no_hasproperty(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_string *);
int	SEE_no_delete(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_string *);
void	SEE_no_defaultvalue(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_value *, struct SEE_value *);

/* SEE_no_enumerated is a deprecated function; use NULL instead */
struct SEE_enum *SEE_no_enumerator(struct SEE_interpreter *, 
		struct SEE_object *);

#endif /* _SEE_h_no_ */
