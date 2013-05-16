/*
 * Copyright (c) 2006
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <windows.h>
#include <see/type.h>

#include "platform.h"

SEE_number_t
_SEE_platform_time(interp)
	struct SEE_interpreter *interp;
{
	LONGLONG ft;

	GetSystemTimeAsFileTime((FILETIME *)&ft);
	/*
	 * Shift from 1601 to 1970 timebase, and convert to msec. See also
	 * http://msdn.microsoft.com/library/default.asp?url=/library/
	 *  en-us/sysinfo/base/converting_a_time_t_value_to_a_file_time.asp
	 */
	return (ft - 116444736000000000LL) / 10000.0;

}

SEE_number_t
_SEE_platform_tza(interp)
	struct SEE_interpreter *interp;
{
	return 0;
}

SEE_number_t
_SEE_platform_dst(interp, ysec, ily, wstart)
	struct SEE_interpreter *interp;
	SEE_number_t ysec;
	int ily, wstart;
{
	return 0;
}

/* Abort the current system with a message */
void
_SEE_platform_abort(interp, msg)
	struct SEE_interpreter *interp;
	const char *msg;
{
	abort();
}
