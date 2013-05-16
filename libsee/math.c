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

#if HAVE_FLOAT_H
# include <float.h>
#endif

#include <see/type.h>
#include <see/value.h>

/* Returns true if the number represents NaN */
int
_SEE_isnan(n)
	SEE_number_t n;
{
#if SEE_NUMBER_IS_FLOAT && HAVE_ISNANF
	return isnanf(n);
#elif HAVE_ISNAN
	return isnan(n);
#elif HAVE__ISNAN
	return _isnan(n);
#else
 #error "No isnan() defined in config.h"
#endif
}

SEE_number_t
_SEE_copysign(x, y)
	SEE_number_t x, y;
{
#if SEE_NUMBER_IS_FLOAT && HAVE_COPYSIGNF
	return copysignf(x, y);
#elif HAVE_COPYSIGN
	return copysign(x, y);
#elif HAVE__COPYSIGN
	return _copysign(x, y);
#else
 #error "No copysign() defined in config.h"
#endif
}

/* Returns true for finite numbers n */
int
_SEE_isfinite(n)
	SEE_number_t n;
{
#if SEE_NUMBER_IS_FLOAT && HAVE_FINITEF
	return finitef(n);
#elif HAVE_ISFINITE
	return isfinite(n);
#elif HAVE_FINITE
	return finite(n);
#elif HAVE__FINITE
	return _finite(n);
#else
 #error "No finite() defined in config.h"
#endif
}

/* Returns true for n == +Infinity */
int
_SEE_ispinf(n)
	SEE_number_t n;
{
#if SEE_NUMBER_IS_FLOAT && HAVE_ISINFF
	return n > 0 && isinff(n);
#elif HAVE_ISINF
	return n > 0 && isinf(n);
#elif HAVE__ISINF
	return n > 0 && _isinf(n);
#else
	return n > 0 && !_SEE_isfinite(n) && !_SEE_isnan(n);
#endif
}

/* Returns true for n == -Infinity */
int
_SEE_isninf(n)
	SEE_number_t n;
{
#if SEE_NUMBER_IS_FLOAT && HAVE_ISINFF
	return n < 0 && isinff(n);
#elif HAVE_ISINF
	return n < 0 && isinf(n);
#elif HAVE__ISINF
	return n < 0 && _isinf(n);
#else
	return n < 0 && !_SEE_isfinite(n) && !_SEE_isnan(n);
#endif
}

