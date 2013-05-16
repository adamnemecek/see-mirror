
noinst_PROGRAMS = httpd

httpd_SOURCES=	httpd.c httpd.h ssp.c ssp.h pool.c pool.h

httpd_LDADD=                $(top_builddir)/libsee/libsee.la
httpd_DEPENDENCIES=         $(top_builddir)/libsee/libsee.la
httpd_LDFLAGS=		    $(PTHREADS_LDFLAGS)
httpd_CFLAGS=		    $(PTHREADS_CFLAGS)
INCLUDES=                   -I$(top_builddir)/include \
                            -I$(top_srcdir)/include

EXTRA_DIST=		test.ssp include.ssp
