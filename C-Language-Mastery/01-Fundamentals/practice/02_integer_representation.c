/* 02_integer_representation.c — two's complement, bit patterns, overflow.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_integer_representation.c -o t && ./t
 *
 * Then run it again with UBSan and watch the signed overflow get caught:
 *   gcc -g -O0 -fsanitize=undefined 02_integer_representation.c -o t && ./t
 */
#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Print any object's raw bytes, most-significant bit first, grouped by byte. */
static void print_bits(const char *label, const void *obj, size_t nbytes)
{
    const unsigned char *p = obj;
    printf("  %-16s ", label);
    /* Walk bytes from high address to low so little-endian machines read
     * "naturally" as a number. (Endianness itself is demonstrated below.) */
    for (size_t i = nbytes; i-- > 0; ) {
        for (int b = 7; b >= 0; b--)
            putchar((p[i] >> b) & 1 ? '1' : '0');
        putchar(' ');
    }
    putchar('\n');
}

int main(void)
{
    puts("=== two's complement: negate = flip all bits, then add 1 ===");
    int8_t a = 5, b = -5;
    print_bits("(int8_t)  5", &a, sizeof a);
    print_bits("(int8_t) -5", &b, sizeof b);
    printf("  ~5 = %d, ~5 + 1 = %d  -> that IS -5\n\n", (int8_t)~a, (int8_t)(~a + 1));

    puts("=== the range is ASYMMETRIC: one more negative than positive ===");
    printf("  INT8_MIN  = %d   INT8_MAX  = %d\n", INT8_MIN, INT8_MAX);
    printf("  INT_MIN   = %d   INT_MAX   = %d\n", INT_MIN, INT_MAX);
    puts("  There is no +128 in an int8_t, so -INT8_MIN cannot be represented.");
    puts("  -INT_MIN is UNDEFINED BEHAVIOUR. abs(INT_MIN) is too.\n");

    puts("=== UNSIGNED overflow is DEFINED: it wraps (mod 2^n) ===");
    unsigned int u = UINT_MAX;
    printf("  UINT_MAX     = %u\n", u);
    printf("  UINT_MAX + 1 = %u   <- wraps to 0, guaranteed by the standard\n", u + 1u);
    unsigned char uc = 250;
    printf("  (unsigned char)250 + 10 = %u\n", (unsigned)(unsigned char)(uc + 10));
    puts("");

    puts("=== SIGNED overflow is UNDEFINED: the compiler may assume it never happens ===");
    puts("  int x = INT_MAX; x + 1;   <-- UB. Do not do this.");
    puts("  The compiler optimises `if (x + 1 < x)` to `if (false)` because");
    puts("  it is entitled to assume overflow cannot occur.");
    puts("  Correct check for a + b overflowing:");
    {
        int x = INT_MAX - 3, y = 10;
        if (y > 0 && x > INT_MAX - y)
            printf("    %d + %d WOULD overflow -> refused\n", x, y);
        /* Or, with GCC/Clang, let the hardware flag tell you: */
        int result;
        if (__builtin_add_overflow(x, y, &result))
            printf("    __builtin_add_overflow agrees: overflow\n");
    }
    puts("");

    puts("=== casting signed <-> unsigned keeps the BITS, changes the MEANING ===");
    {
        int32_t  s = -1;
        uint32_t v;
        memcpy(&v, &s, sizeof v);   /* the portable way to reinterpret bits */
        printf("  (int32_t) -1  as bits -> as uint32_t = %u\n", v);
        print_bits("bits", &s, sizeof s);
        puts("  All ones. That is why (unsigned)-1 is the maximum unsigned value,");
        puts("  and why a stray -1 in a size_t becomes 18446744073709551615.\n");
    }

    puts("=== the classic bug ===");
    {
        int      i = -1;
        unsigned n = 1;
        /* i is converted to unsigned before the comparison (usual arithmetic
         * conversions), so -1 becomes UINT_MAX. */
        printf("  int -1 < unsigned 1 ?  %s   <-- surprising\n",
               ((unsigned)i < n) ? "true" : "FALSE");
        printf("  with an explicit cast: %s   <-- what you meant\n",
               (i < (int)n) ? "true" : "false");
    }
    puts("");

    puts("=== shifts ===");
    {
        unsigned v = 1;
        printf("  1u << 0..7 : ");
        for (int i = 0; i < 8; i++) printf("%u ", v << i);
        puts("");
        printf("  256 >> 4   : %u   (unsigned right shift fills with 0)\n", 256u >> 4);
        printf("  -8 >> 1    : %d   (signed: implementation-defined; in practice\n", -8 >> 1);
        puts("                     an arithmetic shift that keeps the sign)");
        puts("  x << n where n >= width of x, or n < 0, is UNDEFINED.");
        puts("  1 << 31 on a 32-bit signed int is UB. Write 1u << 31.");
    }
    puts("");

    puts("=== endianness: how multi-byte values sit in memory ===");
    {
        uint32_t val = 0x01020304u;
        unsigned char *p = (unsigned char *)&val;
        printf("  0x01020304 stored as bytes: %02X %02X %02X %02X -> %s-endian\n",
               p[0], p[1], p[2], p[3],
               (p[0] == 0x04) ? "LITTLE" : "BIG");
        puts("  x86 and ARM (usually) are little-endian: least significant byte first.");
        puts("  Network byte order is BIG-endian, hence htonl()/ntohl().");
    }
    puts("");

    puts("=== integer promotion: char arithmetic happens in int ===");
    {
        char c1 = 100, c2 = 100;
        int  as_int = c1 + c2;              /* promoted to int: 200 */
        char truncated = (char)(c1 + c2);   /* squeezed back into a char */
        printf("  100 + 100 as int  = %d\n", as_int);
        printf("  100 + 100 as char = %d   <- truncation, not overflow of the add\n",
               truncated);
    }

    return 0;
}
