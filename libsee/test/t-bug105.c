#include "test.inc"
#include <see/see.h>

void
test()
{
	struct SEE_interpreter interp_storage1, *interp1 = &interp_storage1;
	struct SEE_interpreter interp_storage2, *interp2 = &interp_storage2;
	struct SEE_string *gstr, *str1, *str2, *s;

	TEST_DESCRIBE("bug 105, SEE_intern between interpreters");

	/* Globally interning a string should return a string with no interp */
	gstr = SEE_intern_global("something");
	TEST_NULL(gstr->interpreter);

	/* Re-interning the "something" should result in the same pointer */
	s = SEE_intern_global("something");
	TEST_EQ_PTR(s, gstr);

	/* Construct two separate interpreter contexts */
	SEE_interpreter_init(interp1);
	SEE_interpreter_init(interp2);

	/* Interning a string in interp #1 should result in a simple string */
	str1 = SEE_intern_ascii(interp1, "dispatchEvent");
	TEST_NOT_NULL(str1);
	TEST_EQ_PTR(str1->interpreter, interp1);

	/* MAIN TEST: interning the string from interp1 into interp2 
	 * should return a duplicated pointer that is local to interp2 */
	str2 = SEE_intern(interp2, str1);
	TEST_NOT_NULL(str2);
	TEST_NOT_EQ_PTR(str1, str2);
	TEST_EQ_STRING(str1, str2);
	TEST_EQ_PTR(str2->interpreter, interp2);
	TEST_EQ_PTR(str1->interpreter, interp1);

	/* Re-interning the str1 in interp1 shouldn't change it */
	s = SEE_intern(interp1, str1);
	TEST_EQ_PTR(s, str1);
	TEST_EQ_PTR(s->interpreter, interp1);

	/* Re-interning the str2 in interp2 shouldn't change it */
	s = SEE_intern(interp2, str2);
	TEST_EQ_PTR(s, str2);
	TEST_EQ_PTR(s->interpreter, interp2);

	/* Re-interning str2 into interp1 should get str1! */
	s = SEE_intern(interp1, str2);
	TEST_EQ_PTR(s, str1);

	/* Interning the global "something" should always result in gstr */
	s = SEE_intern(interp1, gstr);
	TEST_EQ_PTR(s, gstr);
	s = SEE_intern_ascii(interp1, "something");
	TEST_EQ_PTR(s, gstr);
	s = SEE_intern(interp2, gstr);
	TEST_EQ_PTR(s, gstr);
	s = SEE_intern_ascii(interp2, "something");
	TEST_EQ_PTR(s, gstr);
}
