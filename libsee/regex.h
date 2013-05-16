/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id: regex.h 1311 2008-01-10 13:59:14Z d $ */

#ifndef _SEE_h_regex_
#define _SEE_h_regex_

struct SEE_interpreter;
struct SEE_string;
struct SEE_object;

#define FLAG_GLOBAL     0x01	/* 'g'-flag */
#define FLAG_IGNORECASE 0x02	/* 'i'-flag */
#define FLAG_MULTILINE  0x04	/* 'm'-flag */

struct capture {
	unsigned int start, end;
};
#define CAPTURE_IS_UNDEFINED(c)	((c).end == -1)

struct regex {
    const struct SEE_regex_engine *engine;
    struct SEE_interpreter *interp;
};

struct SEE_regex_engine {
    void (*init)(void);
    struct regex *(*parse)(struct SEE_interpreter *interp, 
	    struct SEE_string *pattern, int flags);
    int (*count_captures)(struct regex *regex);
    int (*get_flags)(struct regex *regex);
    int (*match)(struct SEE_interpreter *interp, 
	    struct regex *regex, struct SEE_string *text, 
	    unsigned int start, struct capture *captures);
};

extern const struct SEE_regex_engine _SEE_ecma_regex_engine;

/* Normal interface to regex operations */
struct regex *SEE_regex_parse(struct SEE_interpreter *interp,
	struct SEE_string *pattern, int flags);
int SEE_regex_count_captures(struct regex *regex);
int SEE_regex_get_flags(struct regex *regex);
int SEE_regex_match(struct SEE_interpreter *interp,
	struct regex *regex, struct SEE_string *text,
	unsigned int start, struct capture *captures);

void SEE_regex_init(void);

/* defined in obj_RegExp.c to wrap RegExp objects: */
int SEE_is_RegExp(struct SEE_object *regexp);
int SEE_RegExp_match(struct SEE_interpreter *interp, 
	struct SEE_object *regexp, struct SEE_string *text, 
	unsigned int start, struct capture *captures);
int SEE_RegExp_count_captures(struct SEE_interpreter *interp,
	struct SEE_object *regexp);

#endif /* _SEE_h_regex_ */
