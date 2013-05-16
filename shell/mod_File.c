/* David Leonard, 2006. This file released into the public domain. */
/* $Id$ */

/*
 * This module is intended to provide an example of how to write a module
 * and host objects for SEE. Feel free to use it for the basis of
 * your own works.
 *
 * When loaded, this example provides the following objects based on
 * stdio FILEs:
 *
 *      File                    - constructor/opener object
 *      File.prototype          - container object for common methods
 *      File.prototype.read     - reads data from the file
 *      File.prototype.eof      - tells if a file is at EOF
 *      File.prototype.write    - writes data to a file
 *      File.prototype.flush    - flushes a file output
 *      File.prototype.close    - closes a file
 *      File.FileError          - object thrown when an error occurs
 *      File.In                 - standard input
 *      File.Out                - standard output
 *      File.Err                - standard error
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <see/see.h>

/* Prototypes */
static int File_mod_init(void);
static void File_alloc(struct SEE_interpreter *);
static void File_init(struct SEE_interpreter *);

static void file_construct(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static void file_proto_read(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void file_proto_eof(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void file_proto_write(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void file_proto_flush(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);
static void file_proto_close(struct SEE_interpreter *, struct SEE_object *,
        struct SEE_object *, int, struct SEE_value **, struct SEE_value *);

static struct file_object *tofile(struct SEE_interpreter *interp,
        struct SEE_object *);
static struct SEE_object *newfile(struct SEE_interpreter *interp,
        FILE *file);
static void file_finalize(struct SEE_interpreter *, void *, void *);

/*
 * The File_module structure is the only symbol exported here.
 * It contains some simple identification information but, most 
 * importantly, pointers to the major initialisation functions in
 * this file.
 * This structure is passed to SEE_module_add() once and early.
 */
struct SEE_module File_module = {
        SEE_MODULE_MAGIC,               /* magic */
        "File",                         /* name */
        "1.0",                          /* version */
        0,                              /* index (set by SEE) */
        File_mod_init,                  /* mod_init */
        File_alloc,                     /* alloc */
        File_init                       /* init */
};

/*
 * We use a private structure to hold per-interpeter data private to this
 * module. It can be accessed through the SEE_MODULE_PRIVATE() macro, or
 * through the simpler PRIVATE macro that we define below.
 * The private data we hold is simply original pointers to the objects
 * that we make during alloc/init. This is because (a) a script is able to
 * change the objects locations at runtime, and (b) this is slightly more
 * efficient than performing a runtime lookup with SEE_OBJECT_GET.
 */
struct module_private {
        struct SEE_object *File;                /* The File object */
        struct SEE_object *File_prototype;      /* File.prototype */
        struct SEE_object *FileError;           /* File.FileError */
};

#define PRIVATE(interp)  \
        ((struct module_private *)SEE_MODULE_PRIVATE(interp, &File_module))

/*
 * To make string usage more efficient, we globally intern some common 
 * strings and provide a STR() macro to access them.
 * Internalised strings are guaranteed to have unique pointers,
 * which means you can use '==' instead of 'strcmp()' to compare names.
 * The pointers must be allocated during mod_init() because the global
 * intern table is locked as soon as an interpreter instance is created.
 */
#define STR(name) s_##name
static struct SEE_string 
        *STR(Err),
        *STR(File),
        *STR(FileError),
        *STR(In),
        *STR(Out),
        *STR(close),
        *STR(eof),
        *STR(flush),
        *STR(prototype),
        *STR(read),
        *STR(write);

/*
 * This next structure is the one we use for a host object. It will
 * behave just like a normal ECAMscript object, except that we
 * "piggyback" a stdio FILE* onto it. A new instance is created whenever
 * you evaluate 'new File()'.
 *
 * The 'struct file_object' wraps a SEE_native object, but I could have
 * chosen SEE_object if I didn't want users to be able to store values
 * on it. Choosing a SEE_native structure adds a property hash table
 * to the object.
 *
 * Instances of this structure can be passed wherever a SEE_object 
 * or SEE_native is needed. (That's because the first field of a SEE_native
 * is a SEE_object.) Each instance's native.object.objectclass field must
 * point to the file_inst_class table defined below. Also, each instance's
 * native.object.prototype field (aka [[Prototype]]) should point to the
 * File.prototype object; that way the file methods can be found by the
 * normal prototype-chain method. (The search is automatically performed by 
 * SEE_native_get.)
 *
 * The alert reader will notice that later we create the special object 
 * File.prototype as an instance of this struct file_object too, except that
 * its file pointer is NULL, and its [[Prototype]] points to Object.prototype.
 */
struct file_object {
        struct SEE_native        native;
        FILE                    *file;
};

/*
 * The 'file_inst_class' class structure describes how to carry out 
 * all object operations on a file_object instance. You can see that
 * many of the function slots point directly to SEE_native_* functions. 
 * This is because file_object wraps a SEE_native structure, and we can
 * get all the standard ('native') ECMAScript object behaviour for free.
 * If we didn't want this behaviour, and instead used struct SEE_object as 
 * the basis of the file_object structure, then we should use the SEE_no_*
 * functions, or wrappers around them.
 *
 * In this class, there is no need for a [[Construct]] nor [[Call]]
 * property, so those are left at NULL. 
 */
static struct SEE_objectclass file_inst_class = {
        "File",                         /* Class */
        SEE_native_get,                 /* Get */
        SEE_native_put,                 /* Put */
        SEE_native_canput,              /* CanPut */
        SEE_native_hasproperty,         /* HasProperty */
        SEE_native_delete,              /* Delete */
        SEE_native_defaultvalue,        /* DefaultValue */
        SEE_native_enumerator,          /* DefaultValue */
        NULL,                           /* Construct */
        NULL,                           /* Call */
        NULL                            /* HasInstance */
};

/*
 * This is the class structure for the toplevel 'File' object. The 'File'
 * object doubles as both a constructor function and as a container object
 * to hold File.prototype and some other useful properties. 'File' has only 
 * one important intrinsic property, namely the [[Construct]] method. It
 * is called whenever the expression 'new File()' is evaluated.
 */
static struct SEE_objectclass file_constructor_class = {
        "File",                         /* Class */
        SEE_native_get,                 /* Get */
        SEE_native_put,                 /* Put */
        SEE_native_canput,              /* CanPut */
        SEE_native_hasproperty,         /* HasProperty */
        SEE_native_delete,              /* Delete */
        SEE_native_defaultvalue,        /* DefaultValue */
        SEE_native_enumerator,          /* DefaultValue */
        file_construct,                 /* Construct */
        NULL                            /* Call */
};

/*
 * mod_init: The module initialisation function.
 * This function is called exactly once, before any interpreters have
 * been created.
 * You can use this to set up global intern strings (like I've done),
 * and/or to initialise global data independent of any single interpreter.
 *
 * It is possible to call SEE_module_add() recursively from this
 * function. However, if a mod_init fails (returns non-zero), SEE
 * will also 'forget' about all the modules that it recursively added,
 * even if they succeeded. 
 */
static int
File_mod_init()
{
        STR(Err)       = SEE_intern_global("Err");
        STR(File)      = SEE_intern_global("File");
        STR(FileError) = SEE_intern_global("FileError");
        STR(In)        = SEE_intern_global("In");
        STR(Out)       = SEE_intern_global("Out");
        STR(close)     = SEE_intern_global("close");
        STR(eof)       = SEE_intern_global("eof");
        STR(flush)     = SEE_intern_global("flush");
        STR(prototype) = SEE_intern_global("prototype");
        STR(read)      = SEE_intern_global("read");
        STR(write)     = SEE_intern_global("write");
        return 0;
}

/*
 * alloc: Per-interpreter allocation function.
 * This optional function is called early during interpreter initialisation,
 * but before the interpreter is completely initialised. At this stage,
 * the interpreter is not really ready for use; only some storage has been
 * allocated, so you should not invoke any property accessors at this stage.
 * So, why is this function available? It turns out to be useful if you have 
 * mutually dependent modules that, during init(), need to find pointers in 
 * to the other modules.
 *
 * In this module, we use the alloc function simply to allocate the 
 * per-interpreter module-private storage structure, which we access
 * later through the PRIVATE() macro.
 */
static void
File_alloc(interp)
        struct SEE_interpreter *interp;
{
        SEE_MODULE_PRIVATE(interp, &File_module) =
                SEE_NEW(interp, struct module_private);
}

/*
 * init: Per-interpreter initialisation function.
 * This is the real workhorse of the module. Its job is to build up
 * an initial collection of host objects and install them into a fresh
 * interpreter instance. This function is called every time an interpreter
 * is created.
 *
 * Here we create the 'File' container/constructor object, and
 * populate it with 'File.prototype', 'File.In', etc. The 'File.prototype'
 * object is also created, and given its cfunction properties,
 * 'File.prototype.read', 'File.prototype.write', etc.
 * Functions and methods are most easily created using SEE_cfunction_make().
 *
 * Also, a 'File.FileError' exception is also created (for use when 
 * throwing read or write errors) and the whole tree of new objects is
 * published by inserting the toplevel 'File' object into the interpreter
 * by making it a property of the Global object.
 */
static void
File_init(interp)
        struct SEE_interpreter *interp;
{
        struct SEE_object *File, *File_prototype, *FileError;
        struct SEE_value v;

        /* Convenience macro for adding properties to File */
#define PUTOBJ(parent, name, obj)                                       \
        SEE_SET_OBJECT(&v, obj);                                        \
        SEE_OBJECT_PUT(interp, parent, STR(name), &v, SEE_ATTR_DEFAULT);

        /* Convenience macro for adding functions to File.prototype */
#define PUTFUNC(obj, name, len)                                         \
        SEE_SET_OBJECT(&v, SEE_cfunction_make(interp, file_proto_##name,\
                STR(name), len));                                       \
        SEE_OBJECT_PUT(interp, obj, STR(name), &v, SEE_ATTR_DEFAULT);

        /* Create the File.prototype object (cf. newfile()) */
        File_prototype = (struct SEE_object *)SEE_NEW(interp, 
                struct file_object);
        SEE_native_init((struct SEE_native *)File_prototype, interp,
                &file_inst_class, interp->Object_prototype);
        ((struct file_object *)File_prototype)->file = NULL;

        PUTFUNC(File_prototype, read, 0)
        PUTFUNC(File_prototype, write, 1)
        PUTFUNC(File_prototype, close, 0)
        PUTFUNC(File_prototype, eof, 0)
        PUTFUNC(File_prototype, flush, 0)

        /* Create the File object */
        File = (struct SEE_object *)SEE_NEW(interp, struct SEE_native);
        SEE_native_init((struct SEE_native *)File, interp,
                &file_constructor_class, interp->Object_prototype);
        PUTOBJ(interp->Global, File, File);
        PUTOBJ(File, prototype, File_prototype)

        /* Create the File.FileError error object for I/O exceptions */
        FileError = SEE_Error_make(interp, STR(FileError));
        PUTOBJ(File, FileError, FileError);

        /* Keep pointers to our 'original' objects */
        PRIVATE(interp)->File_prototype = File_prototype;
        PRIVATE(interp)->FileError = FileError;
        PRIVATE(interp)->File = File;

	/* Now we can build our well-known files */
        PUTOBJ(File, In, newfile(interp, stdin))
        PUTOBJ(File, Out, newfile(interp, stdout))
        PUTOBJ(File, Err, newfile(interp, stderr))
}
#undef PUTFUNC
#undef PUTOBJ

/*
 * Converts an object into file_object, or throws a TypeError.
 *
 * This helper functon is called by the method functions in File.prototype
 * mainly to check that each method is being called with a correct 'this'.
 * Because a script may assign the member functions objects to a different
 * (non-file) object and invoke them, we cannot assume the thisobj pointer 
 * always points to a 'struct file_object' structure. 
 * In effect, this function is a 'safe' cast.
 * NOTE: There is a check for null because 'thisobj' can potentially
 * be a NULL pointer.
 */
static struct file_object *
tofile(interp, o)
        struct SEE_interpreter *interp;
        struct SEE_object *o;
{
        if (!o || o->objectclass != &file_inst_class)
                SEE_error_throw(interp, interp->TypeError, NULL);
        return (struct file_object *)o;
}

/*
 * Constructs a file object instance.
 * This helper function constructs and returns a new instance of a 
 * file_object. It initialises the object with the given FILE pointer.
 */
static struct SEE_object *
newfile(interp, file)
        struct SEE_interpreter *interp;
        FILE *file;
{
        struct file_object *obj;

        obj = SEE_NEW_FINALIZE(interp, struct file_object, file_finalize, NULL);
        SEE_native_init(&obj->native, interp,
                &file_inst_class, PRIVATE(interp)->File_prototype);
        obj->file = file;
        return (struct SEE_object *)obj;
}

/*
 * A finalizer function that is (eventually) called on lost file objects.
 * If a system crash or exit occurs, this function may not be called. SEE 
 * cannot guarantee that this is ever called; however your garbage
 * collector implementation may guarantee it.
 */
static void
file_finalize(interp, obj, closure)
        struct SEE_interpreter *interp;
        void *obj, *closure;
{
        struct file_object *fo = (struct file_object *)obj;

        if (fo->file) {
                fclose(fo->file);
                fo->file = NULL;
        }
}

/*
 * new File(pathame [,mode]) -> object
 *
 * The File.[[Construct]] property is called when the user writes 
 * "new File()". This constructor expects two arguments (one is optional): 
 *      argv[0] the filename to open
 *      argv[1] the file mode (eg "r", "+b", etc) defaults to "r"
 * e.g.: new File("/tmp/foo", "r")
 * Throws a RangeError if the first argument is missing.
 *
 * Note: 'undefined' optional arguments are treated as if they were 
 * missing. (This is a common style in ECMAScript objects.)
 */
static void
file_construct(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        char *path, *mode = (char *)"r";
        FILE *file;
	struct SEE_object *newobj;

	SEE_parse_args(interp, argc, argv, "Z|Z", &path, &mode);
	if (!path)
                SEE_error_throw(interp, interp->RangeError, "missing argument");

        file = fopen(path, mode);
	if (!file) {
		/* May have too many files open; collect and try again */
		SEE_gcollect(interp);
		file = fopen(path, mode);
	}
        if (!file)
                SEE_error_throw(interp, PRIVATE(interp)->FileError,
                        "%s", strerror(errno));

	newobj = newfile(interp, file);
        SEE_SET_OBJECT(res, newobj);
}

/*
 * File.prototype.read([length]) -> string/undefined
 *
 * Reads and returns string data. If an argument is given, it limits the 
 * length of the string read. 
 * If the file is closed, this function return undefined.
 */
static void
file_proto_read(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct file_object *fo = tofile(interp, thisobj);
        SEE_uint32_t len, i;
        struct SEE_string *buf;
        int unbound;

        if (argc == 0 || SEE_VALUE_GET_TYPE(argv[0]) == SEE_UNDEFINED) {
                unbound = 1;
                len = 1024;
        } else {
                unbound = 0;
                len = SEE_ToUint32(interp, argv[0]);
        }
        if (!fo->file) {
                SEE_SET_UNDEFINED(res);
                return;
        }
        buf = SEE_string_new(interp, len);
        for (i = 0; unbound || i < len; i++) {
                int ch = fgetc(fo->file);
                if (ch == EOF)
                        break;
                SEE_string_addch(buf, ch);
        }
        SEE_SET_STRING(res, buf);
}

/*
 * File.prototype.eof() -> boolean/undefined
 *
 * Returns true if the last read resulted in an EOF. 
 * Closed files return undefined.
 */
static void
file_proto_eof(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct file_object *fo = tofile(interp, thisobj);

        if (!fo->file)
                SEE_SET_UNDEFINED(res);
        else
                SEE_SET_BOOLEAN(res, feof(fo->file));
}

/*
 * File.prototype.write(data)
 *
 * Writes the string argument to the file.
 * The string must be 8-bit data only.
 * Closed files throw an exception.
 */
static void
file_proto_write(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct file_object *fo = tofile(interp, thisobj);
        struct SEE_value v;
        unsigned int len;

        if (argc < 1)
                SEE_error_throw(interp, interp->RangeError, "missing argument");
        if (!fo->file)
                SEE_error_throw(interp, PRIVATE(interp)->FileError, 
                        "file is closed");
        SEE_ToString(interp, argv[0], &v);
        for (len = 0; len < v.u.string->length; len++) {
            if (v.u.string->data[len] > 0xff)
                SEE_error_throw(interp, interp->RangeError, 
                        "bad data");
            if (fputc(v.u.string->data[len], fo->file) == EOF)
                SEE_error_throw(interp, PRIVATE(interp)->FileError, 
                        "write error");
        }
        SEE_SET_UNDEFINED(res);
}

/*
 * File.prototype.flush()
 *
 * Flushes the file, if not already closed.
 */
static void
file_proto_flush(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct file_object *fo = tofile(interp, thisobj);

        if (fo->file)
                fflush(fo->file);
        SEE_SET_UNDEFINED(res);
}

/* 
 * File.prototype.close()
 *
 * Closes the file. The 'file' field is set to NULL to let other
 * member functions know that the file is closed.
 */
static void
file_proto_close(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct file_object *fo = tofile(interp, thisobj);

        if (fo->file) {
                fclose(fo->file);
                fo->file = NULL;
        }
        SEE_SET_UNDEFINED(res);
}
