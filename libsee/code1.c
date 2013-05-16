/*
 * Copyright (c) 2007
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
/* $Id$ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include <see/interpreter.h>
#include <see/type.h>
#include <see/mem.h>
#include <see/value.h>
#include <see/error.h>
#include <see/try.h>
#include <see/string.h>
#include <see/context.h>
#include <see/system.h>
#include <see/intern.h>
#include <see/eval.h>

#include "dprint.h"
#include "code.h"
#include "stringdefs.h"
#include "scope.h"
#include "nmath.h"
#include "function.h"
#include "enumerate.h"
#include "code1.h"
#include "replace.h"

struct block {
    enum { BLOCK_ENUM, BLOCK_WITH, BLOCK_TRYC, BLOCK_TRYF, BLOCK_FINALLY } type;
    union {
	struct enum_context {
	    struct SEE_string **props0, **props;
	    struct SEE_object *obj;
	    struct enum_context *prev;
	} enum_context;
	struct SEE_scope with;
	struct {
	    SEE_try_context_t context;
	    struct block *last_try_block;
	    SEE_int32_t handler;
	    unsigned int stack;
	    unsigned int block;	/* Block level we are transiting to */
	} tryf;
	struct {
	    SEE_try_context_t context;
	    struct block *last_try_block;
	    SEE_int32_t handler;
	    unsigned int stack;
	    struct SEE_string *ident;
	} tryc;
    } u;
};

#ifdef NDEBUG
# define CAST_CODE(c)	((struct code1 *)(c))
#else
# define CAST_CODE(c)	cast_code((c), __FILE__, __LINE__)
static struct code1 *cast_code(struct SEE_code *, const char *, int);
#endif

/* Prototypes */
static void code1_gen_op0(struct SEE_code *co, enum SEE_code_op0 op);
static void code1_gen_op1(struct SEE_code *co, enum SEE_code_op1 op, int n);
static void code1_gen_literal(struct SEE_code *co, const struct SEE_value *v);
static void code1_gen_func(struct SEE_code *co, struct function *f);
static void code1_gen_loc(struct SEE_code *co, struct SEE_throw_location *loc);
static unsigned int code1_gen_var(struct SEE_code *co, struct SEE_string *name);
static void code1_gen_opa(struct SEE_code *co, enum SEE_code_opa op,
		SEE_code_patchable_t *patchp, SEE_code_addr_t addr);
static SEE_code_addr_t code1_here(struct SEE_code *co);
static void code1_patch(struct SEE_code *co, SEE_code_patchable_t patch,
		SEE_code_addr_t addr);
static void code1_maxstack(struct SEE_code *co, int);
static void code1_maxblock(struct SEE_code *co, int);
static void code1_close(struct SEE_code *co);
static void code1_exec(struct SEE_code *co, struct SEE_context *ctxt,
		struct SEE_value *res);

static unsigned int add_literal(struct code1 *code, 
		const struct SEE_value *val);
static unsigned int add_location(struct code1 *code, 
		const struct SEE_throw_location *location);
static unsigned int add_function(struct code1 *code, struct function *f);
static unsigned int add_var(struct code1 *code, struct SEE_string *ident);
static void add_byte(struct code1 *code, unsigned int c);
static unsigned int here(struct code1 *code);


static struct SEE_code_class code1_class = {
    "code1",
    code1_gen_op0,
    code1_gen_op1,
    code1_gen_literal,
    code1_gen_func,
    code1_gen_loc,
    code1_gen_var,
    code1_gen_opa,
    code1_here,
    code1_patch,
    code1_maxstack,
    code1_maxblock,
    code1_close,
    code1_exec
};

#ifndef NDEBUG
extern int SEE_eval_debug;
int SEE_code_debug;
static SEE_int32_t disasm(struct code1 *, SEE_int32_t pc);
#endif

struct SEE_code *
_SEE_code1_alloc(interp)
    struct SEE_interpreter *interp;
{
    struct code1 *co;
    
    co = SEE_NEW(interp, struct code1);
    co->code.code_class = &code1_class;
    co->code.interpreter = interp;

    SEE_GROW_INIT(interp, &co->ginst, co->inst, co->ninst);
    SEE_GROW_INIT(interp, &co->gliteral, co->literal, co->nliteral);
    SEE_GROW_INIT(interp, &co->gfunc, co->func, co->nfunc);
    SEE_GROW_INIT(interp, &co->glocation, co->location, co->nlocation);
    SEE_GROW_INIT(interp, &co->gvar, co->var, co->nvar);
    co->maxstack = -1;
    co->maxblock = -1;
    co->maxargc = 0;
    return (struct SEE_code *)co;
}

/* Adds a (unique) literal to the code object, returning its index */
static unsigned int
add_literal(code, val)
    struct code1 *code;
    const struct SEE_value *val;
{
    unsigned int i;
    int match = 0;
    struct SEE_interpreter *interp = code->code.interpreter;
    const struct SEE_value *li;

    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(val) != SEE_REFERENCE);
    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(val) != SEE_COMPLETION);

    for (i = 0; i < code->nliteral; i++) {
	li = code->literal + i;
	if (SEE_VALUE_GET_TYPE(li) != SEE_VALUE_GET_TYPE(val))
	    continue;

	switch (SEE_VALUE_GET_TYPE(val)) {
	case SEE_UNDEFINED:
	case SEE_NULL:
	    match = 1;
	    break;
	case SEE_BOOLEAN:
	    match = val->u.boolean ? li->u.boolean : !li->u.boolean;
	    break;
	case SEE_NUMBER:
	    /* Don't use == because -0 and +0 are different */
	    match = (memcmp(&val->u.number, &li->u.number, 
			sizeof val->u.number) == 0);
	    break;
	case SEE_STRING:
	    /* Strings are asserted above as interned */
	    match = val->u.string == li->u.string;
	    break;
	case SEE_OBJECT:
	    match = (val->u.object == li->u.object);
	    break;
	default: 
	    SEE_ASSERT(interp, !"bad value type");
	}
	if (match)
	    return i;
    }

    SEE_ASSERT(interp, i == code->nliteral);
    SEE_GROW_TO(interp, &code->gliteral, code->nliteral + 1);
    memcpy(code->literal + i, (void *)val, sizeof *val);

#ifndef NDEBUG
    if (SEE_code_debug > 1) {
	dprintf("add_literal: %p [%d] = ", code, i);
	dprintv(interp, code->literal + i);
	dprintf("\n");
    }
#endif

    return i;
}


/* Adds a (unique) function to the code object, returning its index */
static unsigned int
add_function(code, f)
    struct code1 *code;
    struct function *f;
{
    unsigned int i;
    struct SEE_interpreter *interp = code->code.interpreter;

    for (i = 0; i < code->nfunc; i++)
	if (code->func[i] == f)
	    return i;
    SEE_GROW_TO(interp, &code->gfunc, code->nfunc + 1);
    code->func[i] = f;
    return i;
}

/* Adds a (unique) location to the code object, returning its index */
static unsigned int
add_location(code, loc)
    struct code1 *code;
    const struct SEE_throw_location *loc;
{
    unsigned int i;
    struct SEE_interpreter *interp = code->code.interpreter;
    struct SEE_string *loc_filename = _SEE_INTERN_ASSERT(interp, loc->filename);

    /* Search backwards because if it's any line, its probably the last one */
    i = code->nlocation;
    while (i > 0) {
	i--;
	if (code->location[i].lineno == loc->lineno &&
	    code->location[i].filename == loc_filename)
	    return i;
    }
    i = code->nlocation;
    SEE_GROW_TO(interp, &code->glocation, code->nlocation + 1);
    code->location[i] = *loc;
    return i;
}

/* Adds a (unique) location to the code object, returning its index */
static unsigned int
add_var(code, ident)
    struct code1 *code;
    struct SEE_string *ident;
{
    unsigned int i, id;
    struct SEE_interpreter *interp = code->code.interpreter;
    struct SEE_value v;

    SEE_SET_STRING(&v, ident);
    id = add_literal(code, &v);

    for (i = 0; i < code->nvar; i++)
	if (code->var[i] == id)
	    return i;
    SEE_GROW_TO(interp, &code->gvar, code->nvar + 1);
    code->var[i] = id;
    return i;
}

/* Appends a byte to the code stream  */
static void
add_byte(code, c)
    struct code1 *code;
    unsigned int c;
{
    struct SEE_interpreter *interp = code->code.interpreter;
    unsigned int offset = code->ninst;

#ifndef NDEBUG
    if (SEE_code_debug > 1)
	dprintf("add_byte(%p, 0x%02x)\n", code, c);
#endif
    SEE_GROW_TO(interp, &code->ginst, code->ninst + 1);
    code->inst[offset] = c;
}

static unsigned int
here(code)
    struct code1 *code;
{
    return code->ninst;
}

/* Appends a 32-bit signed integer to the code stream  */
static void
add_word(code, n)
    struct code1 *code;
    SEE_int32_t n;
{
    struct SEE_interpreter *interp = code->code.interpreter;
    unsigned int offset = code->ninst;

#ifndef NDEBUG
    if (SEE_code_debug > 1)
	dprintf("add_word(%p, %d)\n", code, n);
#endif
    SEE_GROW_TO(interp, &code->ginst, offset + sizeof n);
    memcpy(code->inst + offset, &n, sizeof n);
}

/* Inserts a 32-bit signed integer into the code stream  */
static void
put_word(code, n, offset)
    struct code1 *code;
    SEE_int32_t n;
    unsigned int offset;
{
    memcpy(code->inst + offset, &n, sizeof n);
}

/* Adds a byte followed by a compact integer */
static void
add_byte_arg(code, c, arg)
    struct code1 *code;
    unsigned char c;
    int arg;
{
    if (arg >= 0 && arg < 0x100) {
	add_byte(code, c | INST_ARG_BYTE);
	add_byte(code, arg & 0xff);
    } else {
	add_byte(code, c | INST_ARG_WORD);
	add_word(code, arg);
    }
}


/* Safe cast. Aborts if it is asked to cast a NULL pointer, or a SEE_code 
 * object that does not come from this module. */
#ifndef NDEBUG
static struct code1 *
cast_code(sco, file, line)
    struct SEE_code *sco;
    const char *file;
    int line;
{
    if (!sco || sco->code_class != &code1_class) {
	dprintf("%s:%d: internal error: cast to code1 failed [vers %s]\n",
	    file, line, PACKAGE_VERSION);
	abort();
    }
    return (struct code1 *)sco;
}
#endif

/*------------------------------------------------------------
 * SEE_code interface for parser
 */

static void
code1_gen_op0(sco, op)
	struct SEE_code *sco;
	enum SEE_code_op0 op;
{
	struct code1 *co = CAST_CODE(sco);
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	switch (op) {
	case SEE_CODE_NOP:	add_byte(co, INST_NOP); break;
	case SEE_CODE_DUP:	add_byte(co, INST_DUP); break;
	case SEE_CODE_POP:	add_byte(co, INST_POP); break;
	case SEE_CODE_EXCH:	add_byte(co, INST_EXCH); break;
	case SEE_CODE_ROLL3:	add_byte(co, INST_ROLL3); break;
	case SEE_CODE_THROW:	add_byte(co, INST_THROW); break;
	case SEE_CODE_SETC:	add_byte(co, INST_SETC); break;
	case SEE_CODE_GETC:	add_byte(co, INST_GETC); break;
	case SEE_CODE_THIS:	add_byte(co, INST_THIS); break;
	case SEE_CODE_OBJECT:	add_byte(co, INST_OBJECT); break;
	case SEE_CODE_ARRAY:	add_byte(co, INST_ARRAY); break;
	case SEE_CODE_REGEXP:	add_byte(co, INST_REGEXP); break;
	case SEE_CODE_REF:	add_byte(co, INST_REF); break;
	case SEE_CODE_GETVALUE:	add_byte(co, INST_GETVALUE); break;
	case SEE_CODE_LOOKUP:	add_byte(co, INST_LOOKUP); break;
	case SEE_CODE_PUTVALUE:	add_byte(co, INST_PUTVALUE); break;
	case SEE_CODE_DELETE:	add_byte(co, INST_DELETE); break;
	case SEE_CODE_TYPEOF:	add_byte(co, INST_TYPEOF); break;
	case SEE_CODE_TOOBJECT:	add_byte(co, INST_TOOBJECT); break;
	case SEE_CODE_TONUMBER:	add_byte(co, INST_TONUMBER); break;
	case SEE_CODE_TOBOOLEAN:add_byte(co, INST_TOBOOLEAN); break;
	case SEE_CODE_TOSTRING:	add_byte(co, INST_TOSTRING); break;
	case SEE_CODE_TOPRIMITIVE:add_byte(co, INST_TOPRIMITIVE); break;
	case SEE_CODE_NEG:	add_byte(co, INST_NEG); break;
	case SEE_CODE_INV:	add_byte(co, INST_INV); break;
	case SEE_CODE_NOT:	add_byte(co, INST_NOT); break;
	case SEE_CODE_MUL:	add_byte(co, INST_MUL); break;
	case SEE_CODE_DIV:	add_byte(co, INST_DIV); break;
	case SEE_CODE_MOD:	add_byte(co, INST_MOD); break;
	case SEE_CODE_ADD:	add_byte(co, INST_ADD); break;
	case SEE_CODE_SUB:	add_byte(co, INST_SUB); break;
	case SEE_CODE_LSHIFT:	add_byte(co, INST_LSHIFT); break;
	case SEE_CODE_RSHIFT:	add_byte(co, INST_RSHIFT); break;
	case SEE_CODE_URSHIFT:	add_byte(co, INST_URSHIFT); break;
	case SEE_CODE_LT:	add_byte(co, INST_LT); break;
	case SEE_CODE_GT:	add_byte(co, INST_GT); break;
	case SEE_CODE_LE:	add_byte(co, INST_LE); break;
	case SEE_CODE_GE:	add_byte(co, INST_GE); break;
	case SEE_CODE_INSTANCEOF:add_byte(co, INST_INSTANCEOF); break;
	case SEE_CODE_IN:	add_byte(co, INST_IN); break;
	case SEE_CODE_EQ:	add_byte(co, INST_EQ); break;
	case SEE_CODE_SEQ:	add_byte(co, INST_SEQ); break;
	case SEE_CODE_BAND:	add_byte(co, INST_BAND); break;
	case SEE_CODE_BXOR:	add_byte(co, INST_BXOR); break;
	case SEE_CODE_BOR:	add_byte(co, INST_BOR); break;
	case SEE_CODE_S_ENUM:	add_byte(co, INST_S_ENUM); break;
	case SEE_CODE_S_WITH:	add_byte(co, INST_S_WITH); break;
	default: SEE_ASSERT(sco->interpreter, !"bad op0");
	}

#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static void
code1_gen_op1(sco, op, n)
	struct SEE_code *sco;
	enum SEE_code_op1 op; 
	int n;
{
	struct code1 *co = CAST_CODE(sco);
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	switch (op) {
	case SEE_CODE_NEW:	add_byte_arg(co, INST_NEW, n); break;
	case SEE_CODE_CALL:	add_byte_arg(co, INST_CALL, n); break;
	case SEE_CODE_END:	add_byte_arg(co, INST_END, n); break;
	case SEE_CODE_VREF:	add_byte_arg(co, INST_VREF, n); break;
	case SEE_CODE_PUTVALUEA:add_byte_arg(co, INST_PUTVALUE, n); break;
	default: SEE_ASSERT(sco->interpreter, !"bad op1");
	}

	if (op == SEE_CODE_NEW || op == SEE_CODE_CALL) {
	    if (n > co->maxargc)
		co->maxargc = n;
	}

#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static void
code1_gen_literal(sco, v)
	struct SEE_code *sco;
	const struct SEE_value *v;
{
	struct code1 *co = CAST_CODE(sco);
	unsigned int id = add_literal(co, v);
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	add_byte_arg(co, INST_LITERAL, id);
#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static void
code1_gen_func(sco, f)
	struct SEE_code *sco;
	struct function *f;
{
	struct code1 *co = CAST_CODE(sco);
	unsigned int id = add_function(co, f);
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	add_byte_arg(co, INST_FUNC, id);
#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static void
code1_gen_loc(sco, loc)
	struct SEE_code *sco;
	struct SEE_throw_location *loc;
{
	struct code1 *co = CAST_CODE(sco);
	unsigned int id = add_location(co, loc);
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	add_byte_arg(co, INST_LOC, id);
#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static unsigned int
code1_gen_var(sco, ident)
	struct SEE_code *sco;
	struct SEE_string *ident;
{
	struct code1 *co = CAST_CODE(sco);
	unsigned int id = add_var(co, ident);

#ifndef NDEBUG
	if (SEE_code_debug) {
	    dprintf("code1: var ");
	    dprints(ident);
	    dprintf(" -> id %u\n", id);
	}
#endif
	return id;
}

static void
code1_gen_opa(sco, opa, patchp, addr)
	struct SEE_code *sco;
	enum SEE_code_opa opa;
	SEE_code_patchable_t *patchp;
	SEE_code_addr_t addr;
{
	struct code1 *co = CAST_CODE(sco);
	unsigned char b;
#ifndef NDEBUG
	SEE_int32_t pc = co->ninst;
#endif

	switch (opa) {
	case SEE_CODE_B_ALWAYS:	b = INST_B_ALWAYS; break;
	case SEE_CODE_B_TRUE:	b = INST_B_TRUE; break;
	case SEE_CODE_B_ENUM:	b = INST_B_ENUM; break;
	case SEE_CODE_S_TRYC:	b = INST_S_TRYC; break;
	case SEE_CODE_S_TRYF:	b = INST_S_TRYF; break;
	default: SEE_ASSERT(sco->interpreter, !"bad opa");return;
	}
	add_byte(co, b | INST_ARG_WORD);
	if (patchp)
	    *(SEE_int32_t *)patchp = here(co);
	add_word(co, (SEE_int32_t)addr);

#ifndef NDEBUG
	if (SEE_code_debug > 1)
	    disasm(co, pc);
#endif
}

static SEE_code_addr_t
code1_here(sco)
	struct SEE_code *sco;
{
	struct code1 *co = CAST_CODE(sco);

	return (SEE_code_addr_t)here(co);
}

static void
code1_patch(sco, patch, addr)
	struct SEE_code *sco;
	SEE_code_patchable_t patch;
	SEE_code_addr_t addr;
{
	struct code1 *co = CAST_CODE(sco);
	SEE_int32_t arg = (SEE_int32_t)addr;
	SEE_int32_t offset = (SEE_int32_t)patch;

	put_word(co, arg, offset);

#ifndef NDEBUG
	if (SEE_code_debug > 1) {
	    dprintf("patch [%p] @0x%x <- 0x%x\n", sco, offset, arg);
	    disasm(co, offset - 1);
	}
#endif
}

static void
code1_maxstack(sco, maxstack)
	struct SEE_code *sco;
	int maxstack;
{
	struct code1 *co = CAST_CODE(sco);

	co->maxstack = maxstack;
}

static void
code1_maxblock(sco, maxblock)
	struct SEE_code *sco;
	int maxblock;
{
	struct code1 *co = CAST_CODE(sco);

	co->maxblock = maxblock;
}

static void
code1_close(sco)
	struct SEE_code *sco;
{
	/* Not implemented */
}

/*------------------------------------------------------------
 * Execution
 */

/* Converts a reference to a value, in situ */
static void
GetValue(interp, vp)
	struct SEE_interpreter *interp;
	struct SEE_value *vp;
{
	if (SEE_VALUE_GET_TYPE(vp) == SEE_REFERENCE) {
	    struct SEE_object *base = vp->u.reference.base;
	    struct SEE_string *prop = vp->u.reference.property;
	    if (base == NULL)
		SEE_error_throw_string(interp, interp->ReferenceError, prop);
	    SEE_OBJECT_GET(interp, base, SEE_intern(interp, prop), vp);
	}
}

static void
AbstractRelational(interp, x, y, res)
	struct SEE_interpreter *interp;
	struct SEE_value *x, *y, *res;
{
	struct SEE_value r1, r2, r4, r5;
	struct SEE_value hint;
	int k;

	SEE_SET_OBJECT(&hint, interp->Number);

	SEE_ToPrimitive(interp, x, &hint, &r1);
	SEE_ToPrimitive(interp, y, &hint, &r2);
	if (!(SEE_VALUE_GET_TYPE(&r1) == SEE_STRING && 
	      SEE_VALUE_GET_TYPE(&r2) == SEE_STRING)) 
	{
	    SEE_ToNumber(interp, &r1, &r4);
	    SEE_ToNumber(interp, &r2, &r5);
	    if (SEE_NUMBER_ISNAN(&r4) || SEE_NUMBER_ISNAN(&r5))
		SEE_SET_UNDEFINED(res);
	    else if (r4.u.number == r5.u.number)
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISPINF(&r4))
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISPINF(&r5))
		SEE_SET_BOOLEAN(res, 1);
	    else if (SEE_NUMBER_ISNINF(&r5))
		SEE_SET_BOOLEAN(res, 0);
	    else if (SEE_NUMBER_ISNINF(&r4))
		SEE_SET_BOOLEAN(res, 1);
	    else 
	        SEE_SET_BOOLEAN(res, r4.u.number < r5.u.number);
	} else {
	    for (k = 0; 
		 k < r1.u.string->length && k < r2.u.string->length;
		 k++)
		if (r1.u.string->data[k] != r2.u.string->data[k])
			break;
	    if (k == r2.u.string->length)
		SEE_SET_BOOLEAN(res, 0);
	    else if (k == r1.u.string->length)
		SEE_SET_BOOLEAN(res, 1);
	    else
		SEE_SET_BOOLEAN(res, r1.u.string->data[k] < 
				 r2.u.string->data[k]);
	}
}

static int
Seq(x, y)
        struct SEE_value *x, *y;
{
        if (SEE_VALUE_GET_TYPE(x) != SEE_VALUE_GET_TYPE(y))
            return 0;
        else
            switch (SEE_VALUE_GET_TYPE(x)) {
            case SEE_UNDEFINED:
                return 1;
            case SEE_NULL:
                return 1;
            case SEE_NUMBER:
                if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
                        return 0;
                else
                        return x->u.number == y->u.number;
            case SEE_STRING:
                return SEE_string_cmp(x->u.string, y->u.string) == 0;
            case SEE_BOOLEAN:
                return !x->u.boolean == !y->u.boolean;
            case SEE_OBJECT:
                return SEE_OBJECT_JOINED(x->u.object, y->u.object);
            default:
                return 0;
            }
}

/* From EqualityExpression_eq() */
static int
Eq(interp, x, y)
        struct SEE_interpreter *interp;
        struct SEE_value *x, *y;
{
        struct SEE_value tmp;
        int xtype, ytype;

        if (SEE_VALUE_GET_TYPE(x) == SEE_VALUE_GET_TYPE(y))
            switch (SEE_VALUE_GET_TYPE(x)) {
            case SEE_UNDEFINED:
            case SEE_NULL:
                return 1;
            case SEE_NUMBER:
                if (SEE_NUMBER_ISNAN(x) || SEE_NUMBER_ISNAN(y))
                    return 0;
                else
                    return x->u.number == y->u.number;
            case SEE_STRING:
                return SEE_string_cmp(x->u.string, y->u.string) == 0;
            case SEE_BOOLEAN:
                return !x->u.boolean == !y->u.boolean;
            case SEE_OBJECT:
                return SEE_OBJECT_JOINED(x->u.object, y->u.object);
            default:
                SEE_error_throw_string(interp, interp->Error,
                        STR(internal_error));
            }
        xtype = SEE_VALUE_GET_TYPE(x);
        ytype = SEE_VALUE_GET_TYPE(y);
        if (xtype == SEE_NULL && ytype == SEE_UNDEFINED)
                return 1;
        else if (xtype == SEE_UNDEFINED && ytype == SEE_NULL)
                return 1;
        else if (xtype == SEE_NUMBER && ytype == SEE_STRING) {
                SEE_ToNumber(interp, y, &tmp);
                return Eq(interp, x, &tmp);
        } else if (xtype == SEE_STRING && ytype == SEE_NUMBER) {
                SEE_ToNumber(interp, x, &tmp);
                return Eq(interp, &tmp, y);
        } else if (xtype == SEE_BOOLEAN) {
                SEE_ToNumber(interp, x, &tmp);
                return Eq(interp, &tmp, y);
        } else if (ytype == SEE_BOOLEAN) {
                SEE_ToNumber(interp, y, &tmp);
                return Eq(interp, x, &tmp);
        } else if ((xtype == SEE_STRING || xtype == SEE_NUMBER) &&
                    ytype == SEE_OBJECT) {
                SEE_ToPrimitive(interp, y, x, &tmp);
                return Eq(interp, x, &tmp);
        } else if ((ytype == SEE_STRING || ytype == SEE_NUMBER) &&
                    xtype == SEE_OBJECT) {
                SEE_ToPrimitive(interp, x, y, &tmp);
                return Eq(interp, &tmp, y);
        } else
                return 0;
}

static void
code1_exec(sco, ctxt, res)
	struct SEE_code *sco;
	struct SEE_context *ctxt;
	struct SEE_value *res;
{
	struct SEE_interpreter * const interp = ctxt->interpreter;
	struct code1 * const co = CAST_CODE(sco);
	struct SEE_string *str;
	struct SEE_value t, u, v;		/* scratch values */
	struct SEE_value *up, *vp, *wp;
	struct SEE_value **argv;
	struct SEE_value undefined, Number;
	struct SEE_object *obj, *baseobj;
	struct SEE_throw_location *location = NULL;
	unsigned char op;
	SEE_int32_t arg;
	SEE_int32_t int32;
	SEE_uint32_t uint32;
	int i, new_blocklevel;
	SEE_number_t number;
#define VOLATILE /* volatile */
	VOLATILE unsigned char *pc;
	VOLATILE struct SEE_value *stackbottom;
	VOLATILE struct SEE_value *stack;
	VOLATILE struct block *blockbottom, *block;
	VOLATILE struct block *try_block = NULL;
	VOLATILE int blocklevel;
	VOLATILE struct enum_context *enum_context = NULL;
	VOLATILE struct SEE_scope *scope;

/*
 * The PUSH() and POP() macros work by setting /pointers/ into
 * the stack. They don't copy any values. Only pointers. So,
 * to use these, you call POP to get a pointer onto the stack
 * which you are expected to read; and you use PUSH to get a 
 * pointer into the stack where you are expected to store a 
 * result. Be very careful that you read from the popped pointer
 * before you write into the pushed pointer! The whole reason
 * it is done like this is to improve performance, and avoid
 * value copying. i.e. you must explicitly copy values if you
 * fear overwriting a pointer.
 */

#define POP0()	do {					\
	/* Macro to pop a value and discard it */	\
	SEE_ASSERT(interp, stack > stackbottom);	\
	stack--;					\
    } while (0)

#define POP(vp)	do {					\
	/* Macro to pop a value off the stack		\
	 * and set vp to the value */			\
	SEE_ASSERT(interp, stack > stackbottom);	\
	vp = --stack;					\
    } while (0)

#define PUSH(vp) do {					\
	/* Macro to prepare pushing a value onto the	\
	 * stack. vp is set to point to the storage. */	\
	vp = stack++;					\
	SEE_ASSERT(interp, stack <= stackbottom + co->maxstack); \
    } while (0)

#define TOP(vp)	do {					\
	/* Macro to access the value on top of the	\
	 * without popping it. */			\
	SEE_ASSERT(interp, stack > stackbottom);	\
	vp = stack - 1;					\
    } while (0)


/* Traces a statement-level event or call */
#define TRACE(event) do {				\
	if (SEE_system.periodic)			\
	    (*SEE_system.periodic)(interp);		\
	interp->try_location = location;		\
	if (interp->trace) 				\
	    (*interp->trace)(interp, location,		\
		ctxt, event);				\
    } while (0)

/* TONUMBER() ensures that the value pointer vp points at a number value.
 * It may use storage at the work pointer! */
#define TONUMBER(vp, work) do {				\
    if (SEE_VALUE_GET_TYPE(vp) != SEE_NUMBER) {		\
	SEE_ToNumber(interp, vp, work);			\
	vp = (work);					\
    }							\
 } while (0)

#define TOOBJECT(vp, work) do {				\
    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT) {		\
	SEE_ToObject(interp, vp, work);			\
	vp = (work);					\
    }							\
 } while (0)

#define NOT_IMPLEMENTED					\
	SEE_error_throw_string(interp, interp->Error,	\
	    STR(not_implemented));

#ifndef NDEBUG
    /*SEE_eval_debug = 2; */
    if (SEE_eval_debug) {
	dprintf("code     = %p\n", co);
	dprintf("ninst    = 0x%x\n", co->ninst);
	dprintf("nlocation= %d\n", co->nlocation);
	dprintf("nvar=      %d\n", co->nvar);
	dprintf("maxstack = %d\n", co->maxstack);
	dprintf("maxargc  = %d\n", co->maxargc);
	if (co->nliteral) {
	    dprintf("-- literals:\n");
	    for (i = 0; i < co->nliteral; i++) {
		dprintf("[%d] ", i);
		dprintv(interp, co->literal + i);
		dprintf("\n");
	    }
	}
	if (co->nfunc) {
	    dprintf("-- functions:\n");
	    for (i = 0; i < co->nfunc; i++) {
	        struct function *f = co->func[i];
		dprintf("[%d] %p nparams=%d", i, f, f->nparams);
		if (f->name) {
		  dprintf(" name=");
		  dprints(f->name);
		}
		if (f->is_empty)
		    dprintf(" is_empty");
		dprintf("\n");
	    }
	}
	dprintf("-- code:\n");
	i = 0;
	while (i < co->ninst)
	    i += disasm(co, i);
	dprintf("--\n");
    }
#endif

    SEE_ASSERT(interp, co->maxstack >= 0);

    stackbottom = SEE_ALLOCA(interp, struct SEE_value, co->maxstack);
    argv = SEE_ALLOCA(interp, struct SEE_value *, co->maxargc);
    blockbottom = SEE_ALLOCA(interp, struct block, co->maxblock);
    blocklevel = 0;

    /* Constants */
    SEE_SET_UNDEFINED(&undefined);
    SEE_SET_OBJECT(&Number, interp->Number);

    SEE_SET_UNDEFINED(res);	    /* C = undefined */

    /* Initialise all vars, and build lookups */
    for (i = 0; i < co->nvar; i++) {
	struct SEE_string *ident;
	SEE_ASSERT(interp, co->var[i] < co->nliteral);
	SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&co->literal[co->var[i]]) == 
			    SEE_STRING);
	ident = co->literal[co->var[i]].u.string;
	if (!SEE_OBJECT_HASPROPERTY(interp, ctxt->variable, ident))
	    SEE_OBJECT_PUT(interp, ctxt->variable, ident, &undefined,
	                        ctxt->varattr);
    }

    pc = co->inst;
    stack = stackbottom;
    scope = ctxt->scope;
    for (;;) {

	SEE_ASSERT(interp, pc >= co->inst);
	SEE_ASSERT(interp, pc < co->inst + co->ninst);

#ifndef NDEBUG
	if (SEE_eval_debug > 1) {
	    dprintf("C=");
	    dprintv(interp, res);
	    dprintf(" stack=");
	    if (stack == stackbottom)
		dprintf("[]");
	    else {
		dprintf("[");
		if (stack < stackbottom + 4)
		    i = 0;
		else {
		    i = stack - (stackbottom + 4);
		    dprintf(" ...");
		}
		for (; i < stack - stackbottom; i++) {
		    dprintf(" ");
		    dprintv(interp, stackbottom + i);
		}
		dprintf(" ]");
	    }
	    dprintf(" blocklevel=%d\n", blocklevel);
	    disasm(co, pc - co->inst);
	}
#endif

	/* Fetch next instruction byte into op,arg and increment pc */
#define FETCH_INST(pc, op, arg)	 do {			    \
	op = *pc++;					    \
	if ((op & INST_ARG_MASK) == INST_ARG_NONE) 	    \
	    arg = 0;					    \
	else if ((op & INST_ARG_MASK) == INST_ARG_BYTE)	    \
	    arg = *pc++;				    \
	else {						    \
	    memcpy(&arg, pc, sizeof arg);		    \
	    pc += sizeof arg;				    \
	}						    \
    } while (0)

	FETCH_INST(pc, op, arg);
	switch (op & INST_OP_MASK) {
	case INST_NOP:
	    break;

	case INST_DUP:
	    TOP(vp);
	    PUSH(up);
	    SEE_VALUE_COPY(up, vp);
	    break;

	case INST_POP:
	    POP0();
	    break;

	case INST_EXCH:
	    SEE_VALUE_COPY(&t, stack - 1);
	    SEE_VALUE_COPY(stack - 1, stack - 2);
	    SEE_VALUE_COPY(stack - 2, &t);
	    break;
	
	case INST_ROLL3:
	    SEE_VALUE_COPY(&t, stack - 1);
	    SEE_VALUE_COPY(stack - 1, stack - 2);
	    SEE_VALUE_COPY(stack - 2, stack - 3);
	    SEE_VALUE_COPY(stack - 3, &t);
	    break;

	case INST_THROW:
	    POP(up);	/* val */
	    TRACE(SEE_TRACE_THROW);
	    SEE_THROW(interp, up);
	    /* NOTREACHED */
	    break;

	case INST_SETC:
	    POP(vp);
	    SEE_VALUE_COPY(res, vp);
	    break;

	case INST_GETC:
	    PUSH(vp);
	    SEE_VALUE_COPY(vp, res);
	    break;

	case INST_THIS:
	    PUSH(vp);
	    SEE_SET_OBJECT(vp, ctxt->thisobj);
	    break;

	case INST_OBJECT:
	    PUSH(vp);
	    SEE_SET_OBJECT(vp, interp->Object);
	    break;

	case INST_ARRAY:
	    PUSH(vp);
	    SEE_SET_OBJECT(vp, interp->Array);
	    break;

	case INST_REGEXP:
	    PUSH(vp);	/* obj */
	    SEE_SET_OBJECT(vp, interp->RegExp);
	    break;

	case INST_REF:
	    POP(up);	/* str */
	    TOP(vp);	/* obj */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_STRING);
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_OBJECT);
	    str = up->u.string;
	    obj = vp->u.object;
	    _SEE_SET_REFERENCE(vp, obj, str);
	    break;

	case INST_GETVALUE:
	    TOP(vp);	/* any -> val */
	    GetValue(interp, vp);	    /* [in situ] */
	    break;

	case INST_LOOKUP:
	    TOP(vp);	/* str */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_STRING);
	    str = SEE_intern(interp, vp->u.string);
	    SEE_scope_lookup(interp, scope, str, vp);
	    break;

	case INST_PUTVALUE:
	    POP(up);	/* val */
	    POP(vp);	/* ref */
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_REFERENCE) {
		struct SEE_object *base = vp->u.reference.base;
		struct SEE_string *prop = vp->u.reference.property;
		if (base == NULL)
		    base = interp->Global;
		SEE_OBJECT_PUT(interp, base, SEE_intern(interp, prop), up,
		    arg);
	    } else
		SEE_error_throw_string(interp, interp->ReferenceError,
		    STR(bad_lvalue));
	    break;

	case INST_VREF:
	    SEE_ASSERT(interp, arg >= 0);
	    SEE_ASSERT(interp, arg < co->nvar);
	    PUSH(vp);	/* ref */
	    SEE_ASSERT(interp, co->var[arg] < co->nliteral);
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(&co->literal[co->var[arg]])
				    == SEE_STRING);
	    _SEE_SET_REFERENCE(vp, ctxt->variable, 
		    co->literal[co->var[arg]].u.string);
	    break;

	case INST_DELETE:
	    TOP(vp);	/* any -> bool */
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_REFERENCE) {
		struct SEE_object *base = vp->u.reference.base;
		struct SEE_string *prop = vp->u.reference.property;
		if (base == NULL || 
		    SEE_OBJECT_DELETE(interp, base, SEE_intern(interp, prop)))
			SEE_SET_BOOLEAN(vp, 1);
		else
			SEE_SET_BOOLEAN(vp, 0);
	    } else
		SEE_SET_BOOLEAN(vp, 0);
	    break;

	case INST_TYPEOF:
	    TOP(vp);	/* any -> str */
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_REFERENCE &&
		vp->u.reference.base == NULL) 
		    SEE_SET_STRING(vp, STR(undefined));
	    else {
		struct SEE_string *s;
		GetValue(interp, vp);
		switch (SEE_VALUE_GET_TYPE(vp)) {
		case SEE_UNDEFINED:	s = STR(undefined); break;
		case SEE_NULL:     	s = STR(object);    break;
		case SEE_BOOLEAN:  	s = STR(boolean);   break;
		case SEE_NUMBER:   	s = STR(number);    break;
		case SEE_STRING:   	s = STR(string);    break;
		case SEE_OBJECT:   	s = SEE_OBJECT_HAS_CALL(vp->u.object)
					  ? STR(function)
					  : STR(object);    break;
		default:		s = STR(unknown);
		}
		SEE_SET_STRING(vp, s);
	    }
	    break;

	case INST_TOOBJECT:
	    TOP(vp);	    /* val -> obj */
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT) {
		struct SEE_value tmp;
		SEE_VALUE_COPY(&tmp, vp);
		SEE_ToObject(interp, &tmp, vp);
	    }
	    break;

	case INST_TONUMBER:
	    TOP(vp);	    /* val -> num */
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_NUMBER) {
		struct SEE_value tmp;
		SEE_VALUE_COPY(&tmp, vp);
		SEE_ToNumber(interp, &tmp, vp);
	    }
	    break;

	case INST_TOBOOLEAN:
	    TOP(vp);	    /* val -> bool */
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_BOOLEAN) {
		struct SEE_value tmp;
		SEE_VALUE_COPY(&tmp, vp);
		SEE_ToBoolean(interp, &tmp, vp);
	    }
	    break;

	case INST_TOSTRING:
	    TOP(vp);	    /* val -> str */
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_STRING) {
		struct SEE_value tmp;
		SEE_VALUE_COPY(&tmp, vp);
		SEE_ToString(interp, &tmp, vp);
	    }
	    break;

	case INST_TOPRIMITIVE:
	    TOP(vp);	    /* val -> str */
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_OBJECT) {
		struct SEE_object *obj = vp->u.object;
		SEE_OBJECT_DEFAULTVALUE(interp, obj, NULL, vp);
	    }
	    break;

	case INST_NEG:
	    TOP(vp);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_NUMBER);
	    vp->u.number = -vp->u.number;
	    break;

	case INST_INV:
	    TOP(vp);
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) != SEE_REFERENCE);
	    int32 = SEE_ToInt32(interp, vp);
	    SEE_SET_NUMBER(vp, ~int32);
	    break;

	case INST_NOT:
	    TOP(vp);
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_BOOLEAN);
	    vp->u.boolean = !vp->u.boolean;
	    break;

	case INST_MUL:
	    POP(vp);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_NUMBER);
	    TOP(up);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_NUMBER);
	    number = up->u.number * vp->u.number;
	    SEE_SET_NUMBER(up, number);
	    break;

	case INST_DIV:
	    POP(vp);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_NUMBER);
	    TOP(up);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_NUMBER);
	    number = up->u.number / vp->u.number;
	    SEE_SET_NUMBER(up, number);
	    break;

	case INST_MOD:
	    POP(vp);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_NUMBER);
	    TOP(up);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_NUMBER);
	    number = NUMBER_fmod(up->u.number, vp->u.number);
	    SEE_SET_NUMBER(up, number);
	    break;

	case INST_ADD:
	    POP(vp);	/* prim */
	    TOP(up);	/* prim -> num/str */
	    wp = up;
	    if (SEE_VALUE_GET_TYPE(up) == SEE_STRING ||
		    SEE_VALUE_GET_TYPE(vp) == SEE_STRING)
	    {
		if (SEE_VALUE_GET_TYPE(up) != SEE_STRING)
		    SEE_ToString(interp, up, &u), up = &u;
		if (SEE_VALUE_GET_TYPE(vp) != SEE_STRING)
		    SEE_ToString(interp, vp, &v), vp = &v;
		str = SEE_string_concat(interp,
		    up->u.string, vp->u.string);
		SEE_SET_STRING(wp, str);
	    } else {
		if (SEE_VALUE_GET_TYPE(up) != SEE_NUMBER)
		    SEE_ToNumber(interp, up, &u), up = &u;
		if (SEE_VALUE_GET_TYPE(vp) != SEE_NUMBER)
		    SEE_ToNumber(interp, vp, &v), vp = &v;
		number = up->u.number + vp->u.number;
		SEE_SET_NUMBER(wp, number);
	    }
	    break;

	case INST_SUB:
	    POP(vp);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_NUMBER);
	    TOP(up);	    /* num */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_NUMBER);
	    number = up->u.number - vp->u.number;
	    SEE_SET_NUMBER(up, number);
	    break;

	case INST_LSHIFT:
	    POP(vp);	/* val2 */
	    TOP(up);	/* val1 */
	    int32 = SEE_ToInt32(interp, up) << 
		(SEE_ToUint32(interp, vp) & 0x1f);
	    SEE_SET_NUMBER(up, int32);
	    break;

	case INST_RSHIFT:
	    POP(vp);	/* val2 */
	    TOP(up);	/* val1 */
	    int32 = SEE_ToInt32(interp, up) >> 
		    (SEE_ToUint32(interp, vp) & 0x1f);
	    SEE_SET_NUMBER(up, int32);
	    break;

	case INST_URSHIFT:
	    POP(vp);	/* val2 */
	    TOP(up);	/* val1 */
	    uint32 = SEE_ToUint32(interp, up) >> 
		    (SEE_ToUint32(interp, vp) & 0x1f);
	    SEE_SET_NUMBER(up, uint32);
	    break;

	case INST_LT:
	    POP(vp);	/* y */
	    TOP(up);	/* x */
	    AbstractRelational(interp, up, vp, up);
	    if (SEE_VALUE_GET_TYPE(up) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(up, 0);
	    break;

	case INST_GT:
	    POP(vp);	/* y */
	    TOP(up);	/* x */
	    AbstractRelational(interp, vp, up, up);
	    if (SEE_VALUE_GET_TYPE(up) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(up, 0);
	    break;

	case INST_LE:
	    POP(vp);	/* y */
	    TOP(up);	/* x */
	    AbstractRelational(interp, vp, up, up);
	    if (SEE_VALUE_GET_TYPE(up) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(up, 0);
	    else
		up->u.boolean = !up->u.boolean;
	    break;

	case INST_GE:
	    POP(vp);	/* y */
	    TOP(up);	/* x */
	    AbstractRelational(interp, up, vp, up);
	    if (SEE_VALUE_GET_TYPE(up) == SEE_UNDEFINED)
		SEE_SET_BOOLEAN(up, 0);
	    else
		up->u.boolean = !up->u.boolean;
	    break;

	case INST_INSTANCEOF:
	    POP(vp);	/* val */
	    TOP(up);	/* val */
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(instanceof_not_object));
	    i = SEE_object_instanceof(interp, up, vp->u.object);
	    SEE_SET_BOOLEAN(up, i);
	    break;

	case INST_IN:
	    POP(vp);	/* val */
	    TOP(up);	/* str */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(up) == SEE_STRING);
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(in_not_object));
	    i = SEE_OBJECT_HASPROPERTY(interp, /* [in situ] */
		vp->u.object, SEE_intern(interp, up->u.string));
	    SEE_SET_BOOLEAN(up, i);
	    break;

	case INST_EQ:
	    POP(vp);
	    TOP(up);
	    i = Eq(interp, up, vp);
	    SEE_SET_BOOLEAN(up, i);
	    break;

	case INST_SEQ:
	    POP(vp);
	    TOP(up);
	    i = Seq(up, vp);
	    SEE_SET_BOOLEAN(up, i);
	    break;

	case INST_BAND:
	    POP(vp);	    /* val */
	    TOP(up);	    /* val */
	    int32 = SEE_ToInt32(interp, up) & SEE_ToInt32(interp, vp);
	    SEE_SET_NUMBER(up, int32);
	    break;

	case INST_BXOR:
	    POP(vp);	    /* val */
	    TOP(up);	    /* val */
	    int32 = SEE_ToInt32(interp, up) ^ SEE_ToInt32(interp, vp);
	    SEE_SET_NUMBER(up, int32);
	    break;

	case INST_BOR:
	    POP(vp);	    /* val */
	    TOP(up);	    /* val */
	    int32 = SEE_ToInt32(interp, up) | SEE_ToInt32(interp, vp);
	    SEE_SET_NUMBER(up, int32);
	    break;

	case INST_S_ENUM:
	    POP(vp);	    /* obj */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_OBJECT);
	    block = &blockbottom[blocklevel];
	    block->type = BLOCK_ENUM;
	    block->u.enum_context.props0 =
		block->u.enum_context.props =
		    SEE_enumerate(interp, vp->u.object);
	    block->u.enum_context.obj = vp->u.object;
	    block->u.enum_context.prev = enum_context;
	    blocklevel++;
	    enum_context = &block->u.enum_context;
	    break;

	case INST_S_WITH:
	    POP(vp);	    /* obj */
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_OBJECT);
	    block = &blockbottom[blocklevel];
	    block->type = BLOCK_WITH;
	    block->u.with.next = scope;
	    block->u.with.obj = vp->u.object;
	    scope = &block->u.with;
	    blocklevel++;
	    break;

	/*--------------------------------------------------
	 * Instructions that take one argument
	 */

	case INST_NEW:
	    SEE_ASSERT(interp, stack >= stackbottom + arg + 1);
	    stack -= arg;
	    SEE_ASSERT(interp, arg <= co->maxargc);
	    for (i = 0; i < arg; i++)
		argv[i] = stack + i;
	    POP(vp);        /* obj */
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_UNDEFINED)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(no_such_function));
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(not_a_function));
	    obj = vp->u.object;
	    if (!SEE_OBJECT_HAS_CONSTRUCT(obj))
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(not_a_constructor));
	    PUSH(up);
	    TRACE(SEE_TRACE_CALL);
	    SEE_OBJECT_CONSTRUCT(interp, obj, NULL, arg, argv, up);
	    TRACE(SEE_TRACE_RETURN);
	    break;

	case INST_CALL:
	    SEE_ASSERT(interp, stack >= stackbottom + arg + 1);
	    stack -= arg;
	    SEE_ASSERT(interp, arg <= co->maxargc);
	    for (i = 0; i < arg; i++)
		argv[i] = stack + i;
	    TOP(vp);      /* ref */

	    baseobj = NULL;
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_REFERENCE) {
		baseobj = vp->u.reference.base;
		if (baseobj && IS_ACTIVATION_OBJECT(baseobj))
		    baseobj = NULL;
		GetValue(interp, vp);
	    }
	    if (!baseobj)
		baseobj = interp->Global;
	    if (SEE_VALUE_GET_TYPE(vp) == SEE_UNDEFINED)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(no_such_function));
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_OBJECT)
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(not_a_function));
	    obj = vp->u.object;
	    if (!SEE_OBJECT_HAS_CALL(obj))
		SEE_error_throw_string(interp, interp->TypeError,
		    STR(not_callable));
	    TRACE(SEE_TRACE_CALL);
	    if (obj == interp->Global_eval) {
		struct SEE_context context2;
		memcpy(&context2, ctxt, sizeof context2);
		context2.scope = scope;
		if (arg == 0)
		    SEE_SET_UNDEFINED(vp);
		else if (SEE_VALUE_GET_TYPE(argv[0]) != SEE_STRING)
		    SEE_VALUE_COPY(vp, argv[0]);
		else
		    SEE_context_eval(&context2, argv[0]->u.string, vp);
	    } else 
		SEE_OBJECT_CALL(interp, obj, baseobj, arg, argv, vp);
	    TRACE(SEE_TRACE_RETURN);
	    break;

	/*
	 * Ending one or more blocks
	 */
	case INST_END:
	    new_blocklevel = arg;
    end:
    	    while (new_blocklevel <= blocklevel) {
		if (blocklevel == 0)
		    return;
		blocklevel--;
		block = &blockbottom[blocklevel];
		if (block->type == BLOCK_ENUM) {
		    /* Ending an ENUM block terminates the enumerator */
#ifndef NDEBUG
		    if (SEE_eval_debug)
			dprintf("ending ENUM\n");
#endif
		    SEE_ASSERT(interp, enum_context == &block->u.enum_context);
		    SEE_enumerate_free(interp, enum_context->props0);
		    enum_context = enum_context->prev;
		} else if (block->type == BLOCK_WITH) {
		    /* Ending an WITH block restores the scope chain */
#ifndef NDEBUG
		    if (SEE_eval_debug)
			dprintf("ending WITH\n");
#endif
		    scope = block->u.with.next;
		} else if (block->type == BLOCK_TRYC) {
		    /* Ending a TRYC (catch) block handles a caught 
		     * exception and converts into a WITH block */
#ifndef NDEBUG
		    if (SEE_eval_debug)
			dprintf("ending TRYC\n");
#endif
		    if (block->u.tryc.context.done) {
			SEE_ASSERT(interp, block == try_block);
			try_block = block->u.tryc.last_try_block;
			_SEE_TRY_FINI(interp, block->u.tryc.context);
		    }
		    vp = SEE_CAUGHT(block->u.tryc.context);
		    if (vp) {
			stack = stackbottom + block->u.tryc.stack;
			obj = SEE_Object_new(interp);
			SEE_OBJECT_PUT(interp, obj, block->u.tryc.ident,
			    vp, SEE_ATTR_DONTDELETE);
			pc = co->inst + block->u.tryc.handler;
			/* Convert the block into a WITH */
			block->type = BLOCK_WITH;
			block->u.with.next = scope;
			block->u.with.obj = obj;
			scope = &block->u.with;
			blocklevel++;
			break;
		    } 
		} else if (block->type == BLOCK_TRYF) {
		    /* Ending a TRYF (try-finally) converts into a FINALLY
		     * block and branches to the finally handler */
#ifndef NDEBUG
		    if (SEE_eval_debug)
			dprintf("ending TRYF - running handler\n");
#endif
		    if (block->u.tryf.context.done) {
			SEE_ASSERT(interp, block == try_block);
			try_block = block->u.tryf.last_try_block;
			_SEE_TRY_FINI(interp, block->u.tryf.context);
		    }
		    block->u.tryf.block = new_blocklevel;
		    stack = stackbottom + block->u.tryc.stack;
		    pc = co->inst + block->u.tryf.handler;
		    /* convert the block into a FINALLY */
		    block->type = BLOCK_FINALLY;
		    blocklevel++;
		    break;
		} else if (block->type == BLOCK_FINALLY) {
		    /* Ending a FINALLY block re-throws any
		     * uncaught exceptions, and resumes unwinding
		     * the block stack. */
#ifndef NDEBUG
		    if (SEE_eval_debug)
			dprintf("ending FINALLY\n");
#endif
		    new_blocklevel = block->u.tryf.block;
		    if (SEE_CAUGHT(block->u.tryf.context))
			TRACE(SEE_TRACE_THROW);
		    SEE_DEFAULT_CATCH(interp, block->u.tryf.context);
		}
	    }
	    break;

	/*--------------------------------------------------
	 * Instructions that take an address argument
	 */

	case INST_B_ALWAYS:
	    pc = co->inst + arg;
	    break;

	case INST_B_TRUE:
	    POP(vp);
	    if (SEE_VALUE_GET_TYPE(vp) != SEE_BOOLEAN) {
		SEE_ToBoolean(interp, vp, &v);
		vp = &v;
	    }
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_BOOLEAN);
	    if (vp->u.boolean)
		pc = co->inst + arg;
	    break;

	case INST_B_ENUM:
	    SEE_ASSERT(interp, enum_context != NULL);
	    while (*enum_context->props && !SEE_OBJECT_HASPROPERTY(interp, 
			enum_context->obj, *enum_context->props))
		enum_context->props++;
	    if (*enum_context->props) {
		PUSH(vp);
		SEE_SET_STRING(vp, *enum_context->props);
		pc = co->inst + arg;
		enum_context->props++;
	    }
	    break;

	case INST_S_TRYC:
	    POP(vp);
	    SEE_ASSERT(interp, SEE_VALUE_GET_TYPE(vp) == SEE_STRING);
	    block = &blockbottom[blocklevel];
	    block->type = BLOCK_TRYC;
	    block->u.tryc.handler = arg;
	    block->u.tryc.stack = stack - stackbottom;
	    block->u.tryc.ident = vp->u.string;
	    block->u.tryc.last_try_block = try_block;
	    try_block = block;
	    _SEE_TRY_INIT(interp, try_block->u.tryc.context);
	    try_block->u.tryc.context.done = 1;
	    if (_SEE_TRY_SETJMP(interp, try_block->u.tryc.context)) {
		try_block->u.tryc.context.done = 0;
		block = try_block;
		try_block = block->u.tryc.last_try_block;
		new_blocklevel = (block - blockbottom) + 1;
		goto end;
	    }
	    blocklevel++;
	    break;

	case INST_S_TRYF:
	    block = &blockbottom[blocklevel];
	    block->type = BLOCK_TRYF;
	    block->u.tryf.handler = arg;
	    block->u.tryf.stack = stack - stackbottom;
	    block->u.tryf.block = -1;
	    block->u.tryf.last_try_block = try_block;
	    try_block = block;
	    _SEE_TRY_INIT(interp, try_block->u.tryf.context);
	    try_block->u.tryf.context.done = 1;
	    if (_SEE_TRY_SETJMP(interp, try_block->u.tryf.context)) {
		try_block->u.tryf.context.done = 0;
		block = try_block;
		try_block = block->u.tryf.last_try_block;
		new_blocklevel = (block - blockbottom) + 1;
		goto end;
	    }
	    blocklevel++;
	    break;

	case INST_FUNC:
	    SEE_ASSERT(interp, arg >= 0);
	    SEE_ASSERT(interp, arg < co->nfunc);
	    PUSH(vp);
	    SEE_SET_OBJECT(vp, SEE_function_inst_create(interp,
		co->func[arg], scope));
	    break;

	case INST_LITERAL:
	    SEE_ASSERT(interp, arg >= 0);
	    SEE_ASSERT(interp, arg < co->nliteral);
	    PUSH(vp);
	    SEE_VALUE_COPY(vp, co->literal + arg);
	    break;

	case INST_LOC:
	    SEE_ASSERT(interp, arg >= 0);
	    SEE_ASSERT(interp, arg < co->nlocation);
	    location = co->location + arg;
	    TRACE(SEE_TRACE_STATEMENT);
	    break;

	default:
	    SEE_ASSERT(interp, !"bad instruction");
	}
    }
}

#ifdef notyet
/*
 * A basic block is a sequence of instructions which are always
 * executed in sequence. The first instruction is the only entry point,
 * and the last has at least one exit that is not the 'next instruction'.
 */
struct basic_block {
	unsigned char *pc;		/* instruction base */
	SEE_int32_t length;		/* instruction segment */
	struct basic_block *out[2];	/* egress links (NULL if unused) */
	unsigned int incoming;		/* number of incoming edges */
	struct basic_block *a_next;	/* linked list #a */
};

/* Constructs a graph of basic blocks. */
static struct basic_block *
build_basic_blocks(co)
	struct code1 *co;
{
	unsigned char op, *pc;
	const unsigned char *endpc = co->inst + co->ninst;
	SEE_int32_t arg;

	/*
	 * Step 1: count the number of basic blocks by finding
	 * instructions that mark the end of a block, or the
	 * branch targets that would start a block.
	 */
	pc = co->inst;
	while (pc < endpc) {
	    FETCH_INST(pc, op, arg);
	    switch (op) {

	    /* Branch instructions */
	    case INST_B_ALWAYS:
	    case INST_B_TRUE:
	    case INST_B_ENUM:
	    case INST_END:
		; /* TBD */
	    }
	}
}
#endif

#ifndef NDEBUG
static SEE_int32_t
disasm(co, pc)
	struct code1 *co;
	SEE_int32_t pc;
{
	int i, len;
	unsigned char op;
	SEE_int32_t arg = 0;
	const unsigned char *base = co->inst;

	dprintf("%4x: ", pc);

	op = base[pc];
	if ((op & INST_ARG_MASK) == INST_ARG_NONE) {
	    arg = 0;
	    len = 1;
	} else if ((op & INST_ARG_MASK) == INST_ARG_BYTE) {
	    arg = base[pc + 1];
	    len = 2;
	} else {
	    memcpy(&arg, base + pc + 1, sizeof arg);
	    len = 1 + sizeof arg;
	}

	for (i = 0; i < 1 + sizeof arg; i++)
	    if (i < len)
		dprintf("%02x ", base[pc + i]);
	    else
		dprintf("   ");

	switch (op & INST_OP_MASK) {
	case INST_NOP:		dprintf("NOP"); break;
	case INST_DUP:		dprintf("DUP"); break;
	case INST_POP:		dprintf("POP"); break;
	case INST_EXCH:		dprintf("EXCH"); break;
	case INST_ROLL3:	dprintf("ROLL3"); break;
	case INST_THROW:	dprintf("THROW"); break;
	case INST_SETC:		dprintf("SETC"); break;
	case INST_GETC:		dprintf("GETC"); break;
	case INST_THIS:		dprintf("THIS"); break;
	case INST_OBJECT:	dprintf("OBJECT"); break;
	case INST_ARRAY:	dprintf("ARRAY"); break;
	case INST_REGEXP:	dprintf("REGEXP"); break;
	case INST_REF:		dprintf("REF"); break;
	case INST_GETVALUE:	dprintf("GETVALUE"); break;
	case INST_LOOKUP:	dprintf("LOOKUP"); break;
	case INST_PUTVALUE:	if (len == 1) {
				    dprintf("PUTVALUE"); 
				    break;
				}
				dprintf("PUTVALUE,%-4d  ;", arg);
				if (arg & SEE_ATTR_READONLY)
				    dprintf(" ReadOnly");
				if (arg & SEE_ATTR_DONTENUM)
				    dprintf(" DontEnum");
				if (arg & SEE_ATTR_DONTDELETE)
				    dprintf(" DontDelete");
				if (arg & SEE_ATTR_INTERNAL)
				    dprintf(" Internal");
				break;
	case INST_VREF:		dprintf("VREF,%-4d      ; ", arg);
				if (arg >= 0 && arg < co->nvar &&
				    co->var[arg] < co->nliteral &&
				    SEE_VALUE_GET_TYPE(co->literal +
					co->var[arg]) == SEE_STRING)
				    dprints(co->literal[co->var[arg]].u.string);
				else
				    dprintf("<invalid!>");
				break;
	case INST_DELETE:	dprintf("DELETE"); break;
	case INST_TYPEOF:	dprintf("TYPEOF"); break;
	case INST_TOOBJECT:	dprintf("TOOBJECT"); break;
	case INST_TONUMBER:	dprintf("TONUMBER"); break;
	case INST_TOBOOLEAN:	dprintf("TOBOOLEAN"); break;
	case INST_TOSTRING:	dprintf("TOSTRING"); break;
	case INST_TOPRIMITIVE:	dprintf("TOPRIMITIVE"); break;
	case INST_NEG:		dprintf("NEG"); break;
	case INST_INV:		dprintf("INV"); break;
	case INST_NOT:		dprintf("NOT"); break;
	case INST_MUL:		dprintf("MUL"); break;
	case INST_DIV:		dprintf("DIV"); break;
	case INST_MOD:		dprintf("MOD"); break;
	case INST_ADD:		dprintf("ADD"); break;
	case INST_SUB:		dprintf("SUB"); break;
	case INST_LSHIFT:	dprintf("LSHIFT"); break;
	case INST_RSHIFT:	dprintf("RSHIFT"); break;
	case INST_URSHIFT:	dprintf("URSHIFT"); break;
	case INST_LT:		dprintf("LT"); break;
	case INST_GT:		dprintf("GT"); break;
	case INST_LE:		dprintf("LE"); break;
	case INST_GE:		dprintf("GE"); break;
	case INST_INSTANCEOF:	dprintf("INSTANCEOF"); break;
	case INST_IN:		dprintf("IN"); break;
	case INST_EQ:		dprintf("EQ"); break;
	case INST_SEQ:		dprintf("SEQ"); break;
	case INST_BAND:		dprintf("BAND"); break;
	case INST_BXOR:		dprintf("BXOR"); break;
	case INST_BOR:		dprintf("BOR"); break;
	case INST_S_ENUM:	dprintf("S_ENUM"); break;
	case INST_S_WITH:	dprintf("S_WITH"); break;

	case INST_NEW:		dprintf("NEW,%d", arg); break;
	case INST_CALL:		dprintf("CALL,%d", arg); break;
	case INST_END:		dprintf("END,%d", arg); break;

	case INST_B_ALWAYS:	dprintf("B_ALWAYS,0x%x", arg); break;
	case INST_B_TRUE:	dprintf("B_TRUE,0x%x", arg); break;
	case INST_B_ENUM:	dprintf("B_ENUM,0x%x", arg); break;
	case INST_S_TRYC:	dprintf("S_TRYC,0x%x", arg); break;
	case INST_S_TRYF:	dprintf("S_TRYF,0x%x", arg); break;

	case INST_FUNC:		dprintf("FUNC,%-4d      ;", arg);
				if (arg >= 0 && arg < co->nfunc) {
				    struct function *f = co->func[arg];
				    dprintf(" %p", f);
				    if (f->name) {
				      dprintf(" name=");
				      dprints(f->name);
				    }
				    dprintf(" nparams=%d", f->nparams);
				    if (f->is_empty)
					dprintf(" is_empty");
				} else
				    dprintf(" <invalid!>");
				break;
	case INST_LITERAL:	
				dprintf("LITERAL,%-4d   ; ", arg);
				if (arg >= 0 && arg < co->nliteral)
				    dprintv(co->code.interpreter, 
					co->literal + arg);
				else
				    dprintf("<invalid!>");
				break;
	case INST_LOC:		dprintf("LOC,%-4d       ; ", arg); 
				if (arg >= 0 && arg < co->nlocation) {
				    dprintf("\"");
				    dprints(co->location[arg].filename);
				    dprintf(":%d\"", co->location[arg].lineno);
				} else
				    dprintf("<invalid!>");
				break;
	default:		dprintf("??? <%02x>,%d", op, arg);
	}
	dprintf("\n");

	return len;
}
#endif
