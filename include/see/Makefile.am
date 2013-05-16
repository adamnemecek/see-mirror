## Process this file with automake to produce Makefile.in
## $Id: Makefile.am 1095 2006-07-02 12:07:37Z d $

PERL=@PERL@

BUILT_SOURCES =         error.h try.h type.h
pkginclude_HEADERS =	context.h cfunction.h debug.h error.h eval.h	\
                        input.h intern.h interpreter.h mem.h module.h	\
			native.h no.h object.h see.h string.h system.h 	\
			try.h type.h value.h version.h


# Rather than make our config.h be part of the API, we substitute
# configure-found values into the public API headers as needed.
# The perl script inc-subst is generated to perform the '@' substitutions
# that config.status should perform, but doesn't.

EXTRA_DIST=     error.h.in try.h.in type.h.in
CLEANFILES=	error.h try.h type.h inc-subst

inc-subst: $(CONFIG_HEADER)
	echo 'print "/* Automatically generated. Do not edit. */\n" if $$.==1;'\
		> inc-subst
	$(PERL) -n \
	 -e 'print if s!^#define\s+(\S+)\s+(.+)!s,\\\@$$1\\\@,$$2/*$$1*/,g;!;' \
	 -e 'print if s:^/\*\s*#undef\s+(\S+)\s*\*/.*:s,\\\@$$1\\\@,0/*!$$1*/,g;:;' \
	 < $(CONFIG_HEADER) >> inc-subst

error.h: error.h.in inc-subst
	$(PERL) -p inc-subst < $(srcdir)/error.h.in > error.h
try.h: try.h.in inc-subst
	$(PERL) -p inc-subst < $(srcdir)/try.h.in > try.h
type.h: type.h.in inc-subst
	$(PERL) -p inc-subst < $(srcdir)/type.h.in > type.h
