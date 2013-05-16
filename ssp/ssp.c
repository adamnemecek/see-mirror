#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <see/see.h>
#include "httpd.h"
#include "ssp.h"
#include "pool.h"

/*
 * An input stream around a SSP file.
 * This does two things. First it saves segments of text from the file into 
 * global strings named __inputN, where N is an increasing number.
 * Secondly it replaces the segments with javascript statements of the
 * form 'print(__inputN);'. The idea is that executing the script
 * will print the unescaped text.
 *
 * While processing, the input stream is in one of two states:
 *	COPY	- copying through javascript code without change
 *                until a '%>' is encountered 
 *	TEXT	- emitting the content of the text[] field
 *	NLS	- emitting newlines after the text[] field
 */
struct ssp_input {
	struct SEE_input input;
	FILE *f;
	enum { SSP_COPY, SSP_TEXT, SSP_NL } state;
	int counter;			/* text segment id genertator */
	char text[256];			/* inserted js code */
	int textpos;			/* read position in text[] */
	int nlcount;			/* newlines to insert after text[] */
	int first;			/* set only when "<%" is seen */
	int trail_needed;		/* indicates trailing ');' is needed*/
};

/*
 * A structure attached to each SEE interpreter's host_data field
 */
struct ssp_state {
	FILE *fp;
	struct pool *pool;
	int headers_sent;		/* true if HTTP header sent */
	int raw;			/* true if raw JS to be sent */
	int response_code;		/* usually 200 */
};
#define SSP_STATE(interp)  ((struct ssp_state *)(interp)->host_data)

/* prototypes */
static int read_text(struct ssp_input *inp);
static struct SEE_input *ssp_input_new(struct SEE_interpreter *interp, 
	const char *filename);
static SEE_unicode_t ssp_next(struct SEE_input *input);
static void ssp_close(struct SEE_input *input);
static void *ssp_malloc(struct SEE_interpreter *, SEE_size_t);
static void  ssp_free(struct SEE_interpreter *, void *);
static struct SEE_object *make_headers_object(struct SEE_interpreter *,
	struct header *);

static struct SEE_inputclass ssp_inputclass = { ssp_next, ssp_close };

/*
 * Sets up SEE so that it uses a per-interpreter allocator.
 * Our allocators remember what mallocs were performed, and
 * when a request is complete, we can simply dump all memory
 * associated with that interpreter. This is not a good approach
 * for long-running scripts, but for web-based applications it
 * is probably sufficient.
 */
void
ssp_init()
{
	SEE_system.malloc          = ssp_malloc;
	SEE_system.malloc_finalize = NULL;
	SEE_system.malloc_string   = ssp_malloc;
	SEE_system.free            = ssp_free;
	SEE_system.gcollect        = NULL;
}

/*
 * Allocates storage for an interpreter. Allocated blocks
 * are wrapped in a linked list rooted in the interpreter's ssp_state.
 */
static void *
ssp_malloc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	if (!interp)
		return malloc(size);
	return pool_malloc(SSP_STATE(interp)->pool, size);
}

/*
 * Sometimes SEE calls free(). It can be safely ignored.
 */
static void 
ssp_free(interp, ptr)
	struct SEE_interpreter *interp;
	void *ptr;
{
	if (!interp)
		free(ptr);
}

/* Reads text up until EOF or "<%". Returns -1 if EOF was immediately 
 * read. Sets the text[] array to the JS code that will print the read
 * text. Also increments nlcount by the number of newlines in the
 * text segment */
static int
read_text(inp)
	struct ssp_input *inp;
{
	int ch, ch2;
	struct SEE_interpreter *interp = inp->input.interpreter;
	struct SEE_string *str;
	char label[32];
	struct SEE_value v;
	int textlen;
	
	inp->nlcount = 0;
	inp->first = 0;
	ch = getc(inp->f);
	if (ch == EOF)
		return -1;
	str = SEE_string_new(interp, 0);
	do {
		if (ch == '<') {
		    if ((ch2 = getc(inp->f)) == '%') {
		    	inp->first = 1;
		        break;
		    }
		    SEE_string_addch(str, '<');
		    ch = ch2;
		    if (ch == EOF)
		    	break;
		}
	        SEE_string_addch(str, ch);
		if (ch == '\n')
			inp->nlcount++;
	} while ((ch = getc(inp->f)) != EOF);

	if (str->length) {
		inp->counter++;
		snprintf(label, sizeof label, "__input%u", inp->counter);
		SEE_SET_STRING(&v, str);
		SEE_OBJECT_PUTA(interp, interp->Global, label, &v, 
			SEE_ATTR_DONTENUM | SEE_ATTR_READONLY | 
			SEE_ATTR_DONTDELETE);
		textlen = strlen(inp->text);
		snprintf(inp->text + textlen, sizeof inp->text - textlen, 
			";print(%s);", label);
	} else
		inp->text[0] = 0;

	inp->textpos = 0;
	return 0;
}

/* Creates an input object that generates JS program text from an SSP file */
static struct SEE_input *
ssp_input_new(interp, filename)
	struct SEE_interpreter *interp;
	const char *filename;
{
	struct ssp_input *inp = SEE_NEW(interp, struct ssp_input);

	inp->input.inputclass = &ssp_inputclass;
	inp->input.interpreter = interp;
	inp->input.eof = 0;
	inp->input.lookahead = 0;
	inp->input.filename = SEE_string_sprintf(interp, "%s", filename);
	inp->input.first_lineno = 1;
	inp->f = fopen(filename, "rb");
	if (!inp->f) {
		warn("%s", filename);
		return NULL;
	}
	inp->counter = 0;
	inp->trail_needed = 0;
	inp->text[0] = 0;

	if (read_text(inp)) {
		/* empty file */
		inp->input.eof = 1;
		return &inp->input;
	}
	inp->state = SSP_TEXT;

	ssp_next(&inp->input);	/* prime */
	return &inp->input;
}

/* Generates the next JS character from an SSP file */
static SEE_unicode_t
ssp_next(input)
	struct SEE_input *input;
{
	struct ssp_input *inp = (struct ssp_input *)input;
	int ch;
	SEE_unicode_t ret = inp->input.lookahead;

  again:

	if (inp->state == SSP_TEXT) {
		if (inp->text[inp->textpos] == 0) {
			inp->text[0] = 0;
			inp->state = SSP_NL;
		} else {
			inp->input.lookahead = inp->text[inp->textpos];
			inp->textpos++;
			return ret;
		}
	}

	if (inp->state == SSP_NL) {
		if (inp->nlcount == 0)
			inp->state = SSP_COPY;
		else {
			inp->nlcount--;
			inp->input.lookahead = '\n';
			return ret;
		}
	}

	ch = getc(inp->f);
	if (ch == EOF) {
		inp->input.eof = 1;
		return ret;
	}

	/*
	 * If the first character after <% is = then we insert
	 * 'print(' and expect the following to be an ecmascript
	 * expression. We also set the trail_needed flag so that
	 * when the '%>' is encountered, a closing ')' is inserted.
	 */
	if (inp->first) {
		inp->first = 0;
		if (ch == '=') {
		    inp->first = 0;
		    snprintf(inp->text, sizeof inp->text, ";print(");
		    inp->textpos = 0;
		    inp->nlcount = 0;
		    inp->trail_needed = 1;
		    inp->state = SSP_TEXT;
		    goto again;
		}
	}

	if (ch == '%') {
		ch = getc(inp->f);
		if (ch == EOF) {
			warnx("file ended with %%");
			inp->input.eof = 1;
			return ret;
		} else if (ch == '>') {
			if (inp->trail_needed) {
				snprintf(inp->text, sizeof inp->text, ");");
				inp->trail_needed = 0;
			}
			if (read_text(inp)) {
			    inp->input.eof = 1;
			    return ret;
			}
		} else {
			inp->text[0] = '%';
			inp->text[1] = ch;	/* XXX ch may be '%' */
			inp->text[2] = 0;
			inp->textpos = 0;
			inp->nlcount = 0;
		}
		inp->state = SSP_TEXT;
		goto again;
	}

	inp->input.lookahead = ch;
	return ret;
}

/*
 * Closes the input stream. Releases resources allocated by
 * ssp_input_new().
 */
static void
ssp_close(input)
	struct SEE_input *input;
{
	struct ssp_input *inp = (struct ssp_input *)input;

	fclose(inp->f);
}

/*
 * Writes the HTTP header if not already sent.
 */
static void
ssp_flush_header(interp)
	struct SEE_interpreter *interp;
{
	if (!SSP_STATE(interp)->headers_sent) {
		FILE *fp = SSP_STATE(interp)->fp;

		fprintf(fp, "HTTP/1.0 %u\r\n", 
			SSP_STATE(interp)->response_code);
		fprintf(fp, "Content-Type: text/plain\r\n");
		fprintf(fp, "\r\n");
		SSP_STATE(interp)->headers_sent = 1;
	}
}

/*
 * print() function provided to the interpreter environment.
 * Simply writes to the stdio file pointer given to this request.
 */
static void
print_fn(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	struct SEE_string *s;

	SEE_parse_args(interp, argc, argv, "s", &s);
	if (s) {
		ssp_flush_header(interp);
		SEE_string_fputs(s, SSP_STATE(interp)->fp);
	}
	SEE_SET_UNDEFINED(res);
}

static struct SEE_object *
make_headers_object(interp, headers)
	struct SEE_interpreter *interp;
	struct header *headers;
{
	struct SEE_object *obj;
	struct SEE_value v;
	struct header *h;

	obj = SEE_Object_new(interp);
	for (h = headers; h; h = h->next) {
		SEE_SET_STRING(&v, 
			SEE_string_sprintf(interp, "%s", h->value));
		SEE_OBJECT_PUTA(interp, obj, h->name, &v, 0);
	}
	return obj;
}

/* Includes a file, treating it as SSP */
static void
ssp_include(interp, path)
	struct SEE_interpreter *interp;
	const char *path;
{
	struct SEE_input *input;
	SEE_try_context_t ctxt;
	struct SEE_value res;

	/* Start up the code input stream generator */
	input = ssp_input_new(interp, path);
	if (!input) {
		SEE_error_throw(interp, interp->Error, 
			"cannot create input stream for %s", path);
		return;
	}

	/* Parse and execute the input in one go; trapping exceptions */
	SEE_TRY(interp, ctxt) {
	    if (SSP_STATE(interp)->raw)
		/* Print the generated script (for debugging) */
		while (!input->eof) {
			int ch = SEE_INPUT_NEXT(input) & 0x7f;
			putc(ch, SSP_STATE(interp)->fp);
		}
	    else
		/* Execute the generated script */
		SEE_Global_eval(interp, input, &res);
	}
	/* Finally: close the input */
	SEE_INPUT_CLOSE(input);
	/* Rethrow any exception */
	SEE_DEFAULT_CATCH(interp, ctxt);
}


/*
 * include(): includes and runs another file
 */
static void
include_fn(interp, self, thisobj, argc, argv, res)
	struct SEE_interpreter *interp;
	struct SEE_object *self, *thisobj;
	int argc;
	struct SEE_value **argv, *res;
{
	char *path;

	SEE_parse_args(interp, argc, argv, "z", &path);
	if (path)
		ssp_include(interp, path);
	SEE_SET_UNDEFINED(res);
}

/*
 * Processes a request for an SSP file.
 * The URI is opened as a file relative to the current directory,
 * its contents converted into a (large) SEE script, and then
 * it is executed.
 */
void
process_request(fp, method, uri, headers)
	FILE *fp;
	const char *method;
	const char *uri;
	struct header *headers;
{
	struct SEE_interpreter interp;
	char *query_string, *s;
	SEE_try_context_t ctxt;
	struct SEE_value v;
	struct ssp_state ssp_state;

	s = strchr(uri, '?');
	if (s) {
		*s++ = 0;
		query_string = s;
	} else
		query_string = "";

	ssp_state.fp = fp;
	ssp_state.headers_sent = 0;
	ssp_state.raw = strcmp(query_string, "raw") == 0;
	ssp_state.response_code = 200;
	ssp_state.pool = pool_new();

	/* Create an interpreter instance that uses our memory allocator */
	interp.host_data = &ssp_state;
	SEE_interpreter_init(&interp);

	/* Insert the print() function into the interpreter context */
	SEE_CFUNCTION_PUTA(&interp, interp.Global, "print", print_fn, 1, 0);
	SEE_CFUNCTION_PUTA(&interp, interp.Global, "include", include_fn, 1, 0);

	/* Set QUERY_STRING and other global variable */
	SEE_SET_STRING(&v, SEE_string_sprintf(&interp, "%s", query_string));
	SEE_OBJECT_PUTA(&interp, interp.Global, "QUERY_STRING", &v, 
		SEE_ATTR_DEFAULT);
	SEE_SET_STRING(&v, SEE_string_sprintf(&interp, "%s", method));
	SEE_OBJECT_PUTA(&interp, interp.Global, "REQUEST_METHOD", &v, 
		SEE_ATTR_DEFAULT);
	SEE_SET_STRING(&v, SEE_string_sprintf(&interp, "%s", uri));
	SEE_OBJECT_PUTA(&interp, interp.Global, "REQUEST_URI", &v, 
		SEE_ATTR_DEFAULT);
	SEE_SET_OBJECT(&v, make_headers_object(&interp, headers));
	SEE_OBJECT_PUTA(&interp, interp.Global, "HEADER", &v, 
		SEE_ATTR_DEFAULT);

	/* Include the file named by the URI */
	SEE_TRY(&interp, ctxt) {
		ssp_include(&interp, uri + 1);
	}

	/* Print any exceptions to stderr */
	if (SEE_CAUGHT(ctxt)) {
		SEE_try_context_t ctxt2;

		ssp_state.response_code = 500;
		SEE_TRY(&interp, ctxt2) {
			SEE_ToString(&interp, SEE_CAUGHT(ctxt), &v);
			fprintf(stderr, "exception:  ");
			SEE_string_fputs(v.u.string, stderr);
			fprintf(stderr, "\n");
			SEE_PrintContextTraceback(&interp, &ctxt, stderr);
		}
		if (SEE_CAUGHT(ctxt2)) {
			/* Exception while printing exception! */
			fprintf(stderr, "nested exception error\n");
		}
	}

	ssp_flush_header(interp);
	fflush(fp);


	/* Release memory */
	pool_destroy(SSP_STATE(&interp)->pool);
}


