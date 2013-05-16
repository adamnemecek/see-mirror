#include <math.h>
#include "test.inc"
#include <see/see.h>

/*
 * This tests the example given in the documentation.
 */
void
test()
{
	struct SEE_interpreter interp_storage, *interp;
	struct SEE_input *input;
	SEE_try_context_t try_ctxt;
	struct SEE_value result;
	char *program_text = "Math.sqrt(3 + 4 * 7) + 9";

	TEST_DESCRIBE("the simple example from the documentation");

	/* Initialise the SEE library */
	SEE_init();

	/* Initialise an interpreter */
	SEE_interpreter_init(&interp_storage);
	interp = &interp_storage;

	/* Create an input stream that provides program text */
	input = SEE_input_utf8(interp, program_text);
	TEST_NOT_NULL(input);

	/* Establish an exception context */
	SEE_TRY(interp, try_ctxt) {
		/* Call the program evaluator */
		SEE_Global_eval(interp, input, &result);

		/* Print the result */
		TEST(SEE_VALUE_GET_TYPE(&result) == SEE_NUMBER);
		if (SEE_VALUE_GET_TYPE(&result) == SEE_NUMBER)
			TEST_EQ_FLOAT(result.u.number, sqrt(3+4*7.)+9);
	}

	/* Finally: */
	SEE_INPUT_CLOSE(input);

	TEST_NULL(SEE_CAUGHT(try_ctxt));
}
