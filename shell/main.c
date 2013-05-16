/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: main.c 1363 2008-02-08 14:24:55Z d $ */

/*
 * The SEE shell
 *
 * If no files are given, it prompts for javascript code interactively.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#if HAVE_GETOPT_H
# include <getopt.h>
#endif

#if !HAVE_GETOPT
int getopt(int, char *const [], const char *);
extern char *optarg;
extern int optind;
#endif

#if WITH_BOEHM_GC
# include <gc/gc.h>
#endif

#include <see/see.h>
#include "shell.h"
#include "compat.h"
#include "debug.h"
#include "module.h"
#include "gtrace.h"

/* Prototypes */
static void debug(int);
static void trace_enable(void);
static int run_input(struct SEE_interpreter *, struct SEE_input *, 
        struct SEE_value *);
static void run_file(struct SEE_interpreter *, char *);
static void run_interactive(struct SEE_interpreter *);
static void run_html(struct SEE_interpreter *, char *);
static void run_string(struct SEE_interpreter *, char *);

static struct debug *debugger;

/* 
 * Enables the debugging flag given by character c.
 * This relies on the SEE library having been compiled with 
 * debugging support (the default).
 */
static void
debug(c)
	int c;			/* promoted char */
{
#ifndef NDEBUG
	extern int SEE_native_debug, SEE_Error_debug, 
	SEE_parse_debug, SEE_lex_debug,
	SEE_eval_debug, SEE_error_debug,
	SEE_scope_debug, SEE_regex_debug,
	SEE_mem_debug;

	switch (c) {
	case 'E': SEE_Error_debug++; break;
	case 'T': trace_enable(); break;
	case 'c':
#if WITH_PARSER_CODEGEN
	{	extern int SEE_code_debug; SEE_code_debug++; }
#else
		fprintf(stderr, "debug flag 'c' ignored: no codegen\n");
#endif
		break;
	case 'e': SEE_error_debug++; break;
	case 'g': gtrace_enable(); break;
	case 'l': SEE_lex_debug++; break;
	case 'm': SEE_mem_debug++; break;
	case 'n': SEE_native_debug++; break;
	case 'p': SEE_parse_debug++; break;
	case 'r': SEE_regex_debug++; break;
	case 's': SEE_scope_debug++; break;
	case 'v': SEE_eval_debug++; break;
	default:
		fprintf(stderr, "unknown debug flag '%c'\n", c);
	}
#endif
}


/*
 * Trace function callback: prints current location to stderr.
 * This function is called when the -dT flag is given to enable
 * tracing. It is called by the parser at each evaluation step in
 * the program parse tree.
 */
static void
trace(interp, loc, context, event)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
	struct SEE_context *context;
	enum SEE_trace_event event;
{
	if (loc) {
	    fprintf(stderr, "trace: %s ",
	    	event == SEE_TRACE_CALL ? "CALL" :
	    	event == SEE_TRACE_RETURN ? "RETURN" :
	    	event == SEE_TRACE_STATEMENT ? "STATEMENT" :
	    	event == SEE_TRACE_THROW ? "THROW" :
		"???");
	    if (loc->filename) {
		SEE_string_fputs(loc->filename, stderr);
		fprintf(stderr, ", ");
	    }
	    fprintf(stderr, "line %d\n", loc->lineno);
	}
}

/* Enables verbose tracing */
static void
trace_enable()
{
	static int trace_enabled = 0;

	if (!trace_enabled) {
	    trace_enabled = 1;
	    shell_add_trace(trace);
	}
}

/*
 * Runs the input given by inp, printing any exceptions
 * to stderr.
 * This function first establishes a local exception catch context.
 * Next, it passes the unicode input provider ('inp') to the generic
 * evaluation procedure SEE_Global_eval which executes the program
 * text in the ECMAScript global context. This function also examines
 * the result of the evaluation, being careful to print out exceptions
 * correctly.
 * Returns 0 if an exception was uncaught, or 1 if the script ran to
 * completion.
 */
static int
run_input(interp, inp, res)
	struct SEE_interpreter *interp;
	struct SEE_input *inp;
	struct SEE_value *res;
{
	struct SEE_value v;
	SEE_try_context_t ctxt, ctxt2;

	interp->traceback = NULL;
        SEE_TRY (interp, ctxt) {
	    if (debugger)
	        debug_eval(interp, debugger, inp, res);
	    else
	        SEE_Global_eval(interp, inp, res);
        }
        if (SEE_CAUGHT(ctxt)) {
            fprintf(stderr, "exception:\n");
            SEE_TRY(interp, ctxt2) {
                SEE_ToString(interp, SEE_CAUGHT(ctxt), &v);
                fprintf(stderr, "  ");
                SEE_string_fputs(v.u.string, stderr);
                fprintf(stderr, "\n");
#ifndef NDEBUG
                if (ctxt.throw_file)
		    fprintf(stderr, "  (thrown from %s:%d)\n", 
                        ctxt.throw_file, ctxt.throw_line);
#endif
		SEE_PrintContextTraceback(interp, &ctxt, stderr);
            }
            if (SEE_CAUGHT(ctxt2)) {
		/* Exception while printing exception! */
                fprintf(stderr, "[exception thrown while printing exception");
		if (ctxt2.throw_file)
		    fprintf(stderr, " at %s:%d",
		        ctxt2.throw_file, ctxt2.throw_line);
                fprintf(stderr, "]\n");
	    }
	    return 0;
        }
        return 1;
}

/*
 * Opens the file and runs the contents as if ECMAScript code.
 * This function converts a local file into a unicode input stream,
 * and then calls run_input() above.
 */
static void
run_file(interp, filename)
	struct SEE_interpreter *interp;
	char *filename;
{
	struct SEE_input *inp;
	struct SEE_value res;
	FILE *f;
	int ok;

	if (strcmp(filename, "-") == 0)
		f = stdin;	/* Note: stdin will be closed after this */
	else {
		f = fopen(filename, "r");
		if (!f) {
		    perror(filename);
		    exit(4);	/* File argument not found */
		}
	}

	/*
	 * We check to see if the file starts with ""#!". If it does
	 * then it's likely a unix script and we consume to the first 
	 * newline. We have to do this before any byte order mark checks.
	 */
	if (fseek(f, 0, SEEK_SET) != -1) {
		long offset = 0;
		int ch;

		if (fgetc(f) == '#' && fgetc(f) == '!') {
		    offset = 2;
		    while ((ch = fgetc(f)) != EOF) 
		        if (ch == '\n')
			    break;
		        else
			    offset++;
	        }
	        fseek(f, offset, SEEK_SET);
        }

	inp = SEE_input_file(interp, f, filename, NULL);

	ok = run_input(interp, inp, &res);
	SEE_INPUT_CLOSE(inp);
	if (!ok)
		exit(3);	/* Runtime error (uncaught exception) */

}

/*
 * Reads lines of ECMAscript from the user
 * and runs each entered line in the one interpreter instance.
 * This function implements the 'command-prompt' loop. Each line
 * of text typed in is converted to a unicode input stream, and
 * sent to the run_input() function above.
 */
static void
run_interactive(interp)
	struct SEE_interpreter *interp;
{
	char *line;
	struct SEE_input *inp;
	struct SEE_value res;
	int len;

	for (;;) {
	    line = readline("> ");
	    if (line == NULL)
		break;
	    len = strlen(line);
	    while (len && line[len-1] == '\\') {
		char *a, *b;
		a = line;
		b = readline("+ ");
		if (!b) break;
		line = (char *)malloc(len + strlen(b) + 1);
		memcpy(line, a, len - 1);
		line[len-1] = '\n';
		strcpy(line + len, b);
		free(a);
		free(b);
		len = strlen(line);
	    }
	    inp = SEE_input_utf8(interp, line);
	    inp->filename = SEE_intern_ascii(interp, "<interactive>");
	    if (run_input(interp, inp, &res)) {
		printf(" = ");
		SEE_PrintValue(interp, &res, stdout);
		printf("\n");
	    }
            SEE_INPUT_CLOSE(inp);
	    free(line);
	}
}

/*
 * Runs the ECMAscript program passed as a command line argument
 */
static void
run_string(interp, program)
	struct SEE_interpreter *interp;
	char *program;
{
	struct SEE_input *inp;
	struct SEE_value res;
	int ok;

	inp = SEE_input_utf8(interp, program);
	inp->filename = SEE_intern_ascii(interp, "<command-line>");
	ok = run_input(interp, inp, &res);
	SEE_INPUT_CLOSE(inp);
	if (!ok)
		exit(3);	/* Runtime error (uncaught exception) */
}


/* Convert a character to uppercase */
#undef toupper
#define toupper(c) 	(((c) >= 'a' && (c) <= 'z') ? (c)-'a'+'A' : (c))

/*
 * Runs script elements in a HTML file.
 * This function opens a text file, assuming it to be HTML. It copies
 * the HTML verbatim until it finds a <SCRIPT> tag. At that point, it
 * reads and executes the text up to the closing </SCRIPT> tag. Further
 * HTML text read is also copied to standard output.
 */
static void
run_html(interp, filename)
	struct SEE_interpreter *interp;
	char *filename;
{
	FILE *f;
	int ch;
	const char *script_start = "<SCRIPT";
	const char *script_end = "</SCRIPT";
	const char *p;
	struct SEE_string *s = SEE_string_new(interp, 0);
	struct SEE_string *filenamestr;
	int endpos;
	int lineno = 1, first_lineno;
	struct SEE_input *inp;

	f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		return;
	}

	filenamestr = SEE_string_sprintf(interp, "%s", filename);

	p = script_start;
	while ((ch = fgetc(f)) != EOF) {
	    if (ch == '\n' || ch == '\r') lineno++;
	    if (toupper(ch) != *p) {
		if (p != script_start)
		    printf("%.*s", (int)(p - script_start), script_start);
		p = script_start;
	    }
	    if (toupper(ch) == *p) {
		p++;
		if (!*p) {
		    /* skip to closing > */
		    while ((ch = fgetc(f)) != EOF)  {
		        if (ch == '\n' || ch == '\r') lineno++;
			if (ch == '>') break;
		    }
		    /* capture content up to the end tag */
		    s->length = 0;
		    first_lineno = lineno;
		    p = script_end;
		    endpos = 0;
		    while ((ch = fgetc(f)) != EOF) {
		        if (ch == '\n' || ch == '\r') lineno++;
			if (toupper(ch) != *p) {
			    p = script_end;
			    endpos = s->length;
			}
			if (toupper(ch) == *p) {
			    p++;
			    if (!*p) {
				/* truncate string and skip to closing > */
				s->length = endpos;
				while ((ch = fgetc(f)) != EOF) {
				    if (ch == '\n' || ch == '\r') lineno++;
				    if (ch == '>') break;
				}
				break;
			    }
			}
			SEE_string_addch(s, ch);
		    }

		    inp = SEE_input_string(interp, s);
		    inp->filename = filenamestr;
		    inp->first_lineno = first_lineno;
		    run_input(interp, inp, NULL);

		    p = script_start;
		    continue;
		}
	    } else
	        putchar(ch);
	}
	fclose(f);
}

static void
add_shell_globals_once(interp)
	struct SEE_interpreter *interp;
{
	static int shell_globals_added = 0;

	if (!shell_globals_added) {
	    shell_add_globals(interp);
	    shell_globals_added = 1;
	}
}


static void
add_document_globals_once(interp)
	struct SEE_interpreter *interp;
{
	static int document_globals_added = 0;

	if (!document_globals_added) {
	    shell_add_document(interp);
	    document_globals_added = 1;
	}
}


int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct SEE_interpreter interp;
	int interp_initialised = 0;
	int ch, error = 0;
	int ran_something = 0;
	char *s;

#if WITH_BOEHM_GC
	GC_INIT();
#endif

	/* Initialise the shell's global strings */
	shell_strings();

	/* Helpful macro to initialise the interpreter just once */
#define INIT_INTERP_ONCE do {				\
	if (!interp_initialised) {			\
	    SEE_interpreter_init(&interp);		\
	    interp_initialised = 1;			\
	}						\
  } while (0)

	/* Helpful macro to check that flags come in the right order */
#define INTERP_NOT_INITTED(ch) do {				\
	if (interp_initialised) {			\
	    fprintf(stderr, "option -%c must come before -e/-f/-h/-i\n", ch);\
	    error = 1;					\
	    break;					\
	}						\
  } while (0)

	while (!error && (ch = getopt(argc, argv, "c:d:e:f:gh:il:r:V")) != -1)
	    switch (ch) {
	    case 'c':
		if (compat_tovalue(optarg, &SEE_system.default_compat_flags)
			== -1)
		    error = 1;
		if (interp_initialised)
		    interp.compatibility = SEE_system.default_compat_flags;
		break;

	    case 'd':
		if (*optarg == '*')
		    optarg = "nElpvecr";
		for (s = optarg; *s; s++)
		    debug(*s);
		if (interp_initialised)
		    interp.trace = SEE_system.default_trace;
		break;

	    case 'e':
		INIT_INTERP_ONCE;
		add_shell_globals_once(&interp);
		run_string(&interp, optarg);
		ran_something = 1;
		break;

	    case 'f':
		INIT_INTERP_ONCE;
		add_shell_globals_once(&interp);
		run_file(&interp, optarg);
		ran_something = 1;
		break;

	    case 'g':
		INIT_INTERP_ONCE;
	    	if (!debugger)
			debugger = debug_new(&interp);
		break;

	    case 'h':
		SEE_system.default_compat_flags |= SEE_COMPAT_SGMLCOM;
		INIT_INTERP_ONCE;
		add_document_globals_once(&interp);
		run_html(&interp, optarg);
		ran_something = 1;
		break;

	    case 'i':
		INIT_INTERP_ONCE;
		add_shell_globals_once(&interp);
		run_interactive(&interp);
		ran_something = 1;
		break;

	    case 'l':
		INTERP_NOT_INITTED(ch);
	    	if (!load_module(optarg))
			exit(1);
		break;

	    case 'r':
		SEE_system.default_recursion_limit = atoi(optarg);
		printf("(Set recursion limit to %d)\n", 
			SEE_system.default_recursion_limit);
		if (interp_initialised)
		    interp.recursion_limit = SEE_system.default_recursion_limit;
		break;

	    case 'V':
	    	printf("SEE API version: %u.%u\n", SEE_VERSION_API_MAJOR,
			SEE_VERSION_API_MINOR);
	    	printf("Library version: %s\n", SEE_version());
		exit(0);

	    default:
		error = 1;
	    }

	/* Don't expect any more arguments */
	if (optind < argc)
	    error = 1;

	if (error) {
	    fprintf(stderr, "usage: %s\n", argv[0]);
	    fprintf(stderr, "       [-Vg] [-c flag]\n");
	    fprintf(stderr, "       [-r maxrecurs]\n");
#ifndef NDEBUG
	    fprintf(stderr, "       [-d[ETcelmnprsv]]\n");
#endif
	    fprintf(stderr, "       [-l library]...\n");
	    fprintf(stderr, "       [-f file.js | -h file.html | -e program | -i]...\n");
	    exit(2); /* Invalid argument on command line */
	}

	if (!ran_something) {
	    INIT_INTERP_ONCE;
	    add_shell_globals_once(&interp);
	    run_interactive(&interp);
	}

	exit(0);	/* Success */
}
