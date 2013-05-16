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
/* $Id: input_utf8.c 1282 2007-09-13 22:26:35Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include <see/mem.h>
#include <see/type.h>
#include <see/input.h>
#include <see/interpreter.h>

#include "unicode.h"

/*
 * An input for UTF8 encoded C strings. (i.e. nul-terminated byte arrays)
 *
 * Useful when the host application wants to execute some literal code
 * which is either 7-bit ASCII or UTF-8. A byte order mark is not expected.
 * NOTE: Any (say) Latin-1 encodings have to be converted to UTF-8 first!
 * Ref: RFC2279
 */

/* A character representing some invalid input; */

static SEE_unicode_t input_utf8_next(struct SEE_input *);
static void         input_utf8_close(struct SEE_input *);

static struct SEE_inputclass input_utf8_class = {
	input_utf8_next,
	input_utf8_close
};

struct input_utf8 {
	struct SEE_input	inp;
	const unsigned char *	s;
};

static SEE_unicode_t
input_utf8_next(inp)
	struct SEE_input *inp;
{
	struct input_utf8 *inpu = (struct input_utf8 *)inp;
	SEE_unicode_t next, c;
	int i, j, bytes;
	static unsigned char mask[] = { 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
	static SEE_unicode_t safe[] = { 0, 0x80, 0x800, 0x10000, 0x200000, 
					0x4000000, 0x80000000 };

	next = inpu->inp.lookahead;

	/*
	 * Decode a UTF8 string, returning EOF if we reach a nul character.
	 * This is a non-strict decoder.
	 */
	if (*inpu->s == '\0') {
		inpu->inp.eof = 1;
	} else if ((*inpu->s & 0x80) == 0) {
		inpu->inp.lookahead = *inpu->s++;
		inpu->inp.eof = 0;
	} else {
		for (bytes = 1; bytes < 6; bytes++)
		    if ((*inpu->s & mask[bytes]) == mask[bytes - 1])
			break;
		if (bytes < 6) {
		    c = *inpu->s++ & ~mask[bytes];
		    for (i = bytes, j = 0; i--; j++) {
			if ((*inpu->s & 0xc0) != 0x80) {
			     goto bad;
			}
			c = (c << 6) | (*inpu->s++ & 0x3f);
		    }
		    if (c > _UNICODE_MAX)
			    inpu->inp.lookahead = SEE_INPUT_BADCHAR;
		    else if (c < safe[bytes] && 
		        !(inpu->inp.interpreter->compatibility & 
			 SEE_COMPAT_UTF_UNSAFE))
			    inpu->inp.lookahead = SEE_INPUT_BADCHAR;
		    else
			    inpu->inp.lookahead = c;

		    inpu->inp.eof = 0;
		} else {
		bad:
		    inpu->inp.lookahead = SEE_INPUT_BADCHAR;
		    inpu->inp.eof = 0;
		    while ((*inpu->s & 0x80))
			inpu->s++;
		} 
	}
	return next;
}

static void
input_utf8_close(inp)
	struct SEE_input *inp;
{
	/* struct input_utf8 *inpu = (struct input_utf8 *)inp; */
	/* nothing */
}

struct SEE_input *
SEE_input_utf8(interp, s)
	struct SEE_interpreter *interp;
	const char *s;
{
	struct input_utf8 *inpu;

	inpu = SEE_NEW(interp, struct input_utf8);
	inpu->inp.interpreter = interp;
	inpu->inp.inputclass = &input_utf8_class;
	inpu->inp.filename = NULL;
	inpu->inp.first_lineno = 1;
	inpu->s = (const unsigned char *)s;
	SEE_INPUT_NEXT((struct SEE_input *)inpu);	/* prime */
	return (struct SEE_input *)inpu;
}
