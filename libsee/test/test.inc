#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
#endif

#include <see/string.h>
#include <see/value.h>

/* Required for calling GC_INIT() */
#if WITH_BOEHM_GC
# include <gc/gc.h>
#endif

/*
 * This is a simple, generic test framework.
 * The main program should provide a void function called test(), 
 * which calls the following macros.
 */

/* Tests that a pointer is not null*/
#define TEST_NOT_NULL(a)    _TEST0((a) != 0, #a " != NULL")
/* Tests that a pointer is null*/
#define TEST_NULL(a)	    _TEST(!(a), #a " == NULL", (0,"0x%p == 0", (a)))
/* Tests two ints for equality */
#define TEST_EQ_INT(a,b)    _TEST((a)==(b), \
				#a " == " #b, (0,"%d == %d)",(a),(b)))
/* Tests two ints for inequality */
#define TEST_NOT_EQ_INT(a,b)    _TEST((a)!=(b), \
				#a " != " #b, (0,"%d != %d)",(a),(b)))
/* Tests two SEE_numbers for equality */
#define TEST_EQ_FLOAT(a,b)  _TEST0((a)==(b), #a " == " #b) /* XXX */
/* Tests two C strings for equality */
#define TEST_EQ_STR(a,b)    _TEST(strcmp((char *)(a), (char *)(b))==0, \
				#a " == " #b, (0, "'%s' == '%s'", (a), (b)))
/* Tests two SEE strings for equality */
#define TEST_EQ_STRING(a,b) _TEST(SEE_string_cmp(a,b)==0, \
				#a " == " #b, (0, "'%S' == '%S'", (a), (b)))
/* Tests two generic pointers for equality */
#define TEST_EQ_PTR(a,b)    _TEST((void *)(a)==(void *)(b), \
				#a " == " #b, (0, "0x%p == 0x%p", (a), (b)))
/* Tests two generic pointers for inequality */
#define TEST_NOT_EQ_PTR(a,b) _TEST((void *)(a)!=(void *)(b), \
				#a " != " #b, (0, "0x%p != 0x%p", (a), (b)))
/* Tests two SEE_types for equality */
#define TEST_EQ_TYPE(a, b)  _TEST((a) == (b), \
				#a " == " #b, (0, "%s == %s", \
				    _test_type_to_string(a), \
				    _test_type_to_string(b)))
/* Tests a general expression is true */
#define TEST(expr)	    _TEST(expr, #expr, (0, "false"))
/* Tests a general expression is false */
#define TEST_FALSE(expr)    _TEST(!(expr), "!("#expr")", (0,"!(true)"))
#define FAIL(msg)	    _TEST0(0, #msg)
#define PASS(msg)	    _TEST0(1, #msg)

/* Internal _TEST macro used to simplify the above */
#define _TEST(cond, desc, detail) \
	    _test((cond), desc, SEE_string_sprintf detail, __FILE__, __LINE__)
#define _TEST0(cond, desc) \
	    _test((cond), desc, 0, __FILE__, __LINE__)

/* If an entire test should be ignored, then call TEST_EXIT_IGNORE() */
#define TEST_EXIT_IGNORE()  exit(77)

#define TEST_DESCRIBE(txt)  _test_describe(txt)


/* Prototypes */
void test(void);	/* The function called from main() */
static int _test(int, const char *, struct SEE_string *, const char *, int);
const char * _test_basename(const char *);
static void _test_describe(const char *);
const char *_test_type_to_string(enum SEE_type t);

/* TEST internal state */
static int _test_count, _test_failures, _test_verbose=1, _test_strict,
	   _test_described;
static const char *_test_program;
static int _test_isatty;


/* Converts a value type into a string */
const char *
_test_type_to_string(t)
    enum SEE_type t;
{
    switch (t) {
    case SEE_UNDEFINED: return "SEE_UNDEFINED";
    case SEE_NULL:	return "SEE_NULL";
    case SEE_BOOLEAN:	return "SEE_BOOLEAN";
    case SEE_NUMBER:	return "SEE_NUMBER";
    case SEE_STRING:	return "SEE_STRING";
    case SEE_OBJECT:	return "SEE_OBJECT";
    case SEE_REFERENCE:	return "SEE_REFERENCE";
    case SEE_COMPLETION:return "SEE_COMPLETION";
    default:		return "<bad>";
    }
}

/* Returns the basename (last component) of a path */
const char *
_test_basename(const char *name)
{
    const char *n;
    const char *res = name;

    for (n = name; *n; n++)
	if (*n == '/')
	    res = n + 1;
    return res;
}

/* Logs the result of a test */
static int
_test(cond, desc, detail, file, line)
	int cond;
	const char *desc;
	struct SEE_string *detail;
	const char *file;
	int line;
{
	_test_count++;
	if (cond) {
		if (_test_verbose)
		    printf("%s line %3d: %s\n", 
			_test_isatty ? "[32mPASS[m" : "PASS",
			line, desc);
		return 1;
	} else {
		printf("%s line %3d: %s", 
		    _test_isatty ? "[41mFAIL[m" : "FAIL",
		    line, desc);
		if (detail && detail->length) {
		    printf(" [");
		    SEE_string_fputs(detail, stdout);
		    printf("]");
		}
		printf("\n");
		_test_failures++;
		if (_test_strict)
		    abort();
		return 0;
	}
}

static void
_test_describe(const char *txt)
{
	_test_described = 1;
	if (_test_verbose)
	    printf("%s: %s\n", _test_program, txt);
}

/* Driver */
int
main(int argc, char **argv)
{
	int optind;

#if HAVE_ISATTY
	_test_isatty = isatty(1);
#else
	_test_isatty = 0;
#endif

#if WITH_BOEHM_GC
	GC_INIT();
#endif

	_test_program = _test_basename(argv[0]);

	for (optind = 1; optind < argc; optind++) {
	    if (strcmp(argv[optind], "strict") == 0)
		_test_strict = 1;
	    else if (strcmp(argv[optind], "quiet") == 0)
		_test_verbose = 0;
	    else {
		fprintf(stderr, "%s: bad argument '%s'\n", 
		    _test_program, argv[optind]);
		fprintf(stderr, "usage: %s [strict] [quiet]\n", argv[0]);
		TEST_EXIT_IGNORE();
	    }
	}

	test();
	if (_test_verbose) 
	    printf("%d of %d passed (%.2f%%) [%s]\n",
		_test_count - _test_failures, _test_count,
		100.0 * (_test_count - _test_failures) / (double) _test_count,
		_test_program);

	if (!_test_described) {
	    fprintf(stderr, "%s: TEST_DESCRIBE() not called\n", argv[0]);
	    exit(1);
	}
	exit(_test_failures ? 1 : 0);
}

