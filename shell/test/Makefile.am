## Process this file with automake to produce Makefile.in
## $Id: Makefile.am 1359 2008-02-08 11:08:52Z d $

#
# Running 'make check' will run the parser over a basic grammar
# that exercises every branch of the language grammar, a couple
# of the border cases of RegExp and some of the examples found in
# the ECMA-262 standard.
#
# Much better tests can be fashioned out of the overwhelmingly
# comprehensive Mozilla test suite. See:
#	http://www.mozilla.org/js/tests/library.html
#

TESTS=		grammar.js
TESTS+=		regex.js
TESTS+=		function.js
TESTS+=		regress.js
TESTS+=		throw.js 
TESTS+=		obj.Global.js 
TESTS+=		obj.Object.js 
TESTS+=		obj.Function.js 

EXTRA_DIST=	common.js $(TESTS)
TESTS_ENVIRONMENT=  $(LIBTOOL) --mode=execute ../see-shell \
			$$TESTOPTS -f $(srcdir)/common.js -f
SUBDIRS=
