/* 03_io_and_input.c — formatted output, and how to read input WITHOUT scanf bugs.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_io_and_input.c -o t
 *   ./t                    # interactive
 *   printf '42\nhello\n' | ./t     # piped
 *   ./t < /dev/null        # EOF immediately — the code must survive this
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

/* ------------------------------------------------------------------ *
 * Reading a line safely.
 *
 * fgets(buf, size, stdin) reads at most size-1 bytes plus a NUL. It keeps
 * the '\n' if the line fitted. If it did not fit, there is no '\n' and the
 * rest of the line is still waiting in the stream — so we must drain it,
 * otherwise the next read picks up the leftovers.
 *
 * Returns true on success, false on EOF or error.
 * ------------------------------------------------------------------ */
static bool read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return false;                       /* EOF or read error */

    size_t len = strcspn(buf, "\n");        /* index of '\n', or of the NUL */
    if (buf[len] == '\n') {
        buf[len] = '\0';                    /* strip it */
    } else {
        /* No newline found: the line was longer than the buffer.
         * Discard the remainder so it does not contaminate the next read. */
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    return true;
}

/* ------------------------------------------------------------------ *
 * Parsing an integer strictly. strtol tells us three things atoi cannot:
 *   - where parsing stopped   (so we can reject "42abc")
 *   - whether nothing parsed  (so we can reject "abc")
 *   - whether it overflowed   (via errno == ERANGE)
 * ------------------------------------------------------------------ */
static bool parse_long(const char *s, long *out)
{
    if (s == NULL || *s == '\0')
        return false;

    errno = 0;                              /* strtol only SETS errno on error */
    char *end;
    long v = strtol(s, &end, 10);

    if (end == s)          return false;    /* no digits at all */
    while (*end == ' ' || *end == '\t') end++;  /* allow trailing spaces */
    if (*end != '\0')      return false;    /* trailing garbage: "42abc" */
    if (errno == ERANGE)   return false;    /* out of long's range */

    *out = v;
    return true;
}

/* Prompt until a valid integer in [lo, hi] is given, or EOF. */
static bool prompt_int(const char *prompt, long lo, long hi, long *out)
{
    char line[256];
    for (int attempt = 0; attempt < 5; attempt++) {
        printf("%s [%ld..%ld]: ", prompt, lo, hi);
        fflush(stdout);                     /* prompt has no '\n'; force it out */

        if (!read_line(line, sizeof line)) {
            puts("\n(end of input)");
            return false;
        }
        if (!parse_long(line, out)) {
            printf("  \"%s\" is not an integer. Try again.\n", line);
            continue;
        }
        if (*out < lo || *out > hi) {
            printf("  %ld is out of range.\n", *out);
            continue;
        }
        return true;
    }
    puts("  too many bad attempts");
    return false;
}

int main(void)
{
    /* ---------------- printf: the specifiers that matter ---------------- */
    puts("=== printf conversion specifiers ===");
    printf("  %%d  int              : %d\n",     -42);
    printf("  %%u  unsigned         : %u\n",     42u);
    printf("  %%ld long             : %ld\n",    123456789L);
    printf("  %%lld long long       : %lld\n",   1234567890123LL);
    printf("  %%zu size_t           : %zu\n",    sizeof(double));
    printf("  %%f  double           : %f\n",     3.14159);
    printf("  %%.2f 2 decimals      : %.2f\n",   3.14159);
    printf("  %%e  scientific       : %e\n",     314159.0);
    printf("  %%g  shorter of f/e   : %g\n",     0.000031415);
    printf("  %%c  char             : %c\n",     'A');
    printf("  %%s  string           : %s\n",     "text");
    printf("  %%x  hex              : %x   %%#x: %#x\n", 255, 255);
    printf("  %%o  octal            : %o\n",     8);
    /* %p takes a void*, and ISO C only lets you convert OBJECT pointers to
     * void* — function pointers are a separate universe (see module 04). */
    static int some_object;
    printf("  %%p  pointer          : %p   <- cast the argument to (void *)\n",
           (void *)&some_object);
    printf("  %%%%   a literal percent: %%\n");

    puts("\n=== width, precision, alignment ===");
    printf("  |%10s|%-10s|  right / left justified in 10 columns\n", "abc", "abc");
    printf("  |%08.3f|            zero-padded, 8 wide, 3 decimals\n", 3.14159);
    printf("  |%+d|%+d|           forced sign\n", 42, -42);
    printf("  |%*d|               width given as an argument (here 8)\n", 8, 42);
    printf("  |%.3s|               string truncated to 3 chars\n", "abcdefgh");

    puts("\n=== stdout vs stderr ===");
    printf("this goes to stdout — survives 2>/dev/null, vanishes with >/dev/null\n");
    fprintf(stderr, "this goes to stderr — the opposite\n");

    puts("\n=== WHY scanf(\"%s\", buf) IS A BUG ===");
    puts("  char buf[8]; scanf(\"%s\", buf);   <- no bound. Input of 100 chars");
    puts("  overruns buf and smashes the stack. This is THE classic exploit.");
    puts("  If you must use scanf, bound it: scanf(\"%7s\", buf) for char buf[8].");
    puts("  And check the return value: it is the NUMBER OF ITEMS ASSIGNED.");
    puts("  On bad input scanf returns 0 AND leaves the bad characters in the");
    puts("  stream, so `while (scanf(\"%d\",&n) != 1) ;` spins forever.");
    puts("\n  Use fgets + strtol instead. That is what read_line/parse_long do.\n");

    /* ---------------- the safe pattern, live ---------------- */
    puts("=== safe input demo (Ctrl-D / EOF to skip) ===");
    long n;
    if (prompt_int("Enter a number", 1, 100, &n)) {
        printf("  you entered %ld; %ld squared is %ld\n", n, n, n * n);
    }

    char name[64];
    printf("Enter your name: ");
    fflush(stdout);
    if (read_line(name, sizeof name)) {
        if (name[0] == '\0')
            puts("  (empty line)");
        else
            printf("  hello, %s (%zu characters)\n", name, strlen(name));
    }

    /* sscanf on a string you already own IS safe and convenient — the danger
     * is scanf reading unbounded from a stream, not the parsing itself. */
    puts("\n=== sscanf on an in-memory string is fine ===");
    {
        const char *record = "2024-06-01 temp=21.5 ok";
        int y, m, d; double temp;
        if (sscanf(record, "%d-%d-%d temp=%lf", &y, &m, &d, &temp) == 4)
            printf("  parsed: %04d/%02d/%02d  %.1f degrees\n", y, m, d, temp);
        else
            puts("  parse failed");
    }

    return 0;
}
