## Process this file with automake to produce Makefile.in
## $Id: Makefile.am 1362 2008-02-08 14:22:52Z d $

bin_PROGRAMS=			see-shell
see_shell_SOURCES=		main.c shell.c getopt.c readline.c 	\
				debug.c module.c compat.c		\
				trace.c gtrace.c gtrace.h		\
				shell.h debug.h compat.h module.h
see_shell_LDADD=		$(top_builddir)/libsee/libsee.la
see_shell_DEPENDENCIES=		$(top_builddir)/libsee/libsee.la
see_shell_CPPFLAGS=		-DPATH_PKGLIB='"$(pkglib)"'

see_shell_LDADD+=		$(LIBLTDL)
see_shell_CPPFLAGS+=		$(LTDLINCL)

see_shell_LDADD+=		$(LIBSEE_LIBS)

## A statically linked version for running 'make check'
noinst_PROGRAMS=		see-shell.static
see_shell_static_SOURCES=	$(see_shell_SOURCES)
see_shell_static_LDADD=		$(see_shell_LDADD)
see_shell_static_DEPENDENCIES=	$(see_shell_DEPENDENCIES)
see_shell_static_CPPFLAGS=	$(see_shell_CPPFLAGS)
see_shell_static_LDFLAGS=	 -static

pkglib_LTLIBRARIES=		libFile.la
libFile_la_SOURCES=		mod_File.c
libFile_la_LDFLAGS=		-module

#-- the following DEFS line tries to avoid the -I../include/see
DEFS = @DEFS@ -I. -I$(srcdir)
INCLUDES=			-I$(top_builddir)/include \
				-I$(top_srcdir)/include

SUBDIRS=			test

TESTS_ENVIRONMENT=		SHELLARGS="-f $(srcdir)/test/common.js" \
				LIBTOOL="$(LIBTOOL)" $(SHELL)
TESTS=				t-File

EXTRA_DIST=			README t-File t-File.js

