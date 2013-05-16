
AM_CPPFLAGS=	    -I$(srcdir)/../../include
AM_LDFLAGS=	    -L.. -lsee
LDADD=              $(LIBSEE_LIBS)

EXTRA_DIST=	    test.inc

noinst_PROGRAMS=    t-basic
noinst_PROGRAMS+=   t-string
noinst_PROGRAMS+=   t-bug81 
noinst_PROGRAMS+=   t-bug90
noinst_PROGRAMS+=   t-bug104
noinst_PROGRAMS+=   t-bug105
TESTS=		    $(noinst_PROGRAMS)
