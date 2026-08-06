/* hello.c — the smallest real C program, fully annotated.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -g -O2 hello.c -o hello && ./hello
 */

/* The preprocessor replaces this line with the entire text of stdio.h.
 * That gives us the *declaration* of printf (its name, parameters, return type)
 * so the compiler can type-check our call. The actual machine code for printf
 * lives in libc and is attached by the linker, in stage 4. */
#include <stdio.h>

/* EXIT_SUCCESS / EXIT_FAILURE live here. Portable programs prefer them over
 * bare 0 and 1, because the numeric values are not guaranteed on every OS. */
#include <stdlib.h>

/* int main(void)
 *   int  -> the exit status handed back to the operating system
 *   void -> takes no arguments.
 *
 * Do NOT write `int main()`. In C (unlike C++) empty parentheses mean
 * "unspecified argument list", which switches OFF argument checking. */
int main(void)
{
    /* '\n' is one character: newline (ASCII 10). It also flushes a
     * line-buffered stdout, which is why output appears immediately in a
     * terminal. Piped to a file, stdout becomes block-buffered (4-8 KiB) and
     * nothing appears until the buffer fills or the program exits. */
    printf("Hello, world\n");

    /* printf returns the number of characters written, or a negative value on
     * error. Almost nobody checks it, but it exists. */
    int written = printf("This line was %d characters long (incl. newline)\n", 0);
    printf("printf reported: %d\n", written);

    /* Ways to leave main, all equivalent here:
     *     return 0;               <- the exit status
     *     return EXIT_SUCCESS;    <- same thing, self-documenting
     *     exit(EXIT_SUCCESS);     <- from <stdlib.h>, works from any function
     *     (falling off the end)   <- ONLY main gets this implicit return 0
     *
     * Check it from the shell afterwards with:  echo $?
     */
    return EXIT_SUCCESS;
}
