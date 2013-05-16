/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: array.h 725 2004-10-24 00:55:49Z d $ */

#ifndef _SEE_h_code1_
#define _SEE_h_code1_

/*
 * 'code1': simple bytecode interpreter
 *
 * Bytecode layout: 
 *
 *  Instructions are single bytes, sometimes followed by an integer.
 *  The top two bits indicate how many bytes of the integer follow:
 *
 *	0 0	- no integer follows (ARG_NONE)
 *	0 1	- a single unsigned byte follows (ARG_BYTE)
 *	1 0	- a signed 32 bit value (native endian) (ARG_WORD)
 *	1 1	- reserved
 *
 */

/* Instruction byte argument descriptor */
#define INST_ARG_MASK	       0xc0
#define INST_ARG_NONE		0x00
#define INST_ARG_BYTE		0x40
#define INST_ARG_WORD		0x80

/* Instruction byte codes */
#define INST_OP_MASK	       0x3f
#define INST_NOP		0x00
#define INST_DUP		0x01
#define INST_POP		0x02
#define INST_EXCH		0x03
#define INST_ROLL3		0x04
#define INST_THROW		0x05
#define INST_SETC 		0x06
#define INST_GETC 		0x07
#define INST_THIS		0x08
#define INST_OBJECT		0x09
#define INST_ARRAY		0x0a
#define INST_REGEXP		0x0b
#define INST_REF		0x0c
#define INST_GETVALUE		0x0d
#define INST_LOOKUP		0x0e
#define INST_PUTVALUE		0x0f
#define INST_VREF  		0x10
                             /* 0x11 was INST_VAR */
#define INST_DELETE		0x12
#define INST_TYPEOF		0x13

#define INST_TOOBJECT		0x14
#define INST_TONUMBER		0x15
#define INST_TOBOOLEAN		0x16
#define INST_TOSTRING		0x17
#define INST_TOPRIMITIVE	0x18

#define INST_NEG		0x19
#define INST_INV		0x1a
#define INST_NOT		0x1b
#define INST_MUL		0x1c
#define INST_DIV		0x1d
#define INST_MOD		0x1e
#define INST_ADD		0x1f
#define INST_SUB		0x20

#define INST_LSHIFT		0x21
#define INST_RSHIFT		0x22
#define INST_URSHIFT		0x23

#define INST_LT			0x24
#define INST_GT			0x25
#define INST_LE			0x26
#define INST_GE			0x27

#define INST_INSTANCEOF		0x28
#define INST_IN			0x29
#define INST_EQ			0x2a
#define INST_SEQ		0x2b

#define INST_BAND		0x2c
#define INST_BXOR		0x2d
#define INST_BOR		0x2e

#define INST_S_ENUM		0x2f
#define INST_S_WITH		0x30

#define INST_NEW		0x31
#define INST_CALL	    	0x32
#define INST_END 	    	0x33

#define INST_B_ALWAYS		0x34
#define INST_B_TRUE		0x35
#define INST_B_ENUM		0x36
#define INST_S_TRYC		0x37
#define INST_S_TRYF		0x38

#define INST_FUNC		0x39
#define INST_LITERAL		0x3a
#define INST_LOC		0x3b

                             /* 0x3c unused */
                             /* 0x3d unused */
                             /* 0x3e unused */
                             /* 0x3f unused */
                             /* ---- don't exceed 0x3f! */

struct SEE_code;
struct SEE_value;
struct SEE_throw_location;
struct SEE_interpreter;

struct code1 {
    struct SEE_code	 code;
    unsigned char	*inst;
    struct SEE_value	*literal;
    struct SEE_throw_location *location;
    struct function    **func;
    unsigned int        *var;
    unsigned int	 ninst, nliteral, nlocation, nfunc, nvar;
    struct SEE_growable	 ginst, gliteral, glocation, gfunc, gvar;
    int	maxstack, maxblock, maxargc;
};

#endif /* _SEE_h_code1_ */
