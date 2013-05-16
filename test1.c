/*
 * This program exists purely to allow 'make distcheck' to check
 * that an installation will allow a simple SEE program to 
 * be compiled and run.
 */

#include <see/see.h>

/* Simple example of using the interpreter */
int
main()
{
        struct SEE_interpreter interp_storage, *interp;
        struct SEE_input *input;
        SEE_try_context_t try_ctxt;
        struct SEE_value result;
        char *program_text = "Math.sqrt(3 + 4 * 7) + 9";

        /* Initialise an interpreter */
        SEE_interpreter_init(&interp_storage);
        interp = &interp_storage;

        /* Create an input stream that provides program text */
        input = SEE_input_utf8(interp, program_text);

        /* Establish an exception context */
        SEE_TRY(interp, try_ctxt) {
                /* Call the program evaluator */
                SEE_Global_eval(interp, input, &result);

                /* Print the result */
                if (SEE_VALUE_GET_TYPE(&result) == SEE_NUMBER)
                        printf("The answer is %f\n", result.u.number);
                else {
                        printf("Unexpected answer\n");
			exit(1);
		}
        }

        /* Finally: */
        SEE_INPUT_CLOSE(input);

        /* Catch any exceptions */
        if (SEE_CAUGHT(try_ctxt)) {
                printf("Unexpected exception\n");
		exit(1);
	}

        exit(0);
}
