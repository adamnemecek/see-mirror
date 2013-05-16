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
/* $Id: memcmp.c 751 2004-12-02 11:24:25Z d $ */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "replace.h"

#if !HAVE_MEMMOVE
/* ANSI memmove() for systems that don't have it. */
void
memmove(adst, asrc, n)
	void *adst, *asrc;
	int n;
{
	char *dst = (char *)adst;
	char *src = (char *)asrc;

	if (src > dst)
	    while (n-- > 0)
		*dst++ = *src++;
	else if (src < dst) {
	    src += n;
	    dst += n;
	    while (n-- > 0)
		*(--dst) = *(--src);
	}
}
#endif


#if TEST
int main()
{
    char x[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    char e[10] = { 1, 2, 5, 6, 7, 8, 9, 8, 9, 10 };
    char f[10] = { 1, 2, 3, 4, 3, 4, 5, 6, 7, 10 };
    char w[10];

    /* Test copying down */
    memcpy(w, x, sizeof w);
    memmove(w + 2, w + 4, 5);
    if (memcmp(w, e, sizeof w) != 0)
	return 1;

    /* Test copying up */
    memcpy(w, x, sizeof w);
    memmove(w + 4, w + 2, 5);
    if (memcmp(w, f, sizeof w) != 0)
	return 2;

    /* Test copying to same destination */
    memcpy(w, x, sizeof w);
    memmove(w + 2, w + 2, 5);
    if (memcmp(w, x, sizeof w) != 0)
	return 3;

    /* Test copying down with size 0 */
    memcpy(w, x, sizeof w);
    memmove(w + 4, w + 2, 0);
    if (memcmp(w, x, sizeof w) != 0)
	return 4;

    /* Test copying up with size 0 */
    memcpy(w, x, sizeof w);
    memmove(w + 2, w + 4, 0);
    if (memcmp(w, x, sizeof w) != 0)
	return 5;

    return 0;
}
#endif
