#include "test.inc"
#include <see/see.h>

static void obj_finalize(struct SEE_interpreter *i, void *p, void *closure);

struct obj {
	struct SEE_interpreter *interp;
	struct obj *chain;
	unsigned int id;
};

static int initialized = 0;
static int finalized = 0;


static void 
obj_finalize(i, p, closure)
	struct SEE_interpreter *i;
	void *p;
	void *closure;
{
	struct obj *obj = (struct obj *)p;
	if (_test_verbose > 1)
	    printf("finalizing %d %s\n", obj->id, 
		closure ? (char *)closure : "");
	finalized++;
}

static void 
setup(interp, cascade, store)
	struct SEE_interpreter *interp;
	unsigned int cascade;
	void **store;
{
	struct obj *o, *ret = 0;

	for (; cascade; cascade--) {
	    o = SEE_NEW_FINALIZE(interp, struct obj, obj_finalize, 0);
	    o->id = cascade;
	    o->interp = interp;
	    o->chain = ret;
	    if (_test_verbose > 1)
		printf("initializing %d\n", o->id);
	    ret = o;
	    initialized++;
	}
	*store = (void *)ret;
}

#if TEST_CYCLES
static void 
setup2(interp, store)
	struct SEE_interpreter *interp;
	void **store;
{
	struct obj *o1, *o2;

	/* Create a cycle of two objects */
	o1 = SEE_NEW_FINALIZE(interp, struct obj, obj_finalize, "o1");
	o2 = SEE_NEW_FINALIZE(interp, struct obj, obj_finalize, "o2");
	o1->chain = o2;
	o2->chain = o1;
	initialized += 2;

	*store = (void *)o1;
}
#endif


void
test()
{
	struct SEE_interpreter interp_storage, *interp = &interp_storage;
	unsigned int old_finalized;

	TEST_DESCRIBE("bug 104, finalizers run");

	/* Only works if we have a GC */
	if (!SEE_system.gcollect)
	    TEST_EXIT_IGNORE();

	SEE_interpreter_init(interp);

#define COLLECT() do { \
	old_finalized = finalized; \
	SEE_gcollect(interp); \
	if (old_finalized != finalized && _test_verbose > 1) \
	    printf("collect: initialized %d finalized %d\n", \
		initialized,finalized); \
} while (old_finalized != finalized)

	/* Generate a chain of 100 objects that can cascade their finalizers */ 
	setup(interp, 100, &interp->host_data);
	COLLECT();
	TEST_NOT_EQ_INT(initialized, finalized);

	interp->host_data = 0;
	COLLECT();
	TEST_EQ_INT(initialized, finalized);

#if TEST_CYCLES
	/* Generate a cycle of two */
	setup2(interp, &interp->host_data);
	COLLECT();
	TEST_NOT_EQ_INT(initialized, finalized);

	/* XXX This appears to fail because the GC doesn't know which
	 * finalizer to run first! */
	interp->host_data = 0;
	COLLECT();
	TEST_EQ_INT(initialized, finalized);
# if WITH_BOEHM_GC
	{ void GC_dump(void); GC_dump(); }
# endif
#endif

}
