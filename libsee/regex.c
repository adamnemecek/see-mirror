#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <see/system.h>
#include <see/error.h>
#include "regex.h"

#if WITH_PCRE
extern const struct SEE_regex_engine _SEE_pcre_regex_engine;
#endif

/* Parses a source pattern and returns a regex structure for later use */
struct regex *
SEE_regex_parse(interp, pattern, flags)
	struct SEE_interpreter *interp;
	struct SEE_string *pattern;
	int flags;
{
	SEE_ASSERT(interp, interp->regex_engine != NULL);
	return (*interp->regex_engine->parse)(interp, pattern, flags);
}

/* Returns the number of capture parentheses in the compiled regex */
int
SEE_regex_count_captures(regex)
	struct regex *regex;
{
	return (*regex->engine->count_captures)(regex);
}

/* Returns the flags of the expression (FLAG_GLOBAL, etc) */
int
SEE_regex_get_flags(regex)
	struct regex *regex;
{
	return (*regex->engine->get_flags)(regex);
}

/*
 * Executes the regex on the text beginning at index.
 * Returns true of a match was successful.
 */
int
SEE_regex_match(interp, regex, text, start, captures)
	struct SEE_interpreter *interp;
	struct regex *regex;
	struct SEE_string *text;
	unsigned int start;
	struct capture *captures;
{
	return (*regex->engine->match)(interp, regex, text, start, captures);
}

/* 
 * NOTE: Keep regex_name_list[] and regex_engine_list[] in sync!
 */

/* List of known regex engine names */
static const char *regex_name_list[] = {
	"ecma",
#if WITH_PCRE
	"pcre",
#endif
	NULL
};

/* List of known regex engines */
static const struct SEE_regex_engine *regex_engine_list[] = {
	&_SEE_ecma_regex_engine,
#if WITH_PCRE
	&_SEE_pcre_regex_engine,
#endif
	NULL
};

/* Returns a non-empty, read-only, NULL-terminated list of regex engine names */
const char **
SEE_regex_engine_list()
{
	return regex_name_list;
}

/* Returns the regex engine associated with a given name, or NULL if unknown */
const struct SEE_regex_engine *
SEE_regex_engine(name)
	const char *name;
{
	unsigned int i;
	
	for (i = 0; regex_name_list[i]; i++)
	    if (strcmp(name, regex_name_list[i]) == 0)
		return regex_engine_list[i];
	return NULL;
}

/* Initialises all the regex engines */
void
SEE_regex_init()
{
	unsigned int i;
	
	for (i = 0; regex_name_list[i]; i++)
	    if (regex_engine_list[i]->init)
		(*regex_engine_list[i]->init)();
}

