/*
 * Copyright (c) 2008
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

/*
 * Interface to the (much faster) PCRE library.
 *
 * IMPORTANT NOTE: some versions of the the PCRE library are not wholly 
 * compatible with the ECMAScript definition of regular expressions. 
 * Besides allowing extra atoms, the behaviour of some back references, 
 * and of some kinds of capture groups may differ markedly from ECMA-262. 
 *
 * Also, PCRE libraries are sometimes not compiled with Unicode support. 
 * This interface module will try to run the PCRE functions in ASCII mode,
 * but will throw an exception if the PCRE library was not compiled with 
 * UTF-8 when non-ASCII SEE_strings are used.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <pcre.h>

#include <see/mem.h>
#include <see/error.h>
#include <see/string.h>
#include <see/system.h>

#include "regex.h"
#include "dprint.h"

#ifndef NDEBUG
int SEE_regex_debug;
#endif

struct regex_pcre {
	struct regex	regex;
	int		flags;
	pcre *		pcre;
	pcre_extra *	pcre_extra;
	int		ncaptures;
	int		utf8;
	/* Cache of last text searched, converted to a C-string: */
	struct SEE_string *text_string;
	int		text_len;
	char *		text_data;
};

/* Cast from struct regex to struct regex_pcre */
#define REGEX_CAST(r) ((struct regex_pcre *)(r))

/* Prototypes */
static void regex_pcre_init(void);
static struct regex *regex_pcre_parse(struct SEE_interpreter *, 
	struct SEE_string *, int);
static int regex_pcre_count_captures(struct regex *);
static int regex_pcre_get_flags(struct regex *);
static int regex_pcre_match(struct SEE_interpreter *, struct regex *,
	struct SEE_string *, unsigned int, struct capture *);

const struct SEE_regex_engine _SEE_pcre_regex_engine = {
	regex_pcre_init,
	regex_pcre_parse,
	regex_pcre_count_captures,
	regex_pcre_get_flags,
	regex_pcre_match
};

/* Called by PCRE to allocate memory */
static void *
regex_pcre_malloc(size)
	size_t size;
{
	/* XXX don't know when to use string allocation */
	return SEE_malloc(0, size);
}

/* Called by PCRE to free memory */
static void
regex_pcre_free(ptr)
	void *ptr;
{
}

/* Called periodically by pcre when processing a regex */
static int 
regex_pcre_callout(block)
	pcre_callout_block *block;
{
	struct regex_pcre *regex = (struct regex_pcre *)block->callout_data;

	if (SEE_system.periodic)
		(*SEE_system.periodic)(regex->regex.interp);
	return 0;
}

/* Initializes PCRE to use SEE's memory allocator and callout function */
static void
regex_pcre_init()
{
	int val;

	pcre_malloc = regex_pcre_malloc;
	pcre_free = regex_pcre_free;
	if (pcre_config(PCRE_CONFIG_STACKRECURSE, &val) == 0 && val == 0) {
	    pcre_stack_malloc = regex_pcre_malloc;
	    pcre_stack_free = regex_pcre_free;
	}
	pcre_callout = regex_pcre_callout;
}

static void 
regex_pcre_finalize(interp, p, closure)
	struct SEE_interpreter *interp;
	void *p, *closure;
{
	struct regex_pcre *regex = (struct regex_pcre *)p;

	if (regex->pcre) {
	    (*pcre_free)(regex->pcre);
	    regex->pcre = 0;
	}

	if (regex->pcre_extra) {
	    (*pcre_free)(regex->pcre_extra);
	    regex->pcre_extra = 0;
	}
}


/* Parses a source pattern and returns a regex state for later use */
static struct regex *
regex_pcre_parse(interp, pattern, flags)
	struct SEE_interpreter *interp; 
	struct SEE_string *pattern;
	int flags;
{
	struct regex_pcre *regex;
	char *utf8_pat;
	SEE_size_t utf8_len;
	const char *errptr;
	int erroffset;
	int ncaptures;
	pcre *pcre;
	pcre_extra *pcre_extra;
	int val;
	int i, need_utf8 = 0;

	/* Check to see if we need to enable UTF8 support */
	for (i = 0; i < pattern->length; i++)
	    if (pattern->data[i] > 0x7f) {
		need_utf8 = 1;
#ifndef NDEBUG
		if (SEE_regex_debug)
		    dprintf("SEE_regex_parse: unicode char in pattern\n");
#endif
		if (pcre_config(PCRE_CONFIG_UTF8, &val) != 0 || val == 0)
		    SEE_error_throw(interp, interp->Error,
			"pcre_config: UTF8 not supported");
		break;
	    }

	/* Convert the pattern to a UTF-8 string */
	utf8_len = SEE_string_utf8_size(interp, pattern);
	utf8_pat = SEE_malloc_string(interp, utf8_len + 1);
	SEE_string_toutf8(interp, utf8_pat, utf8_len + 1, pattern);
#ifndef NDEBUG
	if (SEE_regex_debug)
		dprintf("SEE_regex_parse: utf8_pat=\"%s\"\n", utf8_pat);
#endif

	/* Compile the pattern */
	pcre = pcre_compile(utf8_pat, 0
	    | PCRE_ANCHORED
	    | ((flags & FLAG_IGNORECASE) ? PCRE_CASELESS : 0)
	    | ((flags & FLAG_MULTILINE) ? PCRE_MULTILINE : 0)
	    | (need_utf8 ? PCRE_UTF8 : 0),
	    &errptr, &erroffset, 0);

	if (!pcre)
		SEE_error_throw(interp, interp->SyntaxError,
		    "pcre_compile: %s, at pattern offset %d", 
		    errptr, erroffset);

	/* Study the pattern to improve performance of repeated calls */
	pcre_extra = pcre_study(pcre, 0, &errptr);

	if (!pcre_extra && errptr) {
		(*pcre_free)(pcre);
		SEE_error_throw(interp, interp->SyntaxError,
		    "pcre_study: %s", errptr);
	}

	/* Count the number of captures */
	if (pcre_fullinfo(pcre, pcre_extra, PCRE_INFO_CAPTURECOUNT, &ncaptures))
	{
		(*pcre_free)(pcre_extra);
		(*pcre_free)(pcre);
		SEE_error_throw(interp, interp->Error,
		    "pcre_fullinfo: %s", errptr);
	}
#ifndef NDEBUG
	if (SEE_regex_debug)
		dprintf("SEE_regex_parse: CAPTURECOUNT = %d\n", ncaptures);
#endif

	if (!pcre_extra) {
	    pcre_extra = SEE_NEW(interp, struct pcre_extra);
	    pcre_extra->flags = 0;
	}

	/* Allocate a module-private regex structure */
	regex = SEE_NEW_FINALIZE(interp, struct regex_pcre, 
		regex_pcre_finalize, 0);
	regex->regex.engine = &_SEE_pcre_regex_engine;
	regex->regex.interp = interp;
	regex->flags = flags;
	regex->pcre = pcre;
	regex->pcre_extra = pcre_extra;
	regex->text_string = 0;
	regex->ncaptures = ncaptures + 1;
	regex->utf8 = need_utf8;

	/* Add callout properties for the periodic hook */
	pcre_extra->callout_data = regex;
	pcre_extra->flags |= PCRE_EXTRA_CALLOUT_DATA;

	return &regex->regex;
}

/* Returns the number of capture parentheses in the compiled regex */
static int
regex_pcre_count_captures(aregex)
	struct regex *aregex;
{
	struct regex_pcre *regex = REGEX_CAST(aregex);

	return regex->ncaptures;
}

/* Returns the flags of the regex object */
static int
regex_pcre_get_flags(aregex)
	struct regex *aregex;
{
	struct regex_pcre *regex = REGEX_CAST(aregex);

	return regex->flags;
}

/*
 * Executes the regex on the text beginning at index.
 * Returns true of a match was successful.
 */
static int
regex_pcre_match(interp, aregex, text, start, captures)
	struct SEE_interpreter *interp;
	struct regex *aregex;
	struct SEE_string *text;
	unsigned int start;
	struct capture *captures;
{
	struct regex_pcre *regex = REGEX_CAST(aregex);
	int text_start;
	struct SEE_string *substr;
	int *ovector;
	int rc;
	int i;

	/* Cache the text string */
	if (!regex->text_string || 
	    SEE_string_cmp(text, regex->text_string) != 0)
	{
#ifndef NDEBUG
		if (SEE_regex_debug)
			dprintf("SEE_regex_parse: text cache miss\n");
#endif
		regex->text_string = SEE_string_dup(interp, text);
		regex->text_len = SEE_string_utf8_size(interp, 
		    regex->text_string);
		regex->text_data = SEE_malloc_string(interp, 
		    regex->text_len + 1);
		SEE_string_toutf8(interp, regex->text_data, regex->text_len + 1,
		    regex->text_string);
	}

	substr = SEE_string_substr(interp, regex->text_string, 0, start);
	text_start = SEE_string_utf8_size(interp, substr);
#ifndef NDEBUG
	if (SEE_regex_debug)
		dprintf("SEE_regex_parse: start=%d text_start=%d\n",
		    start, text_start);
#endif

	ovector = SEE_STRING_ALLOCA(interp, int, 3 * regex->ncaptures);
	rc = pcre_exec(regex->pcre, regex->pcre_extra,
	    regex->text_data, regex->text_len, text_start,
	    regex->utf8 ? PCRE_NO_UTF8_CHECK : 0,
	    ovector, 3 * regex->ncaptures);

	if (rc == PCRE_ERROR_NOMATCH)
		return 0;
	if (rc < 0)
		SEE_error_throw(interp, interp->Error,
		    "pcre_exec: error %d", rc);
#ifndef NDEBUG
	if (SEE_regex_debug) {
		dprintf("SEE_regex_parse: pcre_exec() returned %d [", rc);
		for (i = 0; i < rc * 2; i++) {
		    if (i) dprintf(" ");
		    dprintf("%d", ovector[i]);
		}
		dprintf("]\n");
	}
#endif

	for (i = 0; i < regex->ncaptures; i++)
		if (i < rc) {
		    captures[i].start = ovector[i * 2];
		    captures[i].end = ovector[i * 2 + 1];
		} else {
		    captures[i].start = -1;
		    captures[i].end = -1;
		}
	return 1;
}
