/* Copyright (c) 2006, David Leonard. All rights reserved. */

/*
 * Dynamic module loading. 
 * This code does the hard work to find, open the file and find its
 * exported struct SEE_module. Once the module is loaded, it can't 
 * be un/reloaded.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#if HAVE_STRING_H
# include <string.h>
#endif

#include <ltdl.h>

#include <see/see.h>
#include "module.h"

/* Loads a dynamic SEE module. Returns true on success. */
int
load_module(name)
	const char *name;
{
	lt_dlhandle handle = NULL;
	struct SEE_module *module;
	char symname[256];
	char libname[FILENAME_MAX + 1];

	if (lt_dlinit()) {
	    fprintf(stderr, "Cannot load modules: %s\n", lt_dlerror());
	    return 0;
	}

	handle = lt_dlopenext(name);

	/* If the filename is "foo", try "libfoo" */
	if (!handle && strchr(name, '/') == 0 && 
		strlen(name) + 3 < FILENAME_MAX)
	{
	    memcpy(libname, "lib", 3);
	    strcpy(libname + 3, name);
	    handle = lt_dlopenext(libname);
	}

	if (!handle) {
	    fprintf(stderr, "Cannot load module '%s': %s\n", 
	    	name, lt_dlerror());
	    goto fail;
	}

	/* Look for a simple exported "module" symbol first */
	module = (struct SEE_module *)lt_dlsym(handle, "module");

	if (!module) {
	    const char *p;
	    char *q;

	    /* If the file is "[/path/]foo.ext" try "foo_module" */
	    for (p = name; *p; p++) {}
	    while (p != name && p[-1] != '/') p--;
	    q = symname;
	    for (; *p && *p != '.'; p++) {
		*q++ = *p;
		if (q >= symname + sizeof symname)
		    goto fail;
	    }
	    for (p = "_module"; *p; p++) {
	    	*q++ = *p;
		if (q >= symname + sizeof symname)
		    goto fail;
	    }
	    *q = '\0';
	    module = (struct SEE_module *)lt_dlsym(handle, symname);

	    /* If the file name is "libfoo.ext", remove the "lib" and
	     * try "foo_module" */
	    if (!module && memcmp(symname, "lib", 3) == 0) {
	        module = (struct SEE_module *)lt_dlsym(handle, symname + 3);
	    }
	    	    
	}

	if (!module) {
		fprintf(stderr, "Cannot load module '%s': %s\n", 
			name, lt_dlerror());
	        goto fail;
	}

	if (module->magic != SEE_MODULE_MAGIC) {
		fprintf(stderr, "Cannot load module '%s': bad magic %x!=%x\n", 
			name, module->magic, SEE_MODULE_MAGIC);
	        goto fail;
	}

	if (SEE_module_add(module) != 0) {
		fprintf(stderr, "Error while loading module '%s'\n", name);
		goto fail;
	}

	return 1;

  fail:
	if (handle) lt_dlclose(handle);
  	lt_dlexit();
	return 0;
}
