/* args.c — command-line arguments, exit codes, and the two output streams.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic args.c -o args
 *   ./args alpha beta 42        ; echo "exit status = $?"
 *   ./args                      ; echo "exit status = $?"
 *   ./args a b 2>/dev/null      # note which lines survive
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The second legal signature for main.
 *   argc  - argument count, always >= 1
 *   argv  - argument vector: argc+1 pointers, argv[argc] is guaranteed NULL
 *           argv[0] is conventionally the program name (do not rely on it
 *           being anything in particular — a caller can set it freely).
 */
int main(int argc, char **argv)
{
    /* stdout: normal program output. Redirect with >  */
    printf("program name : %s\n", argv[0]);
    printf("argument count: %d (%d real arguments)\n", argc, argc - 1);

    if (argc < 2) {
        /* stderr: diagnostics. Unbuffered, and survives `> file` redirection,
         * which is exactly why error messages belong here and not on stdout. */
        fprintf(stderr, "usage: %s <arg> [arg...]\n", argv[0]);
        return EXIT_FAILURE;   /* non-zero: the shell sees this as failure */
    }

    /* argv is NULL-terminated, so you can walk it without argc: */
    for (int i = 1; argv[i] != NULL; i++) {
        printf("  argv[%d] = \"%s\" (%zu chars)\n", i, argv[i], strlen(argv[i]));
    }

    /* Arguments are ALWAYS strings, even when they look like numbers.
     * strtol is the correct converter: it reports where parsing stopped, so
     * you can tell "42" from "42abc" from "abc". atoi() cannot, which is why
     * atoi is a trap — it returns 0 for garbage, and 0 is a valid number. */
    char *end;
    long n = strtol(argv[1], &end, 10);
    if (*end == '\0' && end != argv[1]) {
        printf("argv[1] parsed as the number %ld\n", n);
    } else {
        printf("argv[1] is not a plain integer (stopped at \"%s\")\n", end);
    }

    return EXIT_SUCCESS;
}
