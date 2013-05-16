/*
 * SEE shell environment objects
 *
 * The shell provides a couple of objects useful for testing.
 *
 *  print	- a function object which takes one
 *		  string argument and prints it, followed
 *		  by a newline.
 *  compat      - changes the compat flags. Returns old compat flags
 *  gc          - may force a garbage collection
 *  version	- a function that changes compatibility flags
 *		  based on an integer argument
 *
 *  Shell.isatty - function that returns true if stdout is a tty
 *  Shell.exit   - function to force immediate exit of shell
 *  Shell.abort  - force an interpreter abort
 *  Shell.gcdump - calls GC_dump(), if available
 *  Shell.regex_engines - returns array of regex engines
 *  Shell.regex_engine  - sets/gets the current interp's regex engine
 *
 * In HTML mode the following objects are provided:
 *
 *  document		- a simple Object with some properties:
 *  document.write	- a function to print a string to stdout
 *  document.navigator	- a simple Object used as a placeholder
 *  document.userAgent 	- "SEE-shell ($package-$version)"
 *  document.window	- a reference to the Global object
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <unistd.h>
#endif

#include <see/see.h>
#include "shell.h"
#include "compat.h"

struct method {
	const char *name;
	void (*fn)(struct SEE_interpreter *, struct SEE_object *, 
		struct SEE_object *, int, struct SEE_value **, 
		struct SEE_value *);
	int expected_args;
};

/* Prototypes */
static void add_methods(struct SEE_interpreter *, struct SEE_object *,
	const struct method *);
static void print_fn(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void document_write_fn(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void compat_fn(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void version_fn(struct SEE_interpreter *, struct SEE_object *, 
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void gc_fn(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void shell_abort_fn(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void shell_gcdump_fn(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void shell_isatty_fn(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void shell_exit_fn(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void shell_regex_engines_fn(struct SEE_interpreter *, 
	struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
	struct SEE_value *);
static void shell_regex_engine_fn(struct SEE_interpreter *, 
	struct SEE_object *, struct SEE_object *, int, struct SEE_value **, 
	struct SEE_value *);

/*
 * Adds useful symbols into the interpreter's internal symbol table. 
 * This speeds up access to the symbols and lets us use UTF-16 strings
 * natively when populating the object space. It's not strictly necessary
 * to do this.
 */
void
shell_strings()
{
	SEE_intern_global("print");
	SEE_intern_global("version");
	SEE_intern_global("document");
	SEE_intern_global("write");
	SEE_intern_global("navigator");
	SEE_intern_global("userAgent");
	SEE_intern_global("window");
	SEE_intern_global("gcdump");
	SEE_intern_global("gc");
	SEE_intern_global("isatty");
	SEE_intern_global("exit");
	SEE_intern_global("args");
	SEE_intern_global("Shell");
	SEE_intern_global("abort");
	SEE_intern_global("regex_engines");
	SEE_intern_global("regex_engine");
}

/*
 * A print function that prints all its string arguments to stdout.
 * A newline is printed at the end.
 */
static void
print_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;
	int i;

	for (i = 0; i < argc; i++) {
                SEE_ToString(interp, argv[i], &v);
                SEE_string_fputs(v.u.string, stdout);
        }
	printf("\n");
	fflush(stdout);
        SEE_SET_UNDEFINED(res);
}

/*
 * A function to modify the compatibility flags at runtime.
 */
static void
compat_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;
	char *buf;
	int i;
	struct SEE_string *old;
	
	old = compat_tostring(interp, interp->compatibility);
	if (argc > 0 && SEE_VALUE_GET_TYPE(argv[0]) != SEE_UNDEFINED) {
		SEE_ToString(interp, argv[0], &v);

		/* Convert argument to an ASCII C string */
		buf = SEE_STRING_ALLOCA(interp, char, v.u.string->length + 1);
		for (i = 0; i < v.u.string->length; i++)
		    if (v.u.string->data[i] > 0x7f)
		        SEE_error_throw(interp, interp->RangeError, 
			   "argument is not ASCII");
		    else
		    	buf[i] = v.u.string->data[i] & 0x7f;
		buf[i] = '\0';

		if (compat_fromstring(buf, &interp->compatibility) == -1)
		        SEE_error_throw(interp, interp->Error, 
			   "invalid flags");
	}

        SEE_SET_STRING(res, old);
}

/*
 * Query/change the Netscape JavaScript version compatibility value.
 *
 * If no argument is supplied, returns the current version number.
 * If a number is supplied, it is expected to be one of
 *	120, 130, 140, 150 indicating
 * Netscape JavaScript 1.0, 1.1, 1.2 etc.
 *
 * http://www.mozilla.org/rhino/overview.html#versions
 */
static void
version_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	SEE_number_t ver;
	struct SEE_value v;
	int compat;

	if (argc == 0) {
	    switch (SEE_GET_JS_COMPAT(interp)) {
	    case SEE_COMPAT_JS11: ver = 110; break;
	    case SEE_COMPAT_JS12: ver = 120; break;
	    case SEE_COMPAT_JS13: ver = 130; break;
	    case SEE_COMPAT_JS14: ver = 140; break;
	    default:
	    case SEE_COMPAT_JS15: ver = 150; break;
	    }
	    SEE_SET_NUMBER(res, ver);
	    return;
	}

	SEE_ToNumber(interp, argv[0], &v);
	ver = v.u.number;
	if (ver >= 150)
		compat = SEE_COMPAT_JS15;
	else if (ver >= 140)
		compat = SEE_COMPAT_JS14;
	else if (ver >= 130)
		compat = SEE_COMPAT_JS13;
	else if (ver >= 120)
		compat = SEE_COMPAT_JS12;
	else if (ver >= 110)
		compat = SEE_COMPAT_JS11;
	else 
		SEE_error_throw(interp, interp->RangeError, 
		   "cannot set version lower than JS1.1");

	SEE_SET_JS_COMPAT(interp, compat);
	SEE_SET_UNDEFINED(res);
}

/*
 * Dump the garbage collector.
 */
static void
shell_gcdump_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
#if WITH_BOEHM_GC
	void GC_dump(void);

	GC_dump();
#endif
        SEE_SET_UNDEFINED(res);
}

/*
 * Exit the shell process
 */
static void
shell_isatty_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
#if HAVE_ISATTY
	SEE_SET_BOOLEAN(res, isatty(1));
#else
	SEE_SET_BOOLEAN(res, 0);
#endif
}

/*
 * Exit the shell process
 */
static void
shell_exit_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	SEE_uint16_t exitcode = 0;

	if (argc > 0)
		exitcode = SEE_ToUint16(interp, argv[0]);
	exit(exitcode);
	/* NOTREACHED */
}

/*
 * Force an abort
 */
static void
shell_abort_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	char *msg;

	SEE_parse_args(interp, argc, argv, "a", &msg);
	SEE_ABORT(interp, msg);
        SEE_SET_UNDEFINED(res);
}

/*
 * Force a complete garbage collection
 */
static void
gc_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	SEE_gcollect(interp);
        SEE_SET_UNDEFINED(res);
}

/*
 * Return list of regex engines
 */
static void
shell_regex_engines_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	const char **names;
	struct SEE_value v;
	struct SEE_object *push;

	SEE_OBJECT_CONSTRUCT(interp, interp->Array, 0, 0, 0, res);
	SEE_OBJECT_GETA(interp, res->u.object, "push", &v);
	if (SEE_VALUE_GET_TYPE(&v) != SEE_OBJECT)
		SEE_error_throw(interp, interp->Error, 
		   "Array.push method not found");
	push = v.u.object;

	for (names = SEE_regex_engine_list(); *names; names++)
		SEE_call_args(interp, push, res->u.object, &v, "z", *names);
}

/*
 * Set regex engine, and return the name of the previous one
 */
static void
shell_regex_engine_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
	char *name;
	const char **names;
	const struct SEE_regex_engine *old_engine, *new_engine;

	SEE_parse_args(interp, argc, argv, "Z", &name);

	old_engine = interp->regex_engine;

	if (name) {
		new_engine = SEE_regex_engine(name);
		if (!new_engine)
		    SEE_error_throw(interp, interp->Error, 
		       "unknown engine '%.40s'", name);
		interp->regex_engine = new_engine;
	}

	/* Return the name for the old engine */
	for (names = SEE_regex_engine_list(); *names; names++)
	    if (SEE_regex_engine(*names) == old_engine)
		break;
	SEE_SET_STRING(res, SEE_string_sprintf(interp, "%s", 
	    *names ? *names : "?"));
}

static void
add_methods(interp, object, methods)
	struct SEE_interpreter *interp;
	struct SEE_object *object;
	const struct method *methods;
{
	unsigned int i;

	for (i = 0; methods[i].name; i++)
		SEE_CFUNCTION_PUTA(interp, object, 
		    methods[i].name, methods[i].fn,
		    methods[i].expected_args, 0);
}

/*
 * Adds global symbols 'print' and 'version' to the interpreter.
 * 'print' is a function and 'version' will be the undefined value.
 */
void
shell_add_globals(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_value v;
	struct SEE_object *Shell;

	static const struct method global_methods[] = {
		{ "print",		print_fn,		1 },
		{ "compat",		compat_fn,		1 },
		{ "gc",			gc_fn,			0 },
		{ "version",		version_fn,		1 },
		{0}
	}, shell_methods[] = {
		{ "gcdump",		shell_gcdump_fn,	0 },
		{ "isatty",		shell_isatty_fn,	0 },
		{ "exit",		shell_exit_fn,		1 },
		{ "abort",		shell_abort_fn,		1 },
		{ "regex_engines",	shell_regex_engines_fn,	0 },
		{ "regex_engine",	shell_regex_engine_fn,	1 },
		{0}
	};

	/* Create the print function, and attch to the Globals */
	add_methods(interp, interp->Global, global_methods);

	/* Create the Shell object */
	Shell = SEE_Object_new(interp);
	SEE_SET_OBJECT(&v, Shell);
	SEE_OBJECT_PUTA(interp, interp->Global, "Shell", &v, SEE_ATTR_DEFAULT);
	add_methods(interp, Shell, shell_methods);
}

/*
 * A write function that prints its output to stdout.
 * No newline is appended.
 */
static void
document_write_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;

        if (argc) {
                SEE_ToString(interp, argv[0], &v);
                SEE_string_fputs(v.u.string, stdout);
		fflush(stdout);
        }
        SEE_SET_UNDEFINED(res);
}

/*
 * Adds the following variables to emulate a browser environment:
 *   document             - dummy object
 *   document.write       - function to print strings
 *   navigator            - dummy object
 *   navigator.userAgent  - dummy string identifier for the SEE shell
 */
void
shell_add_document(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *document, *navigator;
	struct SEE_value v;

	static const struct method document_methods[] = {
		{ "write",		document_write_fn,	1 },
		{0}
	};

	/* Create a dummy 'document' object. Add it to the global space */
	document = SEE_Object_new(interp);
	SEE_SET_OBJECT(&v, document);
	SEE_OBJECT_PUTA(interp, interp->Global, "document", &v, 0);

	add_methods(interp, document, document_methods);

	/* Create a 'navigator' object and attach to the global space */
	navigator = SEE_Object_new(interp);
	SEE_SET_OBJECT(&v, navigator);
	SEE_OBJECT_PUTA(interp, interp->Global, "navigator", &v, 0);

	/* Create a string and attach as 'navigator.userAgent' */
	SEE_SET_STRING(&v, SEE_string_sprintf(interp, 
		"SEE-shell (%s-%s)", PACKAGE, VERSION));
	SEE_OBJECT_PUTA(interp, navigator, "userAgent", &v, 0);

	/* Create a dummy 'window' object */
	SEE_SET_OBJECT(&v, interp->Global);
	SEE_OBJECT_PUTA(interp, interp->Global, "window", &v, 0);
}
