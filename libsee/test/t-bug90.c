#include "test.inc"
#include <see/see.h>

static const SEE_char_t test_utf16[] =
    { 'd', 'i', 's', 'p', 'a', 't', 'c', 'h', 'E', 'v', 'e', 'n', 't' };
static struct SEE_string test_stringC =
    { 13, (SEE_char_t *)test_utf16 },
  *test_string = &test_stringC;

void
test()
{
	struct SEE_interpreter interp_storage, *interp;
	struct SEE_string *str, *str2;

	TEST_DESCRIBE("bug 90, SEE_intern_ascii");

	SEE_interpreter_init(&interp_storage);
	interp = &interp_storage;

	str = SEE_intern_ascii(interp, "dispatchEvent");

	TEST_NOT_NULL(str);
	TEST_EQ_STRING(str, test_string);

	str2 = SEE_intern_ascii(interp, "dispatchEvent");
	TEST_NOT_NULL(str2);
	TEST_EQ_STRING(str, str2);
}
