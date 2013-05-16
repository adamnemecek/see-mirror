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
/* $Id: input_file.c 1282 2007-09-13 22:26:35Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include <see/mem.h>
#include <see/string.h>
#include <see/type.h>
#include <see/input.h>
#include <see/interpreter.h>

#include "unicode.h"

/*
 * An input filter for a file, opened with C's standard IO.
 *
 * 7-bit ascii is assumed, unless a byte-order mark is seen at the 
 * beginning of the file, or overridden with the right argument.
 *
 * NB If an end-of-file is detected while decoding some bytes,
 * EOF is returned instead of returning a SEE_INPUT_BADCHAR.
 */

#define LOOKAHEAD_MAX	4

struct input_file {
	struct SEE_input	inp;
	FILE *		file;
	unsigned char	lookahead_buf[LOOKAHEAD_MAX];
	unsigned char	*lookahead_pos;
	int		lookahead_len;
};

static int getbyte(struct input_file *);
static SEE_unicode_t ucs32be_next(struct SEE_input *);
static SEE_unicode_t ucs32le_next(struct SEE_input *);
static SEE_unicode_t utf16be_next(struct SEE_input *);
static SEE_unicode_t utf16le_next(struct SEE_input *);
static SEE_unicode_t utf8_next(struct SEE_input *);
static SEE_unicode_t ascii_next(struct SEE_input *);
static void input_file_close(struct SEE_input *);

static struct SEE_inputclass 
   ucs32be_class = { ucs32be_next, input_file_close },
   ucs32le_class = { ucs32le_next, input_file_close },
   utf8_class =    { utf8_next,    input_file_close },
   utf16be_class = { utf16be_next, input_file_close },
   utf16le_class = { utf16le_next, input_file_close },
   ascii_class =   { ascii_next,   input_file_close };

static struct bomtab {
	int	len;
	unsigned char match[4];
	struct SEE_inputclass *inputclass;
	const char *label;
} bomtab[] = {
	{ 4,	{ 0x00, 0x00, 0xfe, 0xff },	&ucs32be_class, "UCS-32BE" },
	{ 4,	{ 0xff, 0xfe, 0x00, 0x00 },	&ucs32le_class, "UCS-32LE" },
	{ 3,	{ 0xef, 0xbb, 0xbf },		&utf8_class,    "UTF-8"    },
	{ 2,	{ 0xfe, 0xff },			&utf16be_class, "UTF-16BE" },
	{ 2,	{ 0xff, 0xfe },			&utf16le_class, "UTF-16LE" },
	{ 0,    { 0 },				&ascii_class,   "ASCII"    },
	{ 0,    { 0 },				&ascii_class,   NULL }
};

/* Return next byte, or EOF (-1) */
static int
getbyte(inpf)
	struct input_file *inpf;
{
	int ch;

	if (inpf->lookahead_len) {
		ch = *inpf->lookahead_pos++;
		inpf->lookahead_len--;
	} else
		ch = fgetc(inpf->file);
	return ch;
}

/* UCS-32 big endian */
static SEE_unicode_t
ucs32be_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next;
	int ch, i;

	next = inpf->inp.lookahead;
	inpf->inp.lookahead = 0;
	inpf->inp.eof = 0;
	for (i = 0; i < 4; i++) {
	    ch = getbyte(inpf);
	    if (ch == EOF) {
		inpf->inp.eof = 1;
		break;
	    } else
		inpf->inp.lookahead |= (ch & 0xff) >> ((3-i) * 8);
	}
	if (inpf->inp.lookahead > _UNICODE_MAX)
		inpf->inp.lookahead = SEE_INPUT_BADCHAR;
	return next;
}

/* UCS-32 little endian */
static SEE_unicode_t
ucs32le_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next;
	int ch, i;

	next = inpf->inp.lookahead;
	inpf->inp.lookahead = 0;
	inpf->inp.eof = 0;
	for (i = 0; i < 4; i++) {
	    ch = getbyte(inpf);
	    if (ch == EOF) {
		inpf->inp.eof = 1;
		break;
	    } else 
		inpf->inp.lookahead |= (ch & 0xff) >> (i * 8);
	}
	if (inpf->inp.lookahead > _UNICODE_MAX)
		inpf->inp.lookahead = SEE_INPUT_BADCHAR;
	return next;
}

/* UTF-16 big endian */
static SEE_unicode_t
utf16be_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next;
	SEE_char_t u1, u2;
	int ch;

	/* RFC2781 */
	next = inpf->inp.lookahead;
	inpf->inp.eof = 1;
	ch = getbyte(inpf);
	if (ch != EOF) {
	    u1 = ch & 0xff;
	    ch = getbyte(inpf);
	    if (ch != EOF) {
		u1 |= (ch & 0xff) << 8;
		inpf->inp.eof = 0;
		inpf->inp.lookahead = u1;
		if ((u1 & 0xfc00) == 0xd800) {
		    ch = getbyte(inpf);
		    inpf->inp.eof = 1;
		    if (ch != EOF) {
		        u2 = ch & 0xff;
		        ch = getbyte(inpf);
		        if (ch != EOF) {
			    inpf->inp.eof = 0;
			    u2 |= (ch & 0xff) << 8;
			    if ((u2 & 0xfc00) == 0xdc00) {
				inpf->inp.lookahead =
					((u1 & 0x3ff) << 10 |
					(u2 & 0x3ff)) + 0x10000;
			    } else
				inpf->inp.lookahead =
					SEE_INPUT_BADCHAR;
			}
		    }
		}
	    }
	}
	return next;
}

/* UTF-16 little endian */
static SEE_unicode_t
utf16le_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next;
	SEE_char_t u1, u2;
	int ch;

	/* RFC2781 */
	next = inpf->inp.lookahead;
	inpf->inp.eof = 1;
	ch = getbyte(inpf);
	if (ch != EOF) {
	    u1 = (ch & 0xff) << 8;
	    ch = getbyte(inpf);
	    if (ch != EOF) {
		u1 |= ch & 0xff;
		inpf->inp.eof = 0;
		inpf->inp.lookahead = u1;
		if ((u1 & 0xfc00) == 0xd800) {
		    ch = getbyte(inpf);
		    inpf->inp.eof = 1;
		    if (ch != EOF) {
		        u2 = (ch & 0xff) << 8;
		        ch = getbyte(inpf);
		        if (ch != EOF) {
			    inpf->inp.eof = 0;
			    u2 |= ch & 0xff;
			    if ((u2 & 0xfc00) == 0xdc00) {
				inpf->inp.lookahead =
					((u1 & 0x3ff) << 10 |
					(u2 & 0x3ff)) + 0x10000;
			    } else
				inpf->inp.lookahead =
					SEE_INPUT_BADCHAR;
			}
		    }
		}
	    }
	}
	return next;
}

/* UTF-8 */
static SEE_unicode_t
utf8_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next, c;
	int ch, bytes, i;

	static unsigned char mask[] = { 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
	static SEE_unicode_t safe[] = { 0, 0x80, 0x800, 0x10000, 0x200000,
						0x4000000, 0x80000000 };

	/* RFC 2279 */
	next = inpf->inp.lookahead;
	ch = getbyte(inpf);
	if (ch == EOF) {
		inpf->inp.eof = 1;
	} else if ((ch & 0x80) == 0) {
		inpf->inp.lookahead = ch;
	} else {
		for (bytes = 1; bytes < 6; bytes++)
		    if ((ch & mask[bytes]) == mask[bytes-1])
			break;
		if (bytes < 6) {
		    c = (ch & ~mask[bytes]);
		    for (i = 0; i < bytes; i++) {
			ch = getbyte(inpf);
			if (ch == EOF) {
			    inpf->inp.eof = 1;
			    break;
			}
			c = (c << 6) | (ch & 0x3f);
		    }
		    if (c > _UNICODE_MAX)
			c = SEE_INPUT_BADCHAR;
		    else if (c < safe[bytes] && 
		    	    !(inpf->inp.interpreter->compatibility &
		                             SEE_COMPAT_UTF_UNSAFE))
			c = SEE_INPUT_BADCHAR;
		} else
		    c = SEE_INPUT_BADCHAR;
		inpf->inp.lookahead = c;
	}
	return next;
}

/* 7-bit ascii */
static SEE_unicode_t
ascii_next(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;
	SEE_unicode_t next;
	int ch;

	next = inpf->inp.lookahead;
	ch = getbyte(inpf);
	if (ch == EOF) {
		inpf->inp.eof = 1;
	} else {
		if (ch & 0x80) 
		    inpf->inp.lookahead = SEE_INPUT_BADCHAR;
		else
		    inpf->inp.lookahead = ch & 0x7f;
		inpf->inp.eof = 0;
	}
	return next;
}

static void
input_file_close(inp)
	struct SEE_input *inp;
{
	struct input_file *inpf = (struct input_file *)inp;

	fclose(inpf->file);
}

/*
 * Construct an input filter for the alread-opened file 'file',
 * with ASCII-encoded filename *filename. If label is non-null,
 * it is used to determine the file's content. Otherwise, a byte-order
 * mark is sought and if one isn't found, falls back to assume
 * 7bit ascii.
 */
struct SEE_input *
SEE_input_file(interp, file, filename, label)
	struct SEE_interpreter *interp;
	FILE *file;
	const char *filename;
	const char *label;
{
	struct input_file *inpf;
	int ch, i;
	struct bomtab *bt;

	inpf = SEE_NEW(interp, struct input_file);
	inpf->inp.interpreter = interp;
	inpf->file = file;
	if (filename)
		inpf->inp.filename = SEE_string_sprintf(interp, 
			"%s", filename);
	else
		inpf->inp.filename = NULL;
	inpf->inp.first_lineno = 1;

	inpf->lookahead_len = 0;
	inpf->lookahead_pos = &inpf->lookahead_buf[0];
	inpf->inp.inputclass = &ascii_class;

	if (label && *label) {
	    for (bt = bomtab; bt->label; bt++) 
		if (strcmp(bt->label, label) == 0) {
		    inpf->inp.inputclass = bt->inputclass;
		    break;
		}
	    if (!bt->label) { /* XXX throw error: unknown encoding */ }
	} else
	    /*
	     * Search for and match any initial byte order mark.
	     * This is where our lookahead buffer comes in handy.
	     */
	    for (bt = bomtab; ; bt++) {
		for (i = 0; i < bt->len; i++) {
		    if (inpf->lookahead_len <= i) {
			ch = fgetc(file);
			if (ch == EOF) 
			    break;
			inpf->lookahead_buf[inpf->lookahead_len++] = ch;
		    }
		    if (inpf->lookahead_buf[i] != bt->match[i])
			break;
		}
		if (i == bt->len) {
		    inpf->inp.inputclass = bt->inputclass;
		    /* Strip the byte order mark */
		    inpf->lookahead_pos += i;
		    inpf->lookahead_len -= i;
		    break;
		}
	    }

	SEE_INPUT_NEXT((struct SEE_input *)inpf);	/* prime the buffer */
	return (struct SEE_input *)inpf;
}
