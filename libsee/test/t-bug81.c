#include "test.inc"
#include <see/see.h>

/* 6 character chinese phrase */
static const unsigned char sample_utf8[] = { 0xe6, 0x88, 0x91, 0xe4, 0xb8, 0x8d, 0xe6, 0x80, 0x80, 0xe8, 0xae, 0xb2, 0xe5, 0xa4, 0x96, 0xe8, 0xaf, 0xad, 0 };
static const SEE_char_t sample_utf16[] = 
    { 0x6211, 0x4e0d, 0x6000, 0x8bb2, 0x5916, 0x8bed };
static struct SEE_string sample_stringC = 
    { 6, (SEE_char_t *)sample_utf16 },
    *sample_string = &sample_stringC;

static const SEE_char_t undefined_utf16[] = 
    { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd' };
static struct SEE_string undefined_stringC = 
    { 9, (SEE_char_t *)undefined_utf16 },
    *undefined_string = &undefined_stringC;

static const SEE_char_t foo_utf16[] = 
    { 'f', 'o', 'o' };
static struct SEE_string foo_stringC = 
    { 3, (SEE_char_t *)foo_utf16 },
    *foo_string = &foo_stringC;

static void
mock_call(i, func, thisobj, argc, argv, res)
    struct SEE_interpreter *i;
    struct SEE_object *func;
    struct SEE_object *thisobj;
    int argc;
    struct SEE_value **argv;
    struct SEE_value *res;
{
    struct SEE_string *s1=0, *s2=0;
    char *a1=0, *a2=0, *a3=0, *a4=0, *a5=0, *a6=0, *a7=0;
    int i1, i2;
    SEE_int32_t ii1, ii2;
    SEE_uint16_t hh;
    struct SEE_value v1, v2, v3, v4;
    SEE_number_t n1;
    struct SEE_object *o1=0, *o2=0, *o3=0, *o4=0;
    extern struct SEE_objectclass _SEE_boolean_inst_class;

    TEST_EQ_INT(argc, 24);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[0]), SEE_STRING))
	TEST_EQ_PTR(argv[0]->u.string, sample_string);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[1]), SEE_UNDEFINED);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[2]), SEE_STRING))
	TEST_EQ_STRING(argv[2]->u.string, foo_string);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[3]), SEE_UNDEFINED);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[4]), SEE_STRING))
	TEST_EQ_STRING(argv[4]->u.string, foo_string);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[5]), SEE_STRING))
	TEST_EQ_STRING(argv[5]->u.string, sample_string);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[6]), SEE_UNDEFINED);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[7]), SEE_STRING))
	TEST_EQ_STRING(argv[7]->u.string, sample_string);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[8]), SEE_STRING))
	TEST_EQ_STRING(argv[8]->u.string, foo_string);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[9]), SEE_BOOLEAN))
	TEST(!argv[9]->u.boolean);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[10]), SEE_BOOLEAN))
	TEST(argv[10]->u.boolean);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[11]), SEE_NUMBER))
	TEST_EQ_FLOAT(argv[11]->u.number, -12345);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[12]), SEE_NUMBER))
	TEST_EQ_FLOAT(argv[12]->u.number, 12345);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[13]), SEE_NUMBER))
	TEST_EQ_FLOAT(argv[13]->u.number, 12345);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[14]), SEE_NULL);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[15]), SEE_NUMBER))
	TEST_EQ_FLOAT(argv[15]->u.number, 123.456);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[16]), SEE_OBJECT))
	TEST_EQ_PTR(argv[16]->u.object, i->Global);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[17]), SEE_UNDEFINED);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[18]), SEE_OBJECT))
	TEST_EQ_PTR(argv[18]->u.object, i->Global);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[19]), SEE_NULL);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[20]), SEE_OBJECT))
	TEST_EQ_PTR(argv[20]->u.object->objectclass, &_SEE_boolean_inst_class);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[21]), SEE_BOOLEAN))
	TEST(argv[21]->u.boolean);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[22]), SEE_UNDEFINED);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(argv[23]), SEE_UNDEFINED);

    /* Set up invalid pointers that should be NULL'd */
    s2 = (struct SEE_string *)1;
    a2 = (char *)1;
    a5 = (char *)1;
    o2 = (struct SEE_object *)1;
    o4 = (struct SEE_object *)1;
    SEE_parse_args(i, argc, argv, "ssAAaZZzabbiuhvnOOoOpvvx.",
	    &s1,          	   	/* s sample */
	    &s2,			/* s undefined */
	    &a1,    			/* A "foo" */
	    &a2, 			/* A undefined */
	    &a3,  			/* a "foo" */
	    &a4,        		/* Z sample */
	    &a5, 			/* Z undefined */
	    &a6,        		/* z sample */
	    &a7,     			/* a "foo" */
	    &i1,			/* b false */
	    &i2,			/* b true */
	    &ii1,  			/* i -12345 */
	    &ii2, 			/* u 12345 */
	    &hh,  			/* h 12345 */
	    &v1,			/* l null */
	    &n1,			/* n 123.456 */
	    &o1,           		/* O [[Global]] */
	    &o2, 			/* O undefined */
	    &o3,           		/* o [[Global]] */
	    &o4,           		/* O null */
	    &v2,     			/* p Boolean(true) */
	    &v3,    			/* v true */
	    &v4 			/* x undefined */
					/* x undefined */
	);

    TEST_EQ_STRING(s1, sample_string);
    TEST_EQ_STRING(s2, undefined_string);
    TEST_EQ_STR(a1, "foo");
    TEST_NULL(a2);
    TEST_EQ_STR(a3, "foo");
    TEST_EQ_STR(a4, sample_utf8);
    TEST_NULL(a5);
    TEST_EQ_STR(a6, sample_utf8);
    TEST_EQ_STR(a7, "foo");
    TEST_EQ_INT(i1, 0);
    TEST_EQ_INT(i2, 1);
    TEST_EQ_INT(ii1, -12345);
    TEST_EQ_INT(ii2, 12345);
    TEST_EQ_INT(hh, 12345);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(&v1), SEE_NULL);
    TEST_EQ_FLOAT(n1, 123.456);
    TEST_EQ_PTR(o1, i->Global);
    TEST_NULL(o2);
    TEST_EQ_PTR(o3, i->Global);
    TEST_NULL(o4);
    if (TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(&v2), SEE_BOOLEAN))
	TEST(v2.u.boolean);
    TEST(v3.u.boolean);
    TEST_EQ_TYPE(SEE_VALUE_GET_TYPE(&v4), SEE_UNDEFINED);

    SEE_SET_UNDEFINED(res);
}

void
test()
{
	struct SEE_interpreter interp_storage, *interp;
	struct SEE_object *func;
	struct SEE_value ret, trueval;

	TEST_DESCRIBE("bug 81, SEE_call_args, SEE_parse_args");

	SEE_interpreter_init(&interp_storage);
	interp = &interp_storage;

	/* Build a mock object whose [[Call]] is call() */
	func = SEE_cfunction_make(interp, mock_call, 
	    SEE_string_sprintf(interp, "mock_call"), 23);

	SEE_SET_BOOLEAN(&trueval, 1);
	SEE_call_args(interp, func, NULL, &ret,
	    "ssAAaZZz*bbiuhlnOOolpvxx",
	    sample_string,	   	/* s sample */
	    NULL,			/* s undefined */
	    "foo",			/* A "foo" */
	    NULL,			/* A undefined */
	    "foo",			/* a "foo" */
	    sample_utf8,		/* Z sample */
	    NULL,			/* Z undefined */
	    sample_utf8,		/* z sample */
	    "foo", 3,			/* * "foo" */
	    0,				/* b false */
	    -1,				/* b true */
	    -12345,			/* i -12345 */
	    12345,			/* u 12345 */
	    12345,			/* h 12345 */
					/* l null */
	    123.456,			/* n 123.456 */
	    interp->Global,		/* O [[Global]] */
	    NULL,			/* O undefined */
	    interp->Global,		/* o [[Global]] */
					/* l null */
	    &trueval,			/* p Boolean(true) */
	    &trueval			/* v true */
					/* x undefined */
					/* x undefined */
	);
}
