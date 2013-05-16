dnl Process this file with autoconf to produce a configure script.

AC_INIT([see],
        [3.0.]esyscmd((svnversion .||echo 0)|tr : _|tr -d \\012),
	[leonard@users.sourceforge.net])
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(include/config.h)
AC_CONFIG_FILES([Makefile
		 include/Makefile
		 include/see/Makefile
		 libsee/Makefile 
		 libsee/test/Makefile 
		 libsee/libsee-config
		 doc/Makefile 
		 shell/Makefile
		 shell/test/Makefile
		 ssp/Makefile
		 ])

## -- fix brokenness with automake-1.4
AC_SUBST(PACKAGE,[$PACKAGE_NAME])
AC_SUBST(VERSION,[$PACKAGE_VERSION])

dnl ------------------------------------------------------------
dnl programs required
dnl

AC_PROG_CC
AC_PROG_SED

AC_LIBLTDL_CONVENIENCE([libltdl])
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL
dnl AC_LIB_LTDL
AC_CONFIG_SUBDIRS([libltdl])
AC_SUBST([LIBLTDL])
AC_SUBST([LTDLINCL])
AC_SUBST([LIBSEE_LIBS])
AC_SUBST([LIBSEE_REQS])

AC_ARG_VAR(PERL, [Practical Extraction and Report Language interpreter])
AC_PATH_PROG(PERL, perl)
m4_define(_perl_vers_var,$])
AC_CACHE_CHECK([$PERL version], [ac_cv_perl_version],
    [ac_cv_perl_version=`$PERL -e 'print _perl_vers_var;' || echo error`])
test "$ac_cv_perl_version" != error || \
    AC_MSG_ERROR([cannot find a working PERL interpreter])

dnl ------------------------------------------------------------
dnl C headers
dnl

AC_HEADER_STDC
AC_HEADER_TIME

dnl (alloca.h needs to know about these headers)
AC_CHECK_HEADERS([errno.h string.h stdlib.h],,,[;])

dnl (These make finding integer sizes simpler)
AC_CHECK_HEADERS([stdint.h inttypes.h],,,[;])

dnl (This is for some windows compat)
AC_CHECK_HEADERS([windows.h getopt.h],,,[;])

dnl (And others)
AC_CHECK_HEADERS([float.h limits.h signal.h],,,[;])

dnl ------------------------------------------------------------
dnl C compiler features
dnl

AC_C_VOLATILE
AC_C_CONST
AC_C_BIGENDIAN

AC_CHECK_SIZEOF(signed int)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(signed short)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(signed long)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(signed long long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(signed LONGLONG)
AC_CHECK_SIZEOF(unsigned LONGLONG)

AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)

AC_TYPE_SIZE_T
AC_TYPE_SIGNAL

AC_DEFUN([SEE_CHECK___FUNCTION__],
  [AC_CACHE_CHECK([for __FUNCTION__], [ac_cv_have___FUNCTION__],
		  [AC_TRY_RUN([int main() { return __FUNCTION__ ? 0 : 1; }],
			      [ac_cv_have___FUNCTION__=yes], 
			      [ac_cv_have___FUNCTION__=no],
			      [ac_cv_have___FUNCTION__=unknown])])
   if test $ac_cv_have___FUNCTION__ = yes; then
       AC_DEFINE([HAVE___FUNCTION__], [1], 
		 [Define to 1 if the compiler generates __FUNCTION__])
   fi
  ])
SEE_CHECK___FUNCTION__

dnl ------------------------------------------------------------
dnl features
dnl

dnl
dnl Helper macro to build sensible --enable-FEATURE options
dnl Usage is: SEE_ARG_ENABLE([feature],[default],[feature description],
dnl                          [auto-actions], [yes-actions],[no-actions])
dnl the auto-actions are run when enable_feature==auto. It should detect
dnl and set dnl $enable_feature=yes if it makes sense to do so.
dnl
AC_DEFUN([_SEE_ARG_ENABLE],[
 AC_ARG_ENABLE([$1],
  AC_HELP_STRING([ifelse($2,yes,[--disable-$1],$2,no,[--enable-$1],$2,auto,[--disable-$1])],[$3]ifelse($2,auto,[ (auto)])),
  [dnl (( # two parens help editor balance
   case "$enableval" in ifelse([$4],,,auto|)yes|no) :;; *) #
    AC_MSG_WARN([unexpected value '$enableval' for --enable-$1. Using '$2'.])
    $7=$2
    ;; esac],
  [$7=$2])
  AS_IF(test x"[$]$7" = x"auto",[$4])
  AC_MSG_CHECKING([whether to enable $3])
  AS_IF(test x"[$]$7" = x"auto",[AC_MSG_ERROR([bad value for $7])])
  AC_MSG_RESULT([$]$7) 
  AS_IF([ifelse($2,yes,test x"[$]$7" != x"no",$2,no,test x"[$]$7" = x"yes")],
   [$5],[$6])
])
dnl SEE_ARG_ENABLED(FEATURE,DEFAULT,HELPTEXT,AUTO_ACTION,YES_ACTION,NO_ACTION)
AC_DEFUN([SEE_ARG_ENABLE],
 [_SEE_ARG_ENABLE([$1],[$2],[$3],[$4],[$5],[$6],[enable_]m4_bpatsubst([$1], -, _))])


SEE_ARG_ENABLE(developer,[no],
   [pedantic compiler warnings],,
   [for flag in -Wall -Wmissing-declarations \
    		 -Wmissing-prototypes  -Wtraditional -pedantic
    do
	AC_MSG_CHECKING([for $flag])
        CFLAGS_save="$CFLAGS"
	CFLAGS="$CFLAGS $flag"
    	AC_COMPILE_IFELSE([/* empty */],
			  [AC_MSG_RESULT(yes)],
			  [AC_MSG_RESULT(no)
			   CFLAGS="$CFLAGS_save"])
    done
    CPPFLAGS="$CPPFLAGS -DGC_DEBUG"
])

SEE_ARG_ENABLE(diagnostic,[yes],
   [internal consistency checks],,,
   [if test x"$enable_developer" = x"yes"; then
        AC_MSG_WARN([Developer mode enabled, but diagnostic was disabled!])
    fi
    CPPFLAGS="$CPPFLAGS -DNDEBUG"
])

SEE_ARG_ENABLE(full-unicode,[yes],
   [full Unicode instead of ASCII tables],,
   [AC_DEFINE(WITH_UNICODE_TABLES, [1],
    	[Define to 1 if you want the Unicode tables for ECMA262 compliance])
])

SEE_ARG_ENABLE(parser-print,[yes],
   [function printing code],,
   [AC_DEFINE(WITH_PARSER_PRINT, [1],
        [Define if you want to be able to print function bodies])
])

SEE_ARG_ENABLE(parser-visit,[no],
   [experimental node visiting API],,
   [AC_DEFINE(WITH_PARSER_VISIT, [1],
        [Define if you want to include experimental AST visitor code])
])

dnl XXX is this incompatible with parser_print?
SEE_ARG_ENABLE(bytecode,[yes],
   [bytecode generator],,
   [AC_DEFINE(WITH_PARSER_CODEGEN, [1],
        [Define if you want to use the code generator])
])
SEE_ARG_ENABLE(ast-eval,[no],
   [in-AST evaluator],,
   [AC_DEFINE(WITH_PARSER_EVAL, [1],
        [Define if you want to use the stable AST evaluator])
])

dnl XXX need to find a better way to get pthreads flags in
SEE_ARG_ENABLE(ssp-example,[no],
   [SEE Servlet Pages (SSP) example],,
   [PTHREADS_CFLAGS=-pthreads
    PTHREADS_LDFLAGS=-lpthread
    AC_SUBST(PTHREADS_CFLAGS)
    AC_SUBST(PTHREADS_LDFLAGS)
])
AM_CONDITIONAL(SSP, test x"$enable_ssp_example" = x"yes")

SEE_ARG_ENABLE(longjmperror,[yes],
   [catching longjmp corruption within SEE],,
   [AC_DEFINE(WITH_LONGJMPERROR, [1],
        [Define if you want SEE to catch longjmp corruption])
])

SEE_ARG_ENABLE(native-dtoa,[auto],
    [dtoa from libc],
    [enable_native_dtoa=yes
     AC_CHECK_FUNCS([strtod dtoa freedtoa],,[enable_native_dtoa=no])
])
AM_CONDITIONAL(NATIVE_DTOA, test x"$enable_native_dtoa" = x"yes")


dnl ------------------------------------------------------------
dnl external libraries
dnl

AC_ARG_WITH(boehm-gc, 
    AC_HELP_STRING([--with-boehm-gc],
	[use Boehm-Weiser garbage collector, default=yes]),
	[with_boehmgc=$withval], [with_boehmgc=yes])
if test x"$with_boehmgc" != x"no"; then
    save_LIBSEE_LIBS="$LIBSEE_LIBS"
    save_LIBS="$LIBS"
    save_LDFLAGS="$LDFLAGS"
    save_CPPFLAGS="$CPPFLAGS"
    case "$with_boehmgc" in 
     yes) 
        LIBSEE_LIBS="$LIBSEE_LIBS -lgc"
        LIBS="$LIBS -lgc"
        ;;
     *) LIBSEE_LIBS="$LIBSEE_LIBS -L${with_boehmgc}/lib -lgc"
        LDFLAGS="$LDFLAGS -L${with_boehmgc}/lib"
        LIBS="$LIBS -lgc"
        CPPFLAGS="$CPPFLAGS -I${with_boehmgc}/include"
	;;
    esac

    # The following minimum for Boehm GC support:
    #
    #   #if WITH_BOEHM_GC
    #   # include <gc/gc.h>
    #   GC_INIT();
    #   GC_MALLOC(...);
    #   GC_dump();
    #   GC_gcollect();
    #   #endif

    have_boehm_gc=yes
    AC_CHECK_HEADERS([gc/gc.h],,[have_boehm_gc=no])
    AC_TRY_LINK([#include <gc/gc.h>],
	[GC_INIT();
	 GC_MALLOC(1);
	 GC_gcollect();
	],,
	[have_boehm_gc=no])
    if test x"$have_boehm_gc" = x"yes"; then
	AC_DEFINE(WITH_BOEHM_GC, [1], [Define if you have Boehm GC])
    else
	LIBSEE_LIBS="$save_LIBSEE_LIBS"
	CPPFLAGS="$save_CPPFLAGS"
    fi
    LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"
else
    have_boehm_gc=no
fi
AC_MSG_CHECKING([if we can use Boehm GC])
AM_CONDITIONAL(WITH_BOEHM_GC, test x"$have_boehm_gc" = x"yes")
AC_MSG_RESULT([$have_boehm_gc])

AC_ARG_WITH(readline, 
    AC_HELP_STRING([--with-readline],
	[see-shell interactive niceness, default=no]),
	[ac_cv_use_readline=$withval], [ac_cv_use_readline=no])
if test x"$ac_cv_use_readline" != x"no"; then
    AC_CHECK_HEADERS([readline/readline.h readline.h],,,[;])
    AC_SEARCH_LIBS(tgoto, [termcap termlib curses ncurses])
    AC_SEARCH_LIBS(readline, [readline edit])
    AC_CHECK_FUNC(readline)
fi

AC_ARG_WITH(pcre, 
    AC_HELP_STRING([--with-pcre],
	[use the PCRE regex library, default=no]),
	[ac_cv_use_pcre=$withval], [ac_cv_use_pcre=no])
if test x"$ac_cv_use_pcre" != x"no"; then
    PKG_CHECK_MODULES([PCRE], [pcre], 
	[have_pcre=yes
	 LIBSEE_REQS="$LIBSEE_REQS,pcre"
	 LIBSEE_LIBS="$LIBSEE_LIBS $PCRE_LIBS"
	 CFLAGS="$CFLAGS $PCRE_CFLAGS"],
	[have_pcre=yes
	 AC_CHECK_HEADER([pcre.h],,[have_pcre=no])
	 save_LIBS="$LIBS"
	 LIBS="$LIBSEE_LIBS"
	 AC_SEARCH_LIBS(pcre_compile, [pcre])
	 AC_CHECK_FUNCS([pcre_compile pcre_exec],, [have_pcre=no])])
	 LIBSEE_LIBS="$LIBS"
	 LIBS="$save_LIBS"
    AC_MSG_CHECKING([if we can use PCRE])
    if test x"$have_pcre" = x"yes"; then
       AC_DEFINE(WITH_PCRE, [1],
	    [Define if you want to use the PCRE regex library])
    fi
    AC_MSG_RESULT([$have_pcre])
else
    have_pcre=no
fi
AM_CONDITIONAL(WITH_PCRE, test x"$have_pcre" = x"yes")


dnl ------------------------------------------------------------
dnl std lib functions
dnl

AC_FUNC_MEMCMP
test $ac_cv_func_memcmp_working = yes && AC_DEFINE(HAVE_MEMCMP)
AC_FUNC_ALLOCA
AC_REPLACE_FUNCS([memmove])

AC_SEARCH_LIBS(sin, [m])		# the evil that lurks within

dnl -- pick the right nan/finite functions for 64-bit floating point type
have_isnan=no
have_finite=no
have_copysign=no
AC_CHECK_FUNCS([isnanf finitef isinff isinf _isinf copysignf])
AC_CHECK_FUNCS([isnan _isnan],[have_isnan=yes])
AC_CHECK_FUNCS([finite _finite isfinite],[have_finite=yes])
AC_CHECK_FUNCS([copysign _copysign],[have_copysign=yes])

dnl -- when using double isnan/finite must be available
if test $ac_cv_sizeof_float -ne 8; then
    test $have_isnan = no && missing_funcs="$missing_funcs isnan";
    test $have_finite = no && missing_funcs="$missing_funcs finite";
    test $have_copysign = no && missing_funcs="$missing_funcs copysign";
fi

dnl --- things to add later?
dnl  AC_CHECK_FUNCS([srand srandom vsprintf funopen])
dnl  AC_CHECK_FUNC(vsnprintf,,AC_LIBOBJ(vsnprintf))

AC_CHECK_FUNC(rand_r)

dnl -- functions that we *need* (i.e. don't have workarounds for):
AC_CHECK_FUNCS([acos atan atan2 ceil cos exp floor log pow sin sqrt tan \
		snprintf vsnprintf \
		abort memcpy memset \
		qsort strcmp strerror],,
	       [missing_funcs="$missing_funcs $ac_func"])
 
if test -n "$missing_funcs"; then
    AC_MSG_ERROR([cannot find required functions:$missing_funcs])
fi

dnl -- functions that have workarounds written for
AC_CHECK_FUNCS([strdup GC_malloc getopt \
		time gettimeofday GetSystemTimeAsFileTime \
		localtime mktime \
		isatty \
		])
AC_CHECK_FUNCS([GC_gcollect GC_dump GC_malloc_atomic GC_free])

dnl ------------------------------------------------------------
dnl miscellanea
dnl

dnl
dnl NaN and Infinity as inline expressions: another optimisation
dnl

AC_CACHE_CHECK([for constant NaN], [ac_cv_cc_constant_NaN_div],
	[AC_TRY_RUN([#include <math.h>
		     #if SIZEOF_FLOAT == 8
		     float nan = 0.0 / 0.0;
		     int main() { exit( isnanf(nan) && !finitef(nan) ? 0:1 ); }
		     #elif SIZEOF_DOUBLE == 8
		     double nan = 0.0 / 0.0;
		     int main() { exit( isnan(nan) && !finite(nan) ? 0:1 ); }
		     #else
		     # error "no 64-bit floating point type"
		     #endif
		     ],
		    [ac_cv_cc_constant_NaN_div=yes],
		    [ac_cv_cc_constant_NaN_div=no],
		    [ac_cv_cc_constant_NaN_div=unknown])])
test $ac_cv_cc_constant_NaN_div = yes && AC_DEFINE(HAVE_CONSTANT_NAN_DIV)

AC_CACHE_CHECK([for constant Inf], [ac_cv_cc_constant_Inf_div],
	[AC_TRY_RUN([#include <math.h>
		     #if SIZEOF_FLOAT == 8
		     float inf = 1.0 / 0.0;
		     int main() { exit( !isnanf(inf) && !finitef(inf) ? 0:1 ); }
		     #elif SIZEOF_DOUBLE == 8
		     double inf = 1.0 / 0.0;
		     int main() { exit( !isnan(inf) && !finite(inf) ? 0:1 ); }
		     #else
		     # error "no 64-bit floating point type"
		     #endif
		     ],
		    [ac_cv_cc_constant_Inf_div=yes],
		    [ac_cv_cc_constant_Inf_div=no],
		    [ac_cv_cc_constant_Inf_div=unknown])])
test $ac_cv_cc_constant_Inf_div = yes && AC_DEFINE(HAVE_CONSTANT_INF_DIV)

dnl
dnl Hexadecimal floating point constants are ANSI, but are not supported
dnl everywhere it seems.
dnl

AC_CACHE_CHECK([for hex float constants], [ac_cv_cc_constant_hex_float],
	[AC_TRY_RUN([#if SIZEOF_FLOAT == 8
		      float 
		     #elif SIZEOF_DOUBLE == 8
		      double
		     #else
		     # error "no 64-bit floating point type"
		     #endif
		        number = 0x1p3;
		     int main() { exit( number == 8 ? 0:1 ); }
		     ],
		    [ac_cv_cc_constant_hex_float=yes],
		    [ac_cv_cc_constant_hex_float=no],
		    [ac_cv_cc_constant_hex_float=unknown])])
test $ac_cv_cc_constant_hex_float = yes && AC_DEFINE(HAVE_CONSTANT_HEX_FLOAT)

dnl
dnl GNU-style variadic macros
dnl

AC_CACHE_CHECK([for variadic macros], [ac_cv_cc_variadic_macros],
	[AC_TRY_COMPILE([
void f(void);
#define f(a, b...) g(a, 5 , ## b)
int g(v, w, x, y, z) int v,w,x,y,z; { return v*w*x*y*z; }
		     ],
		    [exit( f(2,3,7,9) == 2*3*5*7*9 ? 0 : 1 );],
		    [ac_cv_cc_variadic_macros=yes],
		    [ac_cv_cc_variadic_macros=no])])
test $ac_cv_cc_variadic_macros = yes && AC_DEFINE(HAVE_VARIADIC_MACROS)

dnl
dnl setjmp/longjmp
dnl

AC_CHECK_HEADERS([setjmp.h],,,[;])
AC_CACHE_CHECK([for working _longjmp()], [ac_cv_cc__longjmp_works],
	[AC_TRY_RUN([#include <setjmp.h>
		     int main () {
			jmp_buf b; volatile int c = 0;

			if (_setjmp(b) == 6) c++;
			if (c) exit(c == 2 ? 0 : c+1);
			c++;
			_longjmp(b, 6);
			exit(1);
		     }],
		    [ac_cv_cc__longjmp_works=yes],
		    [ac_cv_cc__longjmp_works=no],
		    [ac_cv_cc__longjmp_works=unknown])])
test $ac_cv_cc__longjmp_works = yes && AC_DEFINE(HAVE__LONGJMP)

AC_CACHE_CHECK([for working longjmp()], [ac_cv_cc_longjmp_works],
	[AC_TRY_RUN([#include <setjmp.h>
		     int main () {
			jmp_buf b; volatile int c = 0;

			if (setjmp(b) == 6) c++;
			if (c) exit(c == 2 ? 0 : c+1);
			c++;
			longjmp(b, 6);
			exit(1);
		     }],
		    [ac_cv_cc_longjmp_works=yes],
		    [ac_cv_cc_longjmp_works=no],
		    [ac_cv_cc_longjmp_works=unknown])])

if test $ac_cv_cc_longjmp_works != no; then
	if test $ac_cv_cc_longjmp_works = unknown; then
		AC_MSG_WARN([assuming that longjmp() will actually work])
	fi
	AC_DEFINE(HAVE_LONGJMP)
fi

if test $ac_cv_cc__longjmp_works = no -a \
        $ac_cv_cc_longjmp_works = no; then
		AC_MSG_ERROR([cannot find working setjmp()/longjmp()])
fi

dnl
dnl check that vsnprintf works
dnl

AC_CACHE_CHECK([for working vsnprintf(0,0,,)], [ac_cv_libc_vsnprintf_works],
	[AC_TRY_RUN([#include <stdio.h>
		     #include <stdarg.h>

		     int a(fmt, ap)
			const char *fmt;
			va_list ap;
		     {
			return vsnprintf(0,0,fmt,ap);
		     }
		     int b(char *fmt, ...)
		     {
			va_list ap;
			int ret;
			va_start(ap, fmt);
			ret = a(fmt, ap);
			va_end(ap);
			return ret;
		     }
		     int main() {
			int ret = b("test %s", "string");
			exit( ret == 11 ? 0 : 1);
		     }],
		    [ac_cv_libc_vsnprintf_works=yes],
		    [ac_cv_libc_vsnprintf_works=no],
		    [ac_cv_libc_vsnprintf_works=unknown])])

dnl ------------------------------------------------------------
dnl IEEE doubles/floats
dnl

AC_CACHE_CHECK([for 64-bit IEEE754 fp], [ac_cv_cc_ieee754],
    [AC_TRY_RUN([
	int main() {
#if SIZEOF_FLOAT == 8
	    float 
#elif SIZEOF_DOUBLE == 8
	    double 
#else
# error "no 64-bit floating point type"
#endif
	      fp = 2.7182818284590452354;
	    unsigned char expected[8] = {
		0x40, 0x05, 0xbf, 0x0a, 0x8b, 0x14, 0x57, 0x69
	    }, e;
	    int i;

	    for (i = 0; i < 8; i++) {
#ifdef WORDS_BIGENDIAN
		e = expected[i];
#else
		e = expected[7-i];
#endif
		if (e != ((unsigned char *)&fp)[i])
		    exit(1);
	    }
	    exit(0);
	}],
    [ac_cv_cc_ieee754=yes],
    [ac_cv_cc_ieee754=no],
    [ac_cv_cc_ieee754=unknown])])

if test x"$ac_cv_cc_ieee754" != x"yes"; then
    AC_MSG_WARN([SEE's ECMAScript compliance relies on native IEEE754 support])
fi

AC_CACHE_CHECK([platform class], [ac_cv_see_platform], [
case "$build_os" in
	cygwin*|mingw*|pw32)	ac_cv_see_platform=win32;;
	*)			ac_cv_see_platform=posix;;
esac])
AM_CONDITIONAL(PLATFORM_POSIX, test x"$ac_cv_see_platform" = x"posix")
AM_CONDITIONAL(PLATFORM_WIN32, test x"$ac_cv_see_platform" = x"win32")

dnl ------------------------------------------------------------
dnl templates for autoheader
dnl

AH_TOP([#ifndef _SEE_h_config_
#define _SEE_h_config_])
AH_BOTTOM([#endif /* _SEE_h_config_ */])

AH_TEMPLATE([HAVE_CONSTANT_INF_DIV],
	    [Define if your compiler treats the expression 1.0/0.0 as constant])
AH_TEMPLATE([HAVE_CONSTANT_NAN_DIV],
	    [Define if your compiler treats the expression 0.0/0.0 as constant])
AH_TEMPLATE([HAVE_CONSTANT_HEX_FLOAT],
	    [Define if your compiler handles ANSI hex fp constants like '0x1p3'])
AH_TEMPLATE([HAVE_VARIADIC_MACROS],
	    [Define if your preprocessor understands GNU-style variadic macros])
AH_TEMPLATE([HAVE_LONGJMP],
	    [Define if you have working longjmp() and setjmp() functions])
AH_TEMPLATE([HAVE__LONGJMP],
	    [Define if you have working _longjmp() and _setjmp() functions])
AH_TEMPLATE([HAVE_MEMCMP],
	    [Define if you have a working memcmp() function])

dnl ------------------------------------------------------------
dnl tests
dnl


dnl ------------------------------------------------------------
dnl finalisation
dnl

LIBSEE_REQS=`echo "$LIBSEE_REQS" | $SED -e 's/^,//'` #remove leading commas
AC_OUTPUT
