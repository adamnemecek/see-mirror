#include "test.inc"
#include <see/see.h>

void
test()
{
	struct SEE_interpreter interp_storage, *interp = &interp_storage;
	struct SEE_string *s1, *s2;
	int val;

	TEST_DESCRIBE("string tests");

	SEE_interpreter_init(interp);

	s1 = SEE_intern_ascii(interp, "hello");
	s2 = SEE_string_dup(interp, s1);
	TEST_NOT_EQ_PTR(s1, s2);

	val = SEE_string_cmp(s1, s2);
	TEST_EQ_INT(val, 0);

	val = SEE_string_cmp_ascii(s1, "hello");
	TEST_EQ_INT(val, 0);
	val = SEE_string_cmp_ascii(s2, "hello");
	TEST_EQ_INT(val, 0);

	val = SEE_string_cmp_ascii(s1, "hellz");     /* hello < hellz */
	TEST_EQ_INT(val, -1);
	val = SEE_string_cmp_ascii(s1, "hella");     /* hello > hella */
	TEST_EQ_INT(val, +1);
	val = SEE_string_cmp_ascii(s1, "hell");      /* hello > hell */
	TEST_EQ_INT(val, +1);
	val = SEE_string_cmp_ascii(s1, "helloo");    /* hello < helloo */
	TEST_EQ_INT(val, -1);


	val = SEE_string_cmp(s1, SEE_intern_ascii(interp, "hellz"));
	TEST_EQ_INT(val, -1);
	val = SEE_string_cmp(s1, SEE_intern_ascii(interp, "hella"));
	TEST_EQ_INT(val, +1);
	val = SEE_string_cmp(s1, SEE_intern_ascii(interp, "hell"));
	TEST_EQ_INT(val, +1);
	val = SEE_string_cmp(s1, SEE_intern_ascii(interp, "helloo"));
	TEST_EQ_INT(val, -1);
}
