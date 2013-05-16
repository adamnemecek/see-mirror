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
/* $Id: native.c 1361 2008-02-08 12:36:20Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include <see/native.h>
#include <see/string.h>
#include <see/intern.h>
#include <see/mem.h>
#include <see/error.h>
#include <see/debug.h>
#include <see/system.h>
#include <see/interpreter.h>

#include "stringdefs.h"
#include "dprint.h"

static unsigned int hashfn(struct SEE_string *);
static struct SEE_property **find(struct SEE_interpreter *,
	struct SEE_object *, struct SEE_string *);
static void native_enum_reset(struct SEE_interpreter *,
	struct SEE_enum *);
static struct SEE_string *native_enum_next(struct SEE_interpreter *,
	struct SEE_enum *, int *);

#ifndef NDEBUG
int SEE_native_debug = 0;
#endif

/*------------------------------------------------------------
 * Native objects
 *  - maintains a simple hash table of named properties
 *  - cannot be called as functions
 *  - cannot be called as a constructor
 */

struct SEE_property {
        struct SEE_property *next;
        struct SEE_string *name;
        int attr;
        struct SEE_value value;
};

/* Return a hash value for an interned string, in range [0..HASHLEN) */
static unsigned int
hashfn(s)
	struct SEE_string *s;
{
	return ((((unsigned int)s) >> 8) ^ (unsigned int)s >> 7) 
		% SEE_NATIVE_HASHLEN;
}

/*
 * Find an object property, if it exists.
 * Assumes property is interned.
 * Returns either a pointer to the pointer to the property,
 * or a pointer to the NULL pointer at the end of a hash list.
 * (ie never returns NULL);
 */
static struct SEE_property **
find(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	struct SEE_native *n = (struct SEE_native *)o;
	struct SEE_property **x;

	x = &n->properties[hashfn(_SEE_INTERN_ASSERT(interp, ip))];
	while (*x && (*x)->name != ip)
		x = &((*x)->next);
	return x;
}

/* [[Get]] 8.6.2.1 */
void
SEE_native_get(interp, o, ip, res)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
	struct SEE_value *res;
{
	struct SEE_property **x;
	struct SEE_native *n = (struct SEE_native *)o;

	if (n->lru && n->lru->name == ip) {
#ifndef NDEBUG
	    if (SEE_native_debug) {
		dprintf("native_get: o=");
		dprinto(interp, o);
		dprintf(" ip=");
		dprints(ip);
		dprintf("(%p) LRU HIT -> ", ip);
		dprintv(interp, &n->lru->value);
		dprintf("\n");
	    }
#endif
	    SEE_VALUE_COPY(res, &n->lru->value);
	    return;
	}

	x = find(interp, o, ip);

#ifndef NDEBUG
	if (SEE_native_debug) {
	    dprintf("native_get: o=");
	    dprinto(interp, o);
	    dprintf(" ip=");
	    dprints(ip);
	    dprintf("(%p)", ip);
	    if (*x) { 
		dprintf(" -> ");
		dprintv(interp, &(*x)->value);
		dprintf("\n");
	    } else 
		dprintf(" -> not found\n");
	}
#endif

	if (*x) {
	    n->lru = *x;
	    SEE_VALUE_COPY(res, &(*x)->value);
	} else if (SEE_GET_JS_COMPAT(interp) &&
		 ip == STR(__proto__)) {
	    if (o->Prototype)
		SEE_SET_OBJECT(res, o->Prototype);
	    else
		SEE_SET_NULL(res);
	} else {
#ifndef NDEBUG
	    if (SEE_native_debug) {
		dprintf("native_get: o=");
		dprinto(interp, o);
		dprintf(" has prototype=");
		dprinto(interp, o->Prototype);
		dprintf("\n");
	    }
#endif
	    if (!o->Prototype)
		SEE_SET_UNDEFINED(res);
	    else 
		SEE_OBJECT_GET(interp, o->Prototype, ip, res);
	}
}

/* [[Put]] 8.6.2.2 */
void
SEE_native_put(interp, o, ip, val, attr)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
	struct SEE_value *val;
	int attr;
{
	struct SEE_property **x;
	struct SEE_native *n = (struct SEE_native *)o;

	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(val) != SEE_REFERENCE);

	if (n->lru && n->lru->name == ip && 
	    !(n->lru->attr & SEE_ATTR_READONLY) && !attr)
	{
#ifndef NDEBUG
		if (SEE_native_debug) {
		    dprintf("native_put: o=");
		    dprinto(interp, o);
		    dprintf(" ip=");
		    dprints(ip);
		    dprintf("(%p) LRU HIT <- ", ip);
		    dprintv(interp, val);
		    dprintf("\n");
		}
#endif
		SEE_VALUE_COPY(&n->lru->value, val);
		return;
	}

	if (SEE_GET_JS_COMPAT(interp) && ip == STR(__proto__)) 
	{
		struct SEE_object *po;
		if (SEE_VALUE_GET_TYPE(val) == SEE_NULL) {
			o->Prototype = NULL;
			return;
		}
		if (SEE_VALUE_GET_TYPE(val) != SEE_OBJECT)
			/* XXX: better error message needed. 'bad proto'? */
			SEE_error_throw_string(interp, interp->TypeError, 
				STR(internal_error));
		/* Check for recursive prototype */
		for (po = val->u.object; po; po = po->Prototype)
		    if (SEE_OBJECT_JOINED(o, po))
			SEE_error_throw_string(interp, interp->TypeError, 
				STR(internal_error));
		o->Prototype = val->u.object;
		return;
	}

	/*
	 * Note: supply a non-zero attr implies that
	 * the caller knows what they're doing. This means
	 * we can ignore any restrictions on extant
	 * properties!
	 */
	if (!attr && !SEE_OBJECT_CANPUT(interp, o, ip))
		return;
	x = find(interp, o, ip);
	if (!*x) {
		struct SEE_property *prop;
		prop = SEE_NEW(interp, struct SEE_property);
		prop->next = NULL;
		prop->name = ip;
		prop->attr = attr;
		*x = prop;
	} else if (attr)
		(*x)->attr = attr;
	n->lru = *x;
	SEE_VALUE_COPY(&(*x)->value, val);

#ifndef NDEBUG
	if (SEE_native_debug) {
	    dprintf("native_put: o=");
	    dprinto(interp, o);
	    dprintf(" ip=");
	    dprints(ip);
	    dprintf("(%p) <- ", ip);
	    dprintv(interp, val);
	    if (attr) {
	    	dprintf("{");
		if (attr & SEE_ATTR_READONLY)   dprintf(" ReadOnly");
		if (attr & SEE_ATTR_DONTENUM)   dprintf(" DontEnum");
		if (attr & SEE_ATTR_DONTDELETE) dprintf(" DontDelete");
		if (attr & SEE_ATTR_INTERNAL)   dprintf(" Internal");
	    	dprintf(" }");
	    }
	    dprintf("\n");
	}
#endif
}

/* [[CanPut]] 8.6.2.3 */
int
SEE_native_canput(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	struct SEE_property **x;
	struct SEE_native *n = (struct SEE_native *)o;

	if (n->lru && n->lru->name == ip) {
#ifndef NDEBUG
		if (SEE_native_debug) {
		    dprintf("native_canput: o=");
		    dprinto(interp, o);
		    dprintf(" ip=");
		    dprints(ip);
		    dprintf("(%p) LRU HIT -> %d\n", ip,
			(n->lru->attr & SEE_ATTR_READONLY) ? 0 : 1);
		}
#endif
		return (n->lru->attr & SEE_ATTR_READONLY) ? 0 : 1;
	}

	x = find(interp, o, ip);
	if (*x) {
#ifndef NDEBUG
		if (SEE_native_debug) {
		    dprintf("native_canput: o=");
		    dprinto(interp, o);
		    dprintf(" ip=");
		    dprints(ip);
		    dprintf("(%p) -> %d\n", ip,
			((*x)->attr & SEE_ATTR_READONLY) ? 0 : 1);
		}
#endif
		n->lru = *x;
		return ((*x)->attr & SEE_ATTR_READONLY) ? 0 : 1;
	}
	if (!o->Prototype)
		return 1;
	return SEE_OBJECT_CANPUT(interp, o->Prototype, ip);
}

/* Test if a property is 'local' (ie not belonging to prototype) */
static int
native_hasownproperty(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	struct SEE_property **x;
	struct SEE_native *n = (struct SEE_native *)o;

	if (n->lru && n->lru->name == ip) {
#ifndef NDEBUG
	    if (SEE_native_debug) {
		dprintf("hasownprop: o=");
		dprinto(interp, o);
		dprintf(" ip=");
		dprints(ip);
		dprintf(" LRU HIT -> 1\n");
	    }
#endif
	    return 1;
	}

	x = find(interp, o, ip);
#ifndef NDEBUG
	if (SEE_native_debug) {
	    dprintf("hasownprop: o=");
	    dprinto(interp, o);
	    dprintf(" ip=");
	    dprints(ip);
	    dprintf(" -> %d\n", *x ? 1 : 0);
	}
#endif
	return *x ? 1 : 0;
}

/* [[HasProperty]] 8.6.2.4 */
int
SEE_native_hasproperty(interp, o, p)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *p;
{
	/* The common case is that of traversing native objects
	 * so we do it in a loop to save call overhead. */
	for (;;) {
		if (SEE_native_hasownproperty(interp, o, p))
		    return 1;
		if (!o->Prototype)
		    return 0;
		o = o->Prototype;
		if (o->objectclass->HasProperty != SEE_native_hasproperty)
		    return SEE_OBJECT_HASPROPERTY(interp, o, p);
	}
}

int
SEE_native_hasownproperty(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	return native_hasownproperty(interp, o, ip);
}

/* Return the attribute of a local property, or 0 */
int
SEE_native_getownattr(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	struct SEE_property **x;

	x = find(interp, o, ip);
	return *x ? (*x)->attr : 0;
}

/* [[Delete]] 8.6.2.5 */
int
SEE_native_delete(interp, o, ip)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_string *ip;
{
	struct SEE_property **x;
	struct SEE_native *n = (struct SEE_native *)o;

	x = find(interp, o, ip);
	if (!*x)
		return 1;
	if ((*x)->attr & SEE_ATTR_DONTDELETE)
		return 0;
	if (n->lru == *x)
	    n->lru = NULL;
	*x = (*x)->next;
	return 1;
}

/* [[DefaultValue]] 8.6.2.6 */
void
SEE_native_defaultvalue(interp, o, hint, res)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	struct SEE_value *hint;
	struct SEE_value *res;
{
	struct SEE_value v;
	struct SEE_object *effective_hint;

	if (!hint)
		effective_hint = interp->Number;
		/* Note that Date instances' [[DefaultValue]] is 
		 * provided by date_defaultvalue() */
	else if (SEE_VALUE_GET_TYPE(hint) == SEE_OBJECT && 
	    hint->u.object == interp->String)
		effective_hint = interp->String;
	else if (SEE_VALUE_GET_TYPE(hint) == SEE_OBJECT &&
	    hint->u.object == interp->Number)
		effective_hint = interp->Number;
	else if (SEE_VALUE_GET_TYPE(hint) == SEE_OBJECT &&
	    hint->u.object == interp->Date)
		effective_hint = interp->String;
	else 
		effective_hint = interp->Number;	/* XXX */

	if (effective_hint == interp->String) {
		SEE_OBJECT_GET(interp, o, STR(toString), &v);
		if (SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT && 
		    SEE_OBJECT_HAS_CALL(v.u.object)) 
		{
			SEE_OBJECT_CALL(interp, v.u.object, o, 0, NULL, res);
			if (SEE_VALUE_GET_TYPE(res) != SEE_OBJECT)
				return;
		}
		SEE_OBJECT_GET(interp, o, STR(valueOf), &v);
		if (SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT && 
		    SEE_OBJECT_HAS_CALL(v.u.object)) 
		{
			SEE_OBJECT_CALL(interp, v.u.object, o, 0, NULL, res);
			if (SEE_VALUE_GET_TYPE(res) != SEE_OBJECT)
				return;
		}
		if (SEE_COMPAT_JS(interp, >=, JS11)) /* EXT:5 */
			SEE_SET_STRING(res, SEE_string_sprintf(interp,
			    "[object %p]", o));
		else 
			SEE_error_throw_string(interp, interp->TypeError, 
				STR(defaultvalue_string_bad));
	} else {
		SEE_OBJECT_GET(interp, o, STR(valueOf), &v);
		if (SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT && 
		    SEE_OBJECT_HAS_CALL(v.u.object)) 
		{
			SEE_OBJECT_CALL(interp, v.u.object, o, 0, NULL, res);
			if (SEE_VALUE_GET_TYPE(res) != SEE_OBJECT)
				return;
		}
		SEE_OBJECT_GET(interp, o, STR(toString), &v);
		if (SEE_VALUE_GET_TYPE(&v) == SEE_OBJECT && 
		    SEE_OBJECT_HAS_CALL(v.u.object)) 
		{
			SEE_OBJECT_CALL(interp, v.u.object, o, 0, NULL, res);
			if (SEE_VALUE_GET_TYPE(res) != SEE_OBJECT)
				return;
		}
		if (SEE_COMPAT_JS(interp, >=, JS11)) /* EXT:6 */
			SEE_SET_STRING(res, SEE_string_sprintf(interp,
			    "[object %p]", o));
		else 
			SEE_error_throw_string(interp, interp->TypeError, 
				STR(defaultvalue_number_bad));
	}
}

/*
 * Enumeration support.
 *
 * Object enumerators only reveal the names available
 * at the local level. Object enumerators do not have to be
 * insert/delete-safe - i.e. they can assume that no change
 * to the object is made during enumeration.
 * Local properties with the DONTENUM flag set are still
 * enumerated, but the 'dont_enum' out-argument is set to non-zero
 * when they are encountered.
 */
struct native_enum {
	struct SEE_enum	base;
	struct SEE_native *native;
	int next_column;
	struct SEE_property *next_prop;
};

static void
native_enum_reset(interp, e)
	struct SEE_interpreter *interp;
	struct SEE_enum *e;
{
	struct native_enum *ne = (struct native_enum *)e;
	ne->next_column = 0;
	ne->next_prop = NULL;
}

static struct SEE_string *
native_enum_next(interp, e, dont_enump)
	struct SEE_interpreter *interp;
	struct SEE_enum *e;
	int *dont_enump;
{
	struct native_enum *ne = (struct native_enum *)e;
	struct SEE_native *n = ne->native;
	struct SEE_property *p;

	while (ne->next_prop == NULL) {
	    if (ne->next_column >= SEE_NATIVE_HASHLEN)
		    return NULL;
	    ne->next_prop = n->properties[ne->next_column++];
	}
	p = ne->next_prop;
	ne->next_prop = p->next;

	if (dont_enump)
		*dont_enump = (p->attr & SEE_ATTR_DONTENUM);
	return p->name;
}

static struct SEE_enumclass native_enumclass = {
	0,
	native_enum_next
};

struct SEE_enum *
SEE_native_enumerator(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct SEE_native *n = (struct SEE_native *)o;
	struct native_enum *ne;

	ne = SEE_NEW(interp, struct native_enum);
	ne->native = n;
	ne->base.enumclass = &native_enumclass;
	native_enum_reset(interp, (struct SEE_enum *)ne);
	return (struct SEE_enum *)ne;
}

/*------------------------------------------------------------
 * The 'default' native object.
 */

static struct SEE_objectclass native_class = {
	"native",				/* Class */
	SEE_native_get,				/* Get */
	SEE_native_put,				/* Put */
	SEE_native_canput,			/* CanPut */
	SEE_native_hasproperty,			/* HasProperty */
	SEE_native_delete,			/* Delete */
	SEE_native_defaultvalue,		/* DefaultValue */
	SEE_native_enumerator,			/* enumerator */
	NULL,					/* Construct */
	NULL,					/* Call */
	NULL,					/* HasInstance */
};

/* Return a new, native object */
struct SEE_object *
SEE_native_new(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_native *n;

	n = SEE_NEW(interp, struct SEE_native);
	SEE_native_init(n, interp, &native_class, NULL);
	return (struct SEE_object *)n;
}

/* Initialise a native object to have no properties */
void
SEE_native_init(n, interp, objectclass, prototype)
	struct SEE_native *n;
	struct SEE_interpreter *interp;
	struct SEE_objectclass *objectclass;
	struct SEE_object *prototype;
{
	int i;

	n->object.objectclass = objectclass;
	n->object.Prototype = prototype;
	n->object.host_data = NULL;
	n->lru = NULL;
	for (i = 0; i < SEE_NATIVE_HASHLEN; i++)
		n->properties[i] = NULL;
}
