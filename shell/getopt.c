/*
 * Copyright (c) 2004
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
/* $Id: getopt.c 1282 2007-09-13 22:26:35Z d $ */

/*
 * A cheap implementation of getopt(), based on API described in
 * http://www.opengroup.org/onlinepubs/009695399/functions/getopt.html
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if !HAVE_GETOPT

char *optarg;
int optind = 1, opterr = 1, optopt;

static int optindind = 0;

int
getopt(argc, argv, optstring)
    int argc;
    char * const argv[];
    const char *optstring;
{
    char ch;
    char *arg = argv[optind];
    const char *p;

    /* End on first non-option */
    if (arg == NULL || arg[0] != '-' || arg[1] == '\0')
	return -1;

    ch = arg[++optindind];

    /* End after '--' */
    if (ch == '-' && arg[2] == '\0') {
	optind++;
	return -1;
    }

    for (p = optstring; *p; p++)
	if (*p == ch)
	    break;
    if (!*p) {
	optopt = ch;
	if (opterr) 
	    fprintf(stderr, "%s: illegal option -- %c\n", argv[0], ch);
	return '?';
    }

    if (*(p+1) == ':') {
	optarg = &arg[optindind+1];
	if (!*optarg) {
	    optind++;
	    if (optind >= argc) {
		optopt = ch;
		if (opterr)
		    fprintf(stderr, "%s: option requires an argument -- %c\n",
			argv[0], ch);
		return *optstring == ':' ? ':' : '?';
	    }
	    optarg = argv[optind];
	}
	optind++;
	optindind = 0;
	return ch;
    }

    if (arg[optindind + 1] == '\0') {
	optindind = 0;
	optind++;
    }

    return ch;
}
#endif /* !HAVE_GETOPT */
