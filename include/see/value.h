/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: value.h 1333 2008-02-02 13:52:55Z d $ */

#ifndef _SEE_h_value_
#define _SEE_h_value_

/*
 * Values are small units of short-life, typed memory
 * that contain primitive information, object 
 * references or string references.
 */

#include <math.h>

#include <see/type.h>

/* if not defined, define NULL as a pointer, not as ANSI C's (0) */
#ifndef NULL
#define NULL	((void *)0)
#endif

struct SEE_object;
struct SEE_string;
struct SEE_value;
struct SEE_interpreter;

/* Value types */
enum SEE_type {
	SEE_UNDEFINED,
	SEE_NULL,
	SEE_BOOLEAN,
	SEE_NUMBER,
	SEE_STRING,
	SEE_OBJECT,
	SEE_REFERENCE,			/* internal type (8.7) */
	SEE_COMPLETION			/* internal type (8.9) */
};

/* This structure is not part of the public API and may change */
struct _SEE_reference {
	struct SEE_object *base;
	struct SEE_string *property;
};

/* This structure is not part of the public API and may change */
struct _SEE_completion {
	struct SEE_value *value;	/* Return value */
	unsigned int target;		/* Throw context or break target */
	enum { SEE_COMPLETION_NORMAL,
	       SEE_COMPLETION_BREAK, 
	       SEE_COMPLETION_CONTINUE, 
	       SEE_COMPLETION_RETURN,
	       SEE_COMPLETION_THROW } type;
};

/* Value storage */
struct SEE_value {
	enum SEE_type		      _type;
	union {
		SEE_number_t	      number;
		SEE_boolean_t	      boolean;
		struct SEE_object    *object;
		struct SEE_string    *string;
		/* The following members are not part of the public API */
		struct _SEE_reference  reference;
		struct _SEE_completion completion;
		void *_padding[4];
	} u;
};

/* Copy between value storages */
#define SEE_VALUE_COPY(dst, src)		\
	*(dst) = *(src)

/* Obtain the value's type */
#define SEE_VALUE_GET_TYPE(v)			\
	((v)->_type)

/* This macro is not part of the public API and may change */
#define _SEE_VALUE_SET_TYPE(v, t)		\
	(v)->_type = t

/* Fill a value storage with the undefined value */
#define SEE_SET_UNDEFINED(v)			\
	_SEE_VALUE_SET_TYPE(v, SEE_UNDEFINED)

/* Fill a value storage with the null value */
#define SEE_SET_NULL(v)				\
	_SEE_VALUE_SET_TYPE(v, SEE_NULL)

/* Fill a value storage with a boolean value */
#define SEE_SET_BOOLEAN(v, b) 			\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_BOOLEAN);	\
	(v)->u.boolean = (b);			\
    } while (0)

/* Fill a value storage with a numeric value */
#define SEE_SET_NUMBER(v, n) 			\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_NUMBER);	\
	(v)->u.number = (n);			\
    } while (0)

/* Fill a value storage with a pointer to a string */
#define SEE_SET_STRING(v, s)			\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_STRING);	\
	(v)->u.string = (s);			\
    } while (0)

/* Fill a value storage with a pointer to an object */
#define SEE_SET_OBJECT(v, o)			\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_OBJECT);	\
	(v)->u.object = (o);			\
    } while (0)

/* This macro is not part of the public API and may change */
#define _SEE_SET_REFERENCE(v, b, p)		\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_REFERENCE);	\
	(v)->u.reference.base = (b);		\
	(v)->u.reference.property = (p);	\
    } while (0)

/* This macro is not part of the public API and may change */
/* NB: 'val' must NOT be on the stack */
#define _SEE_SET_COMPLETION(v, typ, val, tgt)	\
    do {					\
	_SEE_VALUE_SET_TYPE(v, SEE_COMPLETION);	\
	(v)->u.completion.type = (typ);		\
	(v)->u.completion.value = (val);	\
	(v)->u.completion.target = (tgt);	\
    } while (0)

int _SEE_isnan(SEE_number_t n);
int _SEE_isfinite(SEE_number_t n);
SEE_number_t _SEE_copysign(SEE_number_t x, SEE_number_t y);
int _SEE_ispinf(SEE_number_t n);
int _SEE_isninf(SEE_number_t n);
#define SEE_ISNAN(n)	        _SEE_isnan(n)
#define SEE_ISFINITE(n)	        _SEE_isfinite(n)
#define SEE_COPYSIGN(x, y)	_SEE_copysign(x, y)
#define SEE_ISPINF(n)	        _SEE_ispinf(n)
#define SEE_ISNINF(n)	        _SEE_isninf(n)

/* Convenience macros for numbers */
#define SEE_NUMBER_ISNAN(v)     SEE_ISNAN((v)->u.number)
#define SEE_NUMBER_ISFINITE(v)  SEE_ISFINITE((v)->u.number)
#define SEE_NUMBER_ISPINF(v)    SEE_ISPINF((v)->u.number)
#define SEE_NUMBER_ISNINF(v)    SEE_ISNINF((v)->u.number)

/* SEE_ISINF() and SEE_NUMBER_ISINF() are deprecated */
#define SEE_ISINF(n)	        (SEE_ISPINF(n) || SEE_ISNINF(n))
#define SEE_NUMBER_ISINF(n)	SEE_ISINF((v)->u.number)

/* Converters */
void SEE_ToPrimitive(struct SEE_interpreter *i,
			struct SEE_value *val, struct SEE_value *type, 
			struct SEE_value *res);
void SEE_ToBoolean(struct SEE_interpreter *i, struct SEE_value *val, 
			struct SEE_value *res);
void SEE_ToNumber(struct SEE_interpreter *i, struct SEE_value *val,
			struct SEE_value *res);
void SEE_ToInteger(struct SEE_interpreter *i, struct SEE_value *val,
			struct SEE_value *res);
void SEE_ToString(struct SEE_interpreter *i, struct SEE_value *val,
			struct SEE_value *res);
void SEE_ToObject(struct SEE_interpreter *i, struct SEE_value *val, 
			struct SEE_value *res);

/* Integer converters */
SEE_int32_t  SEE_ToInt32(struct SEE_interpreter *i, struct SEE_value *val);
SEE_uint32_t SEE_ToUint32(struct SEE_interpreter *i, struct SEE_value *val);
SEE_uint16_t SEE_ToUint16(struct SEE_interpreter *i, struct SEE_value *val);

/* "0123456789abcdef" */
extern char SEE_hexstr_lowercase[16], SEE_hexstr_uppercase[16];

#endif /* _SEE_h_value_ */
