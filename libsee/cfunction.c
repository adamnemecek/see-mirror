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
/* $Id: cfunction.c 1342 2008-02-05 15:10:29Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>   
#endif

#if HAVE_STRING_H
# include <string.h>   
#endif

#include <see/see.h>
#include <see/value.h>
#include <see/mem.h>
#include <see/string.h>
#include <see/object.h>
#include <see/native.h>
#include <see/no.h>
#include <see/cfunction.h>
#include <see/interpreter.h>
#include <see/error.h>

#include "stringdefs.h"
#include "cfunction_private.h"

/*
 * cfunction
 * 
 * These are the ECMAScript objects that wrap native C functions.
 * They are referred to in the introduction of section 15 of the
 * standard as 'built-in' functions.
 *
 * They have a [[Call]] property which invokes the appropriate 
 * C function, and also has a "length" property which 
 * gives the typical number of arguments to the [[Call]] method.
 * Their prototype is Function.prototype.
 *
 * The length property is implemented in a way equivalent to the
 * requirement that it "has the attributes { ReadOnly, DontDelete,
 * DontEnum } (and not others)." (15)
 *
 */

struct cfunction {
	struct SEE_object object;
	SEE_call_fn_t func;
	int length;
	struct SEE_string *name;
	void *sec_domain;
};

static struct cfunction *tocfunction(struct SEE_interpreter *interp,
	struct SEE_object *o);
static void cfunction_get(struct SEE_interpreter *, struct SEE_object *, 
	struct SEE_string *, struct SEE_value *);
static int cfunction_hasproperty(struct SEE_interpreter *, 
	struct SEE_object *, struct SEE_string *);
static void cfunction_call(struct SEE_interpreter *, struct SEE_object *,
	struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static char *to_ascii_string(struct SEE_interpreter *, struct SEE_string *);
static char *to_utf8_string(struct SEE_interpreter *, struct SEE_string *);
static struct SEE_string *from_string_buffer(struct SEE_interpreter *,
	const unsigned char *, size_t);
static struct SEE_string *from_ascii_string(struct SEE_interpreter *,
	const char *);
static struct SEE_string *from_utf8_string(struct SEE_interpreter *,
	const char *);
static void *cfunction_get_sec_domain(struct SEE_interpreter *, 
	struct SEE_object *);

/*
 * CFunction object class
 */
struct SEE_objectclass SEE_cfunction_class = {
	"Function",		/* Class */
	cfunction_get,		/* Get */
	SEE_no_put,		/* Put */
	SEE_no_canput,		/* CanPut */
	cfunction_hasproperty,	/* HasProperty */
	SEE_no_delete,		/* Delete */
	SEE_native_defaultvalue,/* DefaultValue */
	NULL,			/* enumerator */
	NULL,			/* Construct (15) */
	cfunction_call,		/* Call */
	NULL,			/* HasInstance */
	cfunction_get_sec_domain/* get_sec_domain */
};

/*
 * Return a CFunction object that wraps a C function
 */
struct SEE_object *
SEE_cfunction_make(interp, func, name, length)
	struct SEE_interpreter *interp;
	SEE_call_fn_t func;
	struct SEE_string *name;
	int length;
{
	struct cfunction *f;

	f = SEE_NEW(interp, struct cfunction);
	f->object.objectclass = &SEE_cfunction_class;
	f->object.Prototype = interp->Function_prototype;	/* 15 */
	f->object.host_data = NULL;
	f->func = func;
	f->name = name;
	f->length = length;
	f->sec_domain = interp->sec_domain;

	return (struct SEE_object *)f;
}

static struct cfunction *
tocfunction(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	if (!o || o->objectclass != &SEE_cfunction_class)
		SEE_error_throw_string(interp, interp->TypeError,
		   STR(not_cfunction));
	return (struct cfunction *)o;
}

/*------------------------------------------------------------
 * CFunction class methods
 */

static void
cfunction_get(interp, o, p, res)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
        struct SEE_value *res;
{
	struct cfunction *f = (struct cfunction *)o;

	if (p == STR(__proto__) && (SEE_COMPAT_JS(interp, >=, JS11)))
		SEE_SET_OBJECT(res, o->Prototype);
	else if (p == STR(length))
		SEE_SET_NUMBER(res, f->length);
	else
		SEE_OBJECT_GET(interp, o->Prototype, p, res);
}

static int
cfunction_hasproperty(interp, o, p)
	struct SEE_interpreter *interp;
        struct SEE_object *o;
        struct SEE_string *p;
{
	if (p == STR(length))
		return 1;
	return SEE_OBJECT_HASPROPERTY(interp, o->Prototype, p);
}

static void
cfunction_call(interp, o, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *o, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct cfunction *f = (struct cfunction *)o;

	(*f->func)(interp, o, thisobj, argc, argv, res);
}

void
SEE_cfunction_toString(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;
	struct cfunction *f = tocfunction(interp, thisobj);

	s = SEE_string_new(interp, 0);
	SEE_string_append(s, STR(cfunction_body1));
	SEE_string_append(s, f->name);
	SEE_string_append(s, STR(cfunction_body2));
	SEE_string_append_int(s, (int)f->func);
	SEE_string_append(s, STR(cfunction_body3));
	SEE_SET_STRING(res, s);
}

struct SEE_string *
SEE_cfunction_getname(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct cfunction *f = (struct cfunction *)o;

	return f->name;
}

/* Converts a SEE_string of ASCII chars into a C string */
static char *
to_ascii_string(interp, s)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
{
	int i;
	char *zs;

	zs = SEE_NEW_STRING_ARRAY(interp, char, s->length + 1);
	for (i = 0; i < s->length; i++)
	    if (s->data[i] == 0) 
		SEE_error_throw_string(interp, interp->TypeError,
			STR(string_contains_null));
	    else if (s->data[i] >= 0x80)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(string_not_ascii));
	    else
	    	zs[i] = s->data[i];
	zs[s->length] = 0;
	return zs;
}

/* Converts a SEE_string of chars into a UTF-8 string */
static char *
to_utf8_string(interp, s)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
{
	char *zs;
	int zslen, i;

	zslen = SEE_string_utf8_size(interp, s) + 1;
	zs = SEE_NEW_STRING_ARRAY(interp, char, zslen);
	SEE_string_toutf8(interp, zs, zslen, s);
	for (i = 0; i < zslen - 1; i++)
	    if (zs[i] == 0)
		SEE_error_throw_string(interp, interp->TypeError,
			STR(string_contains_null));
	return zs;
}

/* Creates a string from a binary buffer */
static struct SEE_string *
from_string_buffer(interp, buf, bufsz)
	struct SEE_interpreter *interp;
	const unsigned char *buf;
	SEE_size_t bufsz;
{
	struct SEE_string *s;
	int i;

	s = SEE_string_new(interp, bufsz);
	for (i = 0; i < bufsz; i++)
		s->data[i] = buf[i];
	s->length = bufsz;
	return s;
}

/* Creates a string from an ASCII C string */
static struct SEE_string *
from_ascii_string(interp, cp)
	struct SEE_interpreter *interp;
	const char *cp;
{
	struct SEE_string *s;
	int i, len;

	len = strlen(cp);
	s = SEE_string_new(interp, len);
	for (i = 0; i < len; i++)
		s->data[i] = cp[i] & 0x7f;
	s->length = len;
	return s;
}

/* Creates a string from a UTF-8 C string */
static struct SEE_string *
from_utf8_string(interp, cp)
	struct SEE_interpreter *interp;
	const char *cp;
{
	struct SEE_string *s;
	struct SEE_input *input;

	s = SEE_string_new(interp, 0);
	input = SEE_input_utf8(interp, cp);

	while (!input->eof) 
		SEE_string_addch(s, SEE_INPUT_NEXT(input));
	return s;
}

void
SEE_parse_args(interp, argc, argv, fmt)
	struct SEE_interpreter *interp;
	int argc;
	struct SEE_value **argv;
	const char *fmt;
{
	va_list ap;

	va_start(ap, fmt);
	SEE_parse_args_va(interp, argc, argv, fmt, ap);
	va_end(ap);
}

void
SEE_parse_args_va(interp, argc, argv, fmt, ap)
	struct SEE_interpreter *interp;
	int argc;
	struct SEE_value **argv;
	const char *fmt;
	va_list ap;
{
	int argi, init = 1, isundef, ignore;
	const char *f;
	struct SEE_value val, undef, *arg;
	struct SEE_string **stringp;
	int *intp;
	SEE_int32_t *int32p;
	SEE_uint32_t *uint32p;
	SEE_uint16_t *uint16p;
	SEE_number_t *numberp;
	struct SEE_object **objectpp;
	struct SEE_value *valuep;
	char **charpp;

	SEE_SET_UNDEFINED(&undef);

	for (argi = 0, f = fmt; *f; f++) {

	    /* Stop if we passed the '|' barrier and have exhausted args */
	    if (!init && argi >= argc)
	    	break;

	    /* If we have exhausted args, then use the undefined value */
	    if (argi < argc) {
		arg = argv[argi];
		isundef = (SEE_VALUE_GET_TYPE(arg) == SEE_UNDEFINED);
	    } else {
		SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&undef) == SEE_UNDEFINED);
		arg = &undef;
		isundef = 1;
	    }

	    /* If we passed the barrier and the value is undefined, then
	     * we make no change to the storage passed in */
	    ignore = (isundef && !init);

	    /* Process argv[argi] depending on the current format */
	    switch (*f) {
	    case ' ':
		break;
	    case 's':
		stringp = va_arg(ap, struct SEE_string **); argi++;
	        if (!ignore) {
		    SEE_ToString(interp, arg, &val);
		    *stringp = val.u.string;
		}
		break;
	    case 'A':
	    	if (isundef) {
		    charpp = va_arg(ap, char **); argi++;
		    if (!ignore)
			*charpp = NULL;
		    break;
		}
		/* else fallthrough */
	    case 'a':
		charpp = va_arg(ap, char **); argi++;
	        if (!ignore) {
		    SEE_ToString(interp, arg, &val); 
		    *charpp = to_ascii_string(interp, val.u.string);
		}
		break;
	    case 'Z':
	    	if (isundef) {
		    charpp = va_arg(ap, char **); argi++;
		    if (!ignore)
			*charpp = NULL;
		    break;
		}
		/* else fallthrough */
	    case 'z':
		charpp = va_arg(ap, char **); argi++;
	        if (!ignore) {
		    SEE_ToString(interp, arg, &val);
		    *charpp = to_utf8_string(interp, val.u.string);
		}
		break;
	    case 'b':
		intp = va_arg(ap, int *); argi++;
	        if (!ignore) {
		    SEE_ToBoolean(interp, arg, &val);
		    *intp = val.u.boolean ? 1 : 0;
		}
		break;
	    case 'i':
		int32p = va_arg(ap, SEE_int32_t *); argi++;
	        if (!ignore)
		    *int32p = SEE_ToInt32(interp, arg);
		break;
	    case 'u':
		uint32p = va_arg(ap, SEE_uint32_t *); argi++;
	        if (!ignore)
		    *uint32p = SEE_ToUint32(interp, arg);
		break;
	    case 'h':
		uint16p = va_arg(ap, SEE_uint16_t *); argi++;
	        if (!ignore)
		    *uint16p = SEE_ToUint16(interp, arg);
		break;
	    case 'n':
		numberp = va_arg(ap, SEE_number_t *); argi++;
	        if (!ignore) {
		    SEE_ToNumber(interp, arg, &val);
		    *numberp = val.u.number;
		}
		break;
	    case 'O':
	    	if (isundef || SEE_VALUE_GET_TYPE(arg) == SEE_NULL) {
		    objectpp = va_arg(ap, struct SEE_object **); argi++;
		    if (!ignore)
			*objectpp = NULL;
		    break;
		}
		/* else fallthrough */ 
	    case 'o':
		objectpp = va_arg(ap, struct SEE_object **); argi++;
	        if (!ignore) {
		    SEE_ToObject(interp, arg, &val);
		    *objectpp = val.u.object;
		}
		break;
	    case 'p':
		valuep = va_arg(ap, struct SEE_value *); argi++;
	        if (!ignore)
		    SEE_ToPrimitive(interp, arg, NULL, valuep);
		break;
	    case 'v':
		valuep = va_arg(ap, struct SEE_value *); argi++;
	        if (!ignore)
		    SEE_VALUE_COPY(valuep, arg);
		break;
	    case '|':
	    	init = 0;
		break;
	    case 'x':
	    	argi++;
		break;
	    case '.':
	    	if (argi < argc)
			SEE_error_throw_string(interp, interp->TypeError,
				STR(too_many_args));
		break;
	    default:
	    	SEE_ABORT(interp, "SEE_parse_args: bad format");
	    }
	}
}

void
SEE_call_args(interp, func, thisobj, ret, fmt)
	struct SEE_interpreter *interp;
	struct SEE_object *func, *thisobj;
        struct SEE_value *ret;
	const char *fmt;
{
	va_list ap;

	va_start(ap, fmt);
	SEE_call_args_va(interp, func, thisobj, ret, fmt, ap);
	va_end(ap);
}

void
SEE_call_args_va(interp, func, thisobj, ret, fmt, ap)
	struct SEE_interpreter *interp;
	struct SEE_object *func, *thisobj;
        struct SEE_value *ret;
	const char *fmt;
	va_list ap;
{
	int i, argc;
	const char *f;
	struct SEE_value *arg, **argv;
	struct SEE_string *stringv;
	int intv;
	SEE_int32_t int32v;
	SEE_uint32_t uint32v;
	SEE_uint16_t uint16v;
	SEE_number_t numberv;
	struct SEE_object *objectv;
	struct SEE_value *valuev;
	char *charpv;
	unsigned char *bufp;
	SEE_size_t bufsz;

	/* Count the arguments to allocate */
	argc = 0;
	for (f = fmt; *f; f++) 
	    switch (*f) {
	    case ' ':
	    	break;
	    case 'a':
	    case 'A':
	    case 'b':
	    case 'h':
	    case 'i':
	    case 'l':
	    case 'n':
	    case 'o':
	    case 'O':
	    case 'p':
	    case 's':
	    case 'u':
	    case 'v':
	    case 'x':
	    case 'z':
	    case 'Z':
	    case '*':
	    	argc++;
		break;
	    default:
	    	SEE_ABORT(interp, "SEE_call_args: bad format");
	    }

	arg = SEE_ALLOCA(interp, struct SEE_value, argc);
	argv = SEE_ALLOCA(interp, struct SEE_value *, argc);
	for (i = 0; i < argc; i++)
	    argv[i] = &arg[i];

	for (i = 0, f = fmt; *f; f++)
	    switch (*f) {
	    case ' ':
		break;
	    case 's':
	    	stringv = va_arg(ap, struct SEE_string *);
		if (stringv)
			SEE_SET_STRING(argv[i], stringv);
		else
			SEE_SET_UNDEFINED(argv[i]);
		i++;
		break;
	    case 'A':
	    	charpv = va_arg(ap, char *);
		if (charpv) {
			stringv = from_ascii_string(interp, charpv);
			SEE_SET_STRING(argv[i], stringv);
		} else
			SEE_SET_UNDEFINED(argv[i]);
		i++;
		break;
	    case 'a':
	    	charpv = va_arg(ap, char *);
		SEE_ASSERT(interp, charpv != NULL);
		stringv = from_ascii_string(interp, charpv);
		SEE_SET_STRING(argv[i], stringv);
		i++;
		break;
	    case 'Z':
	    	charpv = va_arg(ap, char *);
		if (charpv) {
			stringv = from_utf8_string(interp, charpv);
			SEE_SET_STRING(argv[i], stringv);
		} else
			SEE_SET_UNDEFINED(argv[i]);
		i++;
		break;
	    case 'z':
	    	charpv = va_arg(ap, char *);
		stringv = from_utf8_string(interp, charpv);
		SEE_SET_STRING(argv[i], stringv);
		i++;
		break;
	    case '*':
	    	bufp = va_arg(ap, unsigned char *);
		bufsz = va_arg(ap, SEE_size_t);
		stringv = from_string_buffer(interp, bufp, bufsz);
		SEE_SET_STRING(argv[i], stringv);
		i++;
		break;
	    case 'b':
	    	intv = va_arg(ap, int);
		SEE_SET_BOOLEAN(argv[i], intv);
		i++;
		break;
	    case 'i':
	    	int32v = va_arg(ap, SEE_int32_t);
		SEE_SET_NUMBER(argv[i], int32v);
		i++;
		break;
	    case 'u':
	    	uint32v = va_arg(ap, SEE_uint32_t);
		SEE_SET_NUMBER(argv[i], uint32v);
		i++;
		break;
	    case 'h':
	    	uint16v = va_arg(ap, int);
		SEE_SET_NUMBER(argv[i], uint16v);
		i++;
		break;
	    case 'l':
		SEE_SET_NULL(argv[i]);
		i++;
		break;
	    case 'n':
	    	numberv = va_arg(ap, SEE_number_t);
		SEE_SET_NUMBER(argv[i], numberv);
		i++;
		break;
	    case 'O':
	    	objectv = va_arg(ap, struct SEE_object *);
		if (objectv)
			SEE_SET_OBJECT(argv[i], objectv);
		else
			SEE_SET_UNDEFINED(argv[i]);
		i++;
		break;
	    case 'o':
	    	objectv = va_arg(ap, struct SEE_object *);
		SEE_ASSERT(interp, objectv != NULL);
		SEE_SET_OBJECT(argv[i], objectv);
		i++;
		break;
	    case 'p':
	    	valuev = va_arg(ap, struct SEE_value *);
	        SEE_ToObject(interp, valuev, argv[i]);
		i++;
		break;
	    case 'v':
	    	valuev = va_arg(ap, struct SEE_value *);
	        argv[i] = valuev;
		i++;
		break;
	    case 'x':
	        SEE_SET_UNDEFINED(argv[i]);
		i++;
		break;
	    }
	SEE_ASSERT(interp, i == argc);

	SEE_OBJECT_CALL(interp, func, thisobj, argc, argv, ret);
}

static void *
cfunction_get_sec_domain(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct cfunction *f = (struct cfunction *)o;

	return f->sec_domain;
}
