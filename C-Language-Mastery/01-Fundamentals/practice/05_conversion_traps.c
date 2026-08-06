/* 05_conversion_traps.c — implicit conversions, the #1 source of "impossible" bugs.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 05_conversion_traps.c -o t && ./t
 *
 * -Wextra flags several of these at compile time. That is the real lesson:
 * the compiler already knows. You just have to let it tell you.
 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

/* Deliberately takes size_t so callers hit the promotion rules. */
static size_t count_chars(const char *s) { return strlen(s); }

int main(void)
{
    puts("=== TRAP 1: signed compared against unsigned ===");
    {
        int      i = -1;
        unsigned u =  1;
        /* Usual arithmetic conversions: both operands go to unsigned int,
         * so -1 becomes UINT_MAX (4294967295). */
        printf("  i = %d, u = %u\n", i, u);
        printf("  (i < u) is %s   <- -1 was converted to %u\n",
               ((unsigned)i < u) ? "TRUE" : "FALSE", (unsigned)i);
        printf("  ((long)i < (long)u) is %s   <- widen BOTH to a signed type that fits\n",
               ((long)i < (long)u) ? "true" : "false");
    }

    puts("\n=== TRAP 2: the empty-string loop that runs forever ===");
    {
        const char *empty = "";
        size_t len = count_chars(empty);            /* 0 */
        printf("  strlen(\"\") = %zu, and strlen(\"\") - 1 = %zu\n", len, len - 1);
        puts("  So `for (size_t i = 0; i < strlen(s) - 1; i++)` on an empty string");
        puts("  iterates SIZE_MAX times and walks off the end of memory.");
        puts("  Fix: hoist the length, and compare with i + 1 < len.");
        for (size_t i = 0; i + 1 < len; i++) { /* correctly does nothing */ }
        puts("  (the corrected loop above executed 0 times)");
    }

    puts("\n=== TRAP 3: narrowing on assignment silently discards bits ===");
    {
        int   big  = 300;
        char  c    = (char)big;        /* 300 & 0xFF = 44 = ',' */
        short s    = (short)70000;     /* 70000 - 65536 = 4464 */
        printf("  (char)300    = %d  ('%c')\n", c, c);
        printf("  (short)70000 = %d\n", s);
        puts("  Without the explicit cast, -Wconversion warns. Without -Wconversion,");
        puts("  nothing warns at all — this is legal C.");
    }

    puts("\n=== TRAP 4: float -> int truncates toward zero (it does not round) ===");
    {
        printf("  (int) 2.99 = %d      (int) -2.99 = %d\n", (int)2.99, (int)-2.99);
        printf("  lround(2.5) style rounding needs <math.h>, not a cast.\n");
        puts("  Converting a float whose value does not fit in the int type is UB.");
        printf("  (int)1e20 -> undefined, not INT_MAX.\n");
    }

    puts("\n=== TRAP 5: integer promotion makes small types behave like int ===");
    {
        unsigned char a = 200, b = 200;
        /* Both promote to int, so a+b is 400 as an int — no wraparound here. */
        printf("  (unsigned char)200 + (unsigned char)200 = %d as int\n", a + b);
        printf("  ...but stored back into an unsigned char: %u\n",
               (unsigned)(unsigned char)(a + b));

        /* The famous one: ~ on a small unsigned type. */
        unsigned char x = 0;
        printf("  ~(unsigned char)0 as int = %d   <- promoted to int FIRST, so -1\n", ~x);
        printf("  (unsigned char)~x        = %u   <- what you probably meant\n",
               (unsigned)(unsigned char)~x);
    }

    puts("\n=== TRAP 6: char signedness is implementation-defined ===");
    {
        char c = (char)0xFF;
        printf("  (char)0xFF prints as %d on this compiler\n", c);
        puts("  On a signed-char platform that is -1; on unsigned-char it is 255.");
        puts("  This is why <ctype.h> functions REQUIRE an unsigned char argument:");
        puts("      isalpha(c)                 <- UB if char is signed and c is negative");
        puts("      isalpha((unsigned char)c)  <- correct");
    }

    puts("\n=== TRAP 7: mixing types in arithmetic promotes the whole expression ===");
    {
        int   a = 7, b = 2;
        float f = 2.0f;
        printf("  a / b         = %d      (int / int)\n", a / b);
        printf("  a / f         = %g      (int promoted to float)\n", a / f);
        printf("  a / (double)b = %g\n", a / (double)b);
        printf("  1 / 2 * 3.0   = %g      <- 1/2 is computed as INT first = 0\n", 1 / 2 * 3.0);
        printf("  1 * 3.0 / 2   = %g      <- reorder, or write 1.0\n", 1 * 3.0 / 2);
    }

    puts("\n=== TRAP 8: size_t arithmetic going negative ===");
    {
        size_t n = 3;
        size_t i = 5;
        printf("  n - i where both are size_t = %zu\n", n - i);
        puts("  Never subtract size_t values unless you have proved a >= b.");
        printf("  Signed alternative: (ptrdiff_t)n - (ptrdiff_t)i = %td\n",
               (ptrdiff_t)n - (ptrdiff_t)i);
    }

    puts("\n=== HOW TO DEFEND YOURSELF ===");
    puts("  1. Compile with -Wall -Wextra -Wsign-compare -Wconversion.");
    puts("  2. Pick ONE signedness for a given quantity and stick to it.");
    puts("     Sizes and indices: size_t.  Deltas and counts that can go");
    puts("     negative: ptrdiff_t or int.");
    puts("  3. Make every narrowing conversion an EXPLICIT cast, so a reader");
    puts("     can see you meant it.");
    puts("  4. Hoist loop bounds into a variable instead of recomputing them.");
    puts("  5. Run with -fsanitize=undefined during development.");

    return 0;
}
