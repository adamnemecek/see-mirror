/* Copyright (c) 2006, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_code_
#define _SEE_h_code_

struct SEE_interpreter;
struct SEE_value;
struct SEE_context;
struct SEE_string;
struct function;

/*
 * A code stream generator for executing ECMAscript functions.
 *
 * See ../doc/BYTECODE.txt for bytecode details.
 *
 * This header does not define the byte code format, instead
 * it defines an interface to a code generator that is free
 * to define the code stream encasulated by struct SEE_code.
 *
 * This interface allows the parser to generate the code
 * stream in a single pass with branch patching.
 */

struct SEE_code;

/* Operators that take a simple integer as an operand */
enum SEE_code_op1 {
	SEE_CODE_NEW,			/* obj any1..anyn | obj */
	SEE_CODE_CALL, 			/* any any1..anyn | val */
	SEE_CODE_END,			/*              - | -   */
	SEE_CODE_VREF, 			/*                | ref */
	SEE_CODE_PUTVALUEA		/*        ref val | -   */
};

/* Generic operators that work on the stack or virtual registers */
enum SEE_code_op0 {
	SEE_CODE_NOP,			/*           - | -          */
	SEE_CODE_DUP,			/*         any | any any    */
	SEE_CODE_POP,			/*         any | -	    */
	SEE_CODE_EXCH,			/*         a b | b a	    */
	SEE_CODE_ROLL3,			/*       a b c | c a b      */
	SEE_CODE_THROW,			/*         val | ?          */
	SEE_CODE_SETC,			/*         any | -          */
	SEE_CODE_GETC,			/*           - | any        */
	SEE_CODE_THIS,			/*           - | obj	    */
	SEE_CODE_OBJECT,		/*           - | obj	    */
	SEE_CODE_ARRAY,			/*           - | obj	    */
	SEE_CODE_REGEXP,		/*           - | obj	    */
	SEE_CODE_REF,			/*     obj str | ref	    */
	SEE_CODE_GETVALUE,		/*         ref | val	    */
	SEE_CODE_LOOKUP,		/*         str | ref	    */
	SEE_CODE_PUTVALUE,		/*     ref val | -	    */
	SEE_CODE_DELETE,		/*         any | bool	    */
	SEE_CODE_TYPEOF,		/*         any | str	    */

	SEE_CODE_TOOBJECT,		/*	   val | obj	    */
	SEE_CODE_TONUMBER,		/*	   val | num	    */
	SEE_CODE_TOBOOLEAN,		/*	   val | bool	    */
	SEE_CODE_TOSTRING,		/*	   val | str	    */
	SEE_CODE_TOPRIMITIVE,		/*	   val | prim	    */

	SEE_CODE_NEG,			/*         num | num        */
	SEE_CODE_INV,			/*         num | num        */
	SEE_CODE_NOT,			/*        bool | bool       */
	SEE_CODE_MUL,			/*     num num | num        */
	SEE_CODE_DIV,			/*     num num | num        */
	SEE_CODE_MOD,			/*     num num | num        */
	SEE_CODE_ADD,			/*   prim prim | prim       */
	SEE_CODE_SUB,			/*     num num | num        */

	SEE_CODE_LSHIFT,		/*     val val | num        */
	SEE_CODE_RSHIFT,		/*     val val | num        */
	SEE_CODE_URSHIFT,		/*     val val | num        */

	SEE_CODE_LT,			/*     val val | bool       */
	SEE_CODE_GT,			/*     val val | bool       */
	SEE_CODE_LE,			/*     val val | bool       */
	SEE_CODE_GE,			/*     val val | bool       */
	SEE_CODE_INSTANCEOF,		/*     val val | bool       */
	SEE_CODE_IN,			/*     val val | bool       */
	SEE_CODE_EQ,			/*     val val | bool	    */
	SEE_CODE_SEQ,			/*     val val | bool	    */

	SEE_CODE_BAND,			/*     val val | num        */
	SEE_CODE_BXOR,			/*     val val | num        */
	SEE_CODE_BOR,			/*     val val | num        */

	SEE_CODE_S_ENUM,		/*         obj | -          */
	SEE_CODE_S_WITH 		/*         obj | -          */

};

/* Branch instructions that have a single instruction-address operand.
 * (opa stands for 'operand address') */
enum SEE_code_opa {
	SEE_CODE_B_ALWAYS,		/*           - | -          */
	SEE_CODE_B_TRUE,		/*        bool | -          */
	SEE_CODE_B_ENUM, 		/*           - | [- | str]  */
	SEE_CODE_S_TRYC,  		/*           - | -          */
	SEE_CODE_S_TRYF  		/*           - | -          */
};

/* A branch target address in a code stream */
typedef void *SEE_code_addr_t;
typedef void *SEE_code_patchable_t;

#define SEE_CODE_NO_ADDRESS ((SEE_code_addr_t)0)

/* Method table for the code stream */
struct SEE_code_class {
	const char *name;		/* Name of the code stream class */

	/* Generates an operator that manipulates the stack */
	void	(*gen_op0)(struct SEE_code *co, enum SEE_code_op0 op);

	/* Generates a call/construct instruction with argc values on stack */
	void	(*gen_op1)(struct SEE_code *co, enum SEE_code_op1 op, int n);

	/* Generates an instruction to put a value on the stack: - | val */
	void	(*gen_literal)(struct SEE_code *co, const struct SEE_value *);

	/* Generates a function object bound to the current context,
	 * similar to SEE_function_inst_create() */
	void	(*gen_func)(struct SEE_code *co, struct function *f);

	/* Generates a location update instruction */
	void	(*gen_loc)(struct SEE_code *co, struct SEE_throw_location *loc);

	/* Adds a variable */
	unsigned int (*gen_var)(struct SEE_code *co, struct SEE_string *ident);

	/* Generates an instruction referring to an address. 
	 * If patchp is not NULL, then addr is ignored and a patch ref 
	 * is stored in *patchp for later use by the patch method.
	 */
	void  	(*gen_opa)(struct SEE_code *co, enum SEE_code_opa op,
			SEE_code_patchable_t *patchp, SEE_code_addr_t addr);

	/* Returns the next instruction's address; used for patching */
	SEE_code_addr_t (*here)(struct SEE_code *co);

	/* Patches a previous instruction's address argument */
	void	(*patch)(struct SEE_code *co, SEE_code_patchable_t patch,
			SEE_code_addr_t addr);

	/* Indicates the maximum stack and blocks sizes. (-1 means unknown) */
	void	(*maxstack)(struct SEE_code *co, int);
	void	(*maxblock)(struct SEE_code *co, int);

	/* Indicates no more code. Allows generator to cleanup/optimize. */
	void	(*close)(struct SEE_code *co);

	/* Executes the code in the given context. The result is the
	 * last content of the C register. */
	void	(*exec)(struct SEE_code *co, struct SEE_context *ctxt,
	        	struct SEE_value *res);
};

/* Public fields of the code context superclass */
struct SEE_code {
	struct SEE_code_class *code_class;
	struct SEE_interpreter *interpreter;
};

struct SEE_code *_SEE_code1_alloc(struct SEE_interpreter *interp);

#endif /* _SEE_h_code_ */
