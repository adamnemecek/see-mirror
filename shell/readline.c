/* Copyright (c) 2005, David Leonard. All rights reserved. */
/* $Id$ */

/*
 * Simplified readline, provided for when GNU readline is not available.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if !HAVE_READLINE

# if STDC_HEADERS
#  include <stdio.h>
#  include <stdlib.h>
# endif

#if HAVE_STRING_H
#  include <string.h>
#endif

# if HAVE_UNISTD_H
#  include <unistd.h>
# endif

#include <see/see.h>
#include "shell.h"

# if !HAVE_STRDUP
static char *strdup(const char *);
# endif

# if !HAVE_STRDUP
/* Duplicates a string using dynamically allocated memory. */
static char *
strdup(s)
	const char *s;
{
	int len = strlen(s);
	char *t = (char *)malloc(len + 1);
	memcpy(t, s, len + 1);
	return t;
}
# endif /* !HAVE_STRDUP */

/*
 * Reads a line of prompted text from the user.
 * This is a simple replacement for GNU readline.
 */
char *
readline(prompt)
	const char *prompt;
{
	char buf[1024], *ret;
	int len;

	printf("%s", prompt);
	fflush(stdout);
	ret = fgets(buf, sizeof buf, stdin);
	if (!ret) return NULL;
	len = strlen(ret);
	while (len && (ret[len-1] == '\n' || ret[len-1] == '\r'))
		len--;
	ret[len] = '\0';
	return (char *)strdup(ret);
}

#endif /* !HAVE_READLINE */
