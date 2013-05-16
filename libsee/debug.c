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
/* $Id: debug.c 1346 2008-02-05 15:27:38Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include <see/try.h>
#include <see/value.h>
#include <see/object.h>
#include <see/string.h>
#include <see/debug.h>
#include <see/interpreter.h>
#include "function.h"

static void print_traceback(struct SEE_interpreter *interp,
	struct SEE_traceback *traceback, FILE *f);

/*
 * Print the contents of a value, without raising an exception
 */
void
SEE_PrintValue(interp, v, f)
	struct SEE_interpreter *interp;
	const struct SEE_value *v;
	FILE *f;
{
	if (!f) f=stderr;
	if (v == NULL) {
	    fprintf(f, "NULL");
	    return;
	}
	switch (SEE_VALUE_GET_TYPE(v)) {
	case SEE_UNDEFINED:
	    fprintf(f, "undefined");
	    break;
	case SEE_NULL:
	    fprintf(f, "null");
	    break;
	case SEE_BOOLEAN:
	    fprintf(f, v->u.boolean ? "true" : "false");
	    break;
	case SEE_NUMBER:
	    fprintf(f, "%.30g", v->u.number);
	    break;
	case SEE_STRING:
	    SEE_PrintString(interp, v->u.string, f);
	    break;
	case SEE_OBJECT:
	    SEE_PrintObject(interp, v->u.object, f);
	    break;
	case SEE_REFERENCE:
	    fprintf(f, "<ref base=<object %p> prop=", 
	    	(void *)v->u.reference.base);
	    SEE_string_fputs(v->u.reference.property, f);
	    fprintf(f, ">");
	    break;
	case SEE_COMPLETION:
	    switch (v->u.completion.type) {
	    case SEE_COMPLETION_NORMAL:
		fprintf(f, "<normal");
		if (v->u.completion.value) {
		    fprintf(f, " ");
		    SEE_PrintValue(interp, v->u.completion.value, f);
		}
		fprintf(f, ">");
		break;
	    case SEE_COMPLETION_BREAK:
		fprintf(f, "<break %u>", v->u.completion.target);
		break;
	    case SEE_COMPLETION_CONTINUE:
		fprintf(f, "<continue %u>", v->u.completion.target);
		break;
	    case SEE_COMPLETION_RETURN:
		fprintf(f, "<return ");
		SEE_PrintValue(interp, v->u.completion.value, f);
		fprintf(f, ">");
		break;
	    case SEE_COMPLETION_THROW:
		fprintf(f, "<throw ");
		SEE_PrintValue(interp, v->u.completion.value, f);
		fprintf(f, ">");
		break;
	    default:
		fprintf(f, "<BAD completion %d>", v->u.completion.type);
	    }
	    break;
	default:
	    fprintf(f, "<BAD value %d>", SEE_VALUE_GET_TYPE(v));
	}
}

/*
 * Print an object without raising an exception.
 * The object's class is shown in quotes.
 * If the object is known to the interpreter (eg Object, Array.prototype, etc.)
 * then its original name is shown in parentheses.
 */
void
SEE_PrintObject(interp, o, f)
	struct SEE_interpreter *interp;
	const struct SEE_object *o;
	FILE *f;
{
	const char *known;

	if (!f) f=stderr;
	if (o == NULL)				known = "NULL";
	else if (interp == NULL)		known = NULL;
	else if (o == interp->Global)		known = "Global";
	else if (o == interp->Object)		known = "Object";
	else if (o == interp->Object_prototype)	known = "Object.prototype";
	else if (o == interp->Error)		known = "Error";
	else if (o == interp->EvalError)	known = "EvalError";
	else if (o == interp->RangeError)	known = "RangeError";
	else if (o == interp->ReferenceError)	known = "ReferenceError";
	else if (o == interp->SyntaxError)	known = "SyntaxError";
	else if (o == interp->TypeError)	known = "TypeError";
	else if (o == interp->URIError)		known = "URIError";
	else if (o == interp->String)		known = "String";
	else if (o == interp->String_prototype)	known = "String.prototype";
	else if (o == interp->Function)		known = "Function";
	else if (o == interp->Function_prototype)known = "Function.prototype";
	else if (o == interp->Array)		known = "Array";
	else if (o == interp->Array_prototype)	known = "Array.prototype";
	else if (o == interp->Number)		known = "Number";
	else if (o == interp->Number_prototype)	known = "Number.prototype";
	else if (o == interp->Boolean)		known = "Boolean";
	else if (o == interp->Boolean_prototype)known = "Boolean.prototype";
	else if (o == interp->Math)		known = "Math";
	else if (o == interp->RegExp)		known = "RegExp";
	else if (o == interp->RegExp_prototype)	known = "RegExp.prototype";
	else if (o == interp->Date)		known = "Date";
	else if (o == interp->Date_prototype)	known = "Date.prototype";
	else					known = NULL;

	fprintf(f, "<object %p", (void *)o);
	if (known)
		fprintf(f, " (%s)", known);
	if (o && o->objectclass && !known) {
		fprintf(f, " \"%s\"", o->objectclass->Class);
	}
	if (o->host_data)
		fprintf(f, " %p", o->host_data);
	fprintf(f, ">");
}

/*
 * Print a string, in 'literal' form to the given stdio file.
 */
void
SEE_PrintString(interp, s, f)
	struct SEE_interpreter *interp;
	const struct SEE_string *s;
	FILE *f;
{
	unsigned int i;

	if (!f) f=stderr;
	if (s == NULL) 
	    fprintf(f, "<NULL>");
	else {
	    /* NB Replicates most of SEE_string_literal(). */
	    fprintf(f, "\"");
	    for (i = 0; i < s->length; i++) {
		SEE_char_t c = s->data[i];
		if (c == '\\') fprintf(f, "\\\\");
		else if (c == '\"') fprintf(f, "\\\"");
		else if (c == '\n') fprintf(f, "\\n");
		else if (c == '\t') fprintf(f, "\\t");
		else if (c >= ' ' && c <= '~')
			fputc(c & 0x7f, f);
		else if (c < 0x100)
			fprintf(f, "\\x%02x", c);
		else
			fprintf(f, "\\u%04x", c);
		if (i < s->length && i >= 1024) {
			fprintf(f, "\\(...len=%u)", s->length);
			break;
		}
	    }
	    fprintf(f, "\"<%s%p>", 
		    s->flags & SEE_STRING_FLAG_INTERNED ? "i" : "",
		    (void *)s);
	}
}

static void
print_traceback(interp, traceback, f)
	struct SEE_interpreter *interp;
	struct SEE_traceback *traceback;
	FILE *f;
{
	struct SEE_traceback *tb;
	struct SEE_string *locstr, *fname;
	struct SEE_object *fo;

	if (!f) f=stderr;
	if (!traceback)
		return;
	fprintf(f, "traceback:\n");
	for (tb = traceback; tb; tb = tb->prev) {
		locstr = SEE_location_string(interp, tb->call_location);
		fprintf(f, "\t");
		SEE_string_fputs(locstr, f);
		fo = tb->callee;
		if (tb->call_type == SEE_CALLTYPE_THROW)
		    fprintf(f, "<throw>");
		else if (fo == NULL)
		    fprintf(f, "?");
		else if (tb->call_type == SEE_CALLTYPE_CONSTRUCT)
		    fprintf(f, "new %s", fo->objectclass->Class 
			    ? fo->objectclass->Class 
			    : "?");
		else if (tb->call_type == SEE_CALLTYPE_CALL) {
		    fprintf(f, "call ");
		    /* XXX is fo == interp->Global_eval case handled OK? */
		    fname = SEE_function_getname(interp, fo);
		    if (fname) {
		        SEE_string_fputs(fname, f); 
			fprintf(f, "()");
		    } else 
		        fprintf(f, "<anonymous function>");
		} else
		    SEE_PrintObject(interp, fo, f);
		fprintf(f, "\n");
	}
}

void
SEE_PrintTraceback(interp, f)
	struct SEE_interpreter *interp;
	FILE *f;
{
	print_traceback(interp, interp->traceback, f);
}

void
SEE_PrintContextTraceback(interp, ctxt, f)
	struct SEE_interpreter *interp;
	volatile struct SEE_try_context *ctxt;
	FILE *f;
{
	print_traceback(interp, ctxt->traceback, f);
}
