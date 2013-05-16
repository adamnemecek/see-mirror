## Process this file with automake to produce Makefile.in
## $Id: Makefile.am 1335 2008-02-05 11:13:39Z d $

OBJEXT=		   @OBJEXT@

lib_LTLIBRARIES=   libsee.la
CLEANFILES=	   stringdefs.h stringdefs.inc
BUILT_SOURCES=	   stringdefs.h stringdefs.inc
libsee_la_SOURCES= cfunction.c scope.c debug.c dprint.c enumerate.c \
                   error.c function.c input_file.c input_lookahead.c	\
                   input_string.c input_utf8.c intern.c interpreter.c	\
                   lex.c mem.c native.c no.c obj_Array.c obj_Boolean.c	\
                   obj_Date.c obj_Error.c obj_Function.c obj_Global.c	\
                   obj_Math.c obj_Number.c obj_Object.c obj_RegExp.c	\
                   obj_String.c object.c parse.c printf.c         	\
		   string.c stringdefs.c system.c tokens.c try.c 	\
		   unicase.c unicode.c value.c version.c		\
		   module.c code1.c math.c

libsee_la_SOURCES+= regex.c regex_ecma.c
if WITH_PCRE
libsee_la_SOURCES+= regex_pcre.c
endif

libsee_la_SOURCES += array.h cfunction_private.h code.h dprint.h dtoa.h \
		     dprint.h enumerate.h function.h init.h code1.h	\
		     lex.h nmath.h parse.h platform.h printf.h regex.h 	\
		     scope.h tokens.h unicase.inc unicode.h unicode.inc	\
		     stringdefs.h stringdefs.inc replace.h
		   
if NATIVE_DTOA
libsee_la_SOURCES += dtoa_native.c
else
libsee_la_SOURCES += dtoa.c dtoa_config.h
endif

if PLATFORM_WIN32
libsee_la_SOURCES += platform_win32.c
endif
if PLATFORM_POSIX
libsee_la_SOURCES += platform_posix.c
endif

libsee_la_LIBADD=  @LIBOBJS@
libsee_la_LDFLAGS= -version-info $(libsee_version_info)

EXTRA_DIST=	   string.pl string.defs
PERL=		   @PERL@

bin_SCRIPTS=	   libsee-config

INCLUDES=	   -I$(top_builddir)/include -I$(top_srcdir)/include

stringdefs.inc: string.defs string.pl
	$(PERL) $(srcdir)/string.pl c $(srcdir)/string.defs >$@
stringdefs.h: string.defs string.pl
	$(PERL) $(srcdir)/string.pl h $(srcdir)/string.defs >$@

pkgconfigdir=	    $(libdir)/pkgconfig
pkgconfig_DATA=	    see.pc
see.pc: see.pc.in
	$(SED) \
	    -e 's,[@]libdir[@],$(libdir),g' \
	    -e 's,[@]includedir[@],$(includedir),g' \
	    -e 's,[@]LIBSEE_LIBS[@],$(LIBSEE_LIBS),g' \
	    -e 's,[@]LIBSEE_REQS[@],$(LIBSEE_REQS),g' \
	    -e 's,[@]PACKAGE_NAME[@],$(PACKAGE_NAME),g' \
	    -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g' \
	    < $(srcdir)/see.pc.in >$@
EXTRA_DIST+=	   see.pc.in
CLEANFILES+=	   see.pc

#
# (From the libtool documentation:)
#
# 1. Start with version information of `0:0:0' for each libtool library.
# 
# 2. Update the version information only immediately before a public
#    release of your software.  More frequent updates are unnecessary,
#    and only guarantee that the current interface number gets larger
#    faster.
# 
# 3. If the library source code has changed at all since the last
#    update, then increment REVISION (`c:r:a' becomes `c:r+1:a').
# 
# 4. If any interfaces have been added, removed, or changed since the
#    last update, increment CURRENT, and set REVISION to 0. (`c+1:0:a')
# 
# 5. If any interfaces have been added since the last public release,
#    then increment AGE (`c+1:0:a+1').
# 
# 6. If any interfaces have been removed since the last public release,
#    then set AGE to 0. (`c+1:0:0`)
#

libsee_version_info=	2:0:1

SUBDIRS=		. test

