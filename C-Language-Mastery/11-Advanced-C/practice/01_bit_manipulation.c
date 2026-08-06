/* 01_bit_manipulation.c — every bit idiom worth knowing, and why it works.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 01_bit_manipulation.c -o t && ./t
 *
 * RULE ZERO: use UNSIGNED types. `1 << 31` on a signed 32-bit int is
 * undefined behaviour; `1u << 31` is fine. Right-shifting a negative signed
 * value is implementation-defined. Every idiom here uses unsigned.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

static void print_bits32(const char *label, uint32_t v)
{
    printf("  %-28s ", label);
    for (int i = 31; i >= 0; i--) {
        putchar((v >> i) & 1 ? '1' : '0');
        if (i % 8 == 0 && i) putchar('_');
    }
    printf("  = %u\n", v);
}

/* ================================================================= *
 * POPCOUNT — count the set bits, four ways
 * ================================================================= */

/* O(bits): test every position. */
static int popcount_naive(uint32_t x)
{
    int n = 0;
    for (int i = 0; i < 32; i++) if (x & (1u << i)) n++;
    return n;
}

/* O(set bits): x & (x-1) CLEARS the lowest set bit.
 *
 * WHY: x-1 flips the lowest set bit to 0 and every bit below it to 1.
 * ANDing keeps only the bits above, so the lowest 1 disappears.
 *     x     = 0101_1000
 *     x - 1 = 0101_0111
 *     AND   = 0101_0000     <- the lowest set bit is gone
 * This is Kernighan's algorithm, and it is the single most useful bit trick.
 */
static int popcount_kernighan(uint32_t x)
{
    int n = 0;
    while (x) { x &= x - 1; n++; }
    return n;
}

/* O(log bits): parallel pairwise addition. Each step adds adjacent groups.
 * This is what a CPU without a POPCNT instruction actually compiles to. */
static int popcount_parallel(uint32_t x)
{
    x = x - ((x >> 1) & 0x55555555u);                    /* sums of 2-bit groups */
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);    /* 4-bit groups */
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;                    /* 8-bit groups */
    return (int)((x * 0x01010101u) >> 24);               /* sum the four bytes */
}

/* One instruction on any modern CPU. C23 standardises this as stdc_count_ones. */
static int popcount_builtin(uint32_t x) { return __builtin_popcount(x); }

/* ================================================================= *
 * A BITSET — 64 flags per uint64_t, 8 bits per byte instead of 8 bytes
 * per bool. A million flags costs 128 KB instead of 1 MB.
 * ================================================================= */
typedef struct { uint64_t *words; size_t n_bits, n_words; } BitSet;

static bool bitset_init(BitSet *b, size_t n_bits)
{
    b->n_words = (n_bits + 63) / 64;                     /* round UP */
    b->words = calloc(b->n_words, sizeof *b->words);
    if (b->words == NULL) return false;
    b->n_bits = n_bits;
    return true;
}
static void bitset_free(BitSet *b) { free(b->words); memset(b, 0, sizeof *b); }

/* i / 64 == i >> 6 and i % 64 == i & 63. The compiler does this for you
 * with unsigned types; it is written out here to show the mechanism. */
static void bitset_set  (BitSet *b, size_t i) { b->words[i >> 6] |=  (1ULL << (i & 63)); }
static void bitset_clear(BitSet *b, size_t i) { b->words[i >> 6] &= ~(1ULL << (i & 63)); }
static void bitset_flip (BitSet *b, size_t i) { b->words[i >> 6] ^=  (1ULL << (i & 63)); }
static bool bitset_test (const BitSet *b, size_t i) { return (b->words[i >> 6] >> (i & 63)) & 1; }

static size_t bitset_count(const BitSet *b)
{
    size_t n = 0;
    for (size_t w = 0; w < b->n_words; w++) n += (size_t)__builtin_popcountll(b->words[w]);
    return n;
}

/* The Sieve of Eratosthenes over a bitset — the classic use. */
static size_t sieve(size_t limit, BitSet *composite)
{
    size_t count = 0;
    for (size_t i = 2; i <= limit; i++) {
        if (bitset_test(composite, i)) continue;
        count++;
        for (size_t j = i * i; j <= limit; j += i) bitset_set(composite, j);
    }
    return count;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE FOUR BASIC OPERATIONS ===");
    {
        uint32_t x = 0;
        print_bits32("start", x);
        x |= (1u << 3);   print_bits32("x |= (1u << 3)   set bit 3", x);
        x |= (1u << 7);   print_bits32("x |= (1u << 7)   set bit 7", x);
        printf("  %-28s %s\n", "x & (1u << 3)    test bit 3", (x & (1u << 3)) ? "SET" : "clear");
        printf("  %-28s %s\n", "x & (1u << 5)    test bit 5", (x & (1u << 5)) ? "SET" : "clear");
        x ^= (1u << 3);   print_bits32("x ^= (1u << 3)   toggle bit 3", x);
        x &= ~(1u << 7);  print_bits32("x &= ~(1u << 7)  clear bit 7", x);
        puts("");
        puts("  SET    x |=  (1u << n)");
        puts("  CLEAR  x &= ~(1u << n)");
        puts("  TOGGLE x ^=  (1u << n)");
        puts("  TEST   x &   (1u << n)");
        puts("");
        puts("  Note the `u` suffix everywhere. `1 << 31` on a signed 32-bit int");
        puts("  shifts into the sign bit, which is UNDEFINED BEHAVIOUR.");
    }

    puts("\n=== THE IDIOMS WORTH MEMORISING ===");
    {
        /* 0b... binary literals are a GCC extension (standard only in C23),
         * so this uses hex. 0x58 == 0b0101_1000. */
        uint32_t x = 0x58;

        print_bits32("x", x);
        print_bits32("x & (x - 1)   clear lowest set", x & (x - 1));
        print_bits32("x & -x        ISOLATE lowest set", x & (uint32_t)(-(int32_t)x));
        print_bits32("x | (x - 1)   set all below lowest", x | (x - 1));
        print_bits32("x | (x + 1)   set lowest CLEAR bit", x | (x + 1));
        print_bits32("~x            invert all", ~x);

        puts("\n  WHY x & (x-1) CLEARS THE LOWEST SET BIT:");
        puts("      x     = 0101_1000");
        puts("      x - 1 = 0101_0111    borrowing flips the lowest 1 to 0");
        puts("                           and every bit below it to 1");
        puts("      AND   = 0101_0000    only the bits ABOVE survive");
        puts("");
        puts("  WHY x & -x ISOLATES IT: -x is ~x + 1 (two's complement), which");
        puts("  is x with every bit above the lowest 1 inverted. ANDing leaves");
        puts("  exactly that one bit.");

        puts("\n  POWER OF TWO TEST:");
        for (uint32_t v = 0; v <= 9; v++)
            printf("    %u: %s\n", v, (v != 0 && (v & (v - 1)) == 0) ? "power of 2" : "no");
        printf("    1024: %s\n", (1024 & 1023) == 0 ? "power of 2" : "no");
        puts("    A power of two has exactly ONE set bit, so clearing it gives 0.");
        puts("    The `v != 0` guard matters: 0 & -1 == 0 too.");
    }

    puts("\n=== POPCOUNT: FOUR IMPLEMENTATIONS ===");
    {
        uint32_t tests[] = {0, 1, 7, 255, 0xFFFFFFFFu, 0xDEADBEEFu, 0x80000000u};
        printf("  %-12s %6s %6s %6s %6s\n", "value", "naive", "kernig", "parall", "builtin");
        for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++) {
            int a = popcount_naive(tests[i]), b = popcount_kernighan(tests[i]);
            int c = popcount_parallel(tests[i]), d = popcount_builtin(tests[i]);
            printf("  0x%08X %6d %6d %6d %6d  %s\n", tests[i], a, b, c, d,
                   (a == b && b == c && c == d) ? "" : "*** DISAGREE ***");
        }

        const int N = 5000000;
        clock_t t;
        volatile int sink = 0;

        t = clock(); for (int i = 0; i < N; i++) sink += popcount_naive((uint32_t)i);
        double t1 = seconds_since(t);
        t = clock(); for (int i = 0; i < N; i++) sink += popcount_kernighan((uint32_t)i);
        double t2 = seconds_since(t);
        t = clock(); for (int i = 0; i < N; i++) sink += popcount_parallel((uint32_t)i);
        double t3 = seconds_since(t);
        t = clock(); for (int i = 0; i < N; i++) sink += popcount_builtin((uint32_t)i);
        double t4 = seconds_since(t);

        printf("\n  %d calls:\n", N);
        printf("    naive      %.4f s   O(32), always 32 iterations\n", t1);
        printf("    Kernighan  %.4f s   O(set bits) — fast for SPARSE values\n", t2);
        printf("    parallel   %.4f s   O(log 32) = 5 steps, branch-free\n", t3);
        printf("    builtin    %.4f s   ONE instruction (POPCNT) on modern x86\n", t4);
        puts("  __builtin_popcount is the answer. C23 standardises these as");
        puts("  <stdbit.h>: stdc_count_ones, stdc_leading_zeros, and friends.");
    }

    puts("\n=== COUNTING ZEROS: clz AND ctz ===");
    {
        uint32_t vals[] = {1, 8, 1024, 0x80000000u, 0xF0F0u};
        printf("  %-12s %5s %5s  %s\n", "value", "clz", "ctz", "meaning");
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++) {
            int clz = __builtin_clz(vals[i]);      /* leading zeros  */
            int ctz = __builtin_ctz(vals[i]);      /* trailing zeros */
            printf("  0x%08X %5d %5d  floor(log2) = %d, lowest set bit at %d\n",
                   vals[i], clz, ctz, 31 - clz, ctz);
        }
        puts("  __builtin_clz(0) and __builtin_ctz(0) are UNDEFINED — guard them.");
        puts("");
        puts("  What they are FOR:");
        puts("    31 - clz(x)        floor(log2(x)) in one instruction");
        puts("    1u << (32-clz(x))  round UP to the next power of two");
        puts("    ctz(x)             the index of the lowest set bit — how a");
        puts("                       bitset iterates only its SET bits");
    }

    puts("\n=== A BITSET: 64 FLAGS PER WORD ===");
    {
        BitSet b;
        if (!bitset_init(&b, 200)) return 1;

        for (size_t i = 0; i < 200; i += 7) bitset_set(&b, i);
        printf("  set every 7th bit of 200: %zu bits set\n", bitset_count(&b));
        printf("  bit 0=%d bit 7=%d bit 8=%d bit 196=%d\n",
               bitset_test(&b, 0), bitset_test(&b, 7),
               bitset_test(&b, 8), bitset_test(&b, 196));

        bitset_clear(&b, 7);
        bitset_flip(&b, 8);
        printf("  after clear(7) and flip(8): bit7=%d bit8=%d, count %zu\n",
               bitset_test(&b, 7), bitset_test(&b, 8), bitset_count(&b));

        printf("\n  memory: %zu bits in %zu words = %zu bytes\n",
               b.n_bits, b.n_words, b.n_words * sizeof(uint64_t));
        printf("          an array of %zu bools would be %zu bytes (%.0fx more)\n",
               b.n_bits, b.n_bits, (double)b.n_bits / (double)(b.n_words * 8));
        bitset_free(&b);
    }

    puts("\n=== SIEVE OF ERATOSTHENES OVER A BITSET ===");
    {
        const size_t LIMIT = 10000000;
        BitSet composite;
        if (bitset_init(&composite, LIMIT + 1)) {
            clock_t t = clock();
            size_t primes = sieve(LIMIT, &composite);
            double elapsed = seconds_since(t);

            printf("  primes below %zu: %zu, found in %.4f s\n", LIMIT, primes, elapsed);
            printf("  memory used: %zu KB (a bool array would need %zu KB)\n",
                   composite.n_words * 8 / 1024, LIMIT / 1024);
            printf("  first primes: ");
            size_t shown = 0;
            for (size_t i = 2; i <= 60 && shown < 15; i++)
                if (!bitset_test(&composite, i)) { printf("%zu ", i); shown++; }
            puts("");
            bitset_free(&composite);
        }
        puts("  Eight times less memory means eight times fewer cache misses —");
        puts("  which is why the bitset version is also FASTER, not just smaller.");
    }

    puts("\n=== BIT FLAGS: THE PERMISSIONS PATTERN ===");
    {
        enum {
            PERM_READ    = 1u << 0,      /* 0b0001 */
            PERM_WRITE   = 1u << 1,      /* 0b0010 */
            PERM_EXECUTE = 1u << 2,      /* 0b0100 */
            PERM_DELETE  = 1u << 3,      /* 0b1000 */
            PERM_ALL     = PERM_READ | PERM_WRITE | PERM_EXECUTE | PERM_DELETE,
        };

        unsigned perms = PERM_READ | PERM_WRITE;
        printf("  perms = READ | WRITE = 0x%X\n", perms);
        printf("    can read?    %s\n", (perms & PERM_READ)    ? "yes" : "no");
        printf("    can execute? %s\n", (perms & PERM_EXECUTE) ? "yes" : "no");

        perms |= PERM_EXECUTE;
        printf("  after |= EXECUTE: 0x%X, can execute? %s\n",
               perms, (perms & PERM_EXECUTE) ? "yes" : "no");
        perms &= ~PERM_WRITE;
        printf("  after &= ~WRITE : 0x%X, can write?   %s\n",
               perms, (perms & PERM_WRITE) ? "yes" : "no");

        printf("  has ALL permissions? %s\n",
               (perms & PERM_ALL) == PERM_ALL ? "yes" : "no");
        printf("  has ANY of WRITE|DELETE? %s\n",
               (perms & (PERM_WRITE | PERM_DELETE)) ? "yes" : "no");
        puts("");
        puts("  Each flag is a DISTINCT POWER OF TWO, so they never collide.");
        puts("  This is exactly how open()'s O_RDONLY|O_CREAT|O_APPEND works,");
        puts("  and every graphics and event API you will ever use.");
        puts("");
        puts("  Testing for ALL of a set: (x & MASK) == MASK");
        puts("  Testing for ANY of a set: (x & MASK) != 0");
        puts("  Those two are easy to confuse and the bug is silent.");
    }

    puts("\n=== SWAPPING AND OTHER TRICKS ===");
    {
        unsigned a = 5, b = 9;
        printf("  a=%u b=%u\n", a, b);
        a ^= b; b ^= a; a ^= b;                  /* XOR swap */
        printf("  after XOR swap: a=%u b=%u\n", a, b);
        puts("    XOR swap uses no temporary — and it is a BAD IDEA: it breaks");
        puts("    if a and b are the SAME OBJECT (x^=x zeroes it), and it is");
        puts("    SLOWER than a temporary on any modern CPU because the three");
        puts("    operations are serially dependent. Use a temporary.");

        int32_t v = -42;
        printf("\n  abs without a branch: %d -> %d\n", v, (v ^ (v >> 31)) - (v >> 31));
        puts("    v >> 31 is all-ones for negatives, all-zeros for positives");
        puts("    (arithmetic shift). XOR then subtract gives two's complement");
        puts("    negation for negatives and a no-op for positives.");
        puts("    Branch-free code matters when the branch is UNPREDICTABLE —");
        puts("    a mispredict costs 15-20 cycles. If it predicts well, the");
        puts("    ordinary `if` is faster. MEASURE before using tricks like this.");

        uint32_t n = 1000;
        uint32_t rounded = n;
        rounded--;
        rounded |= rounded >> 1;  rounded |= rounded >> 2;
        rounded |= rounded >> 4;  rounded |= rounded >> 8;
        rounded |= rounded >> 16;
        rounded++;
        printf("\n  round %u up to a power of two: %u\n", n, rounded);
        puts("    Smear the highest set bit down over every lower position, then");
        puts("    add 1. Used by every hash table to size its bucket array (so");
        puts("    that `hash & (n-1)` can replace `hash %% n`).");
        printf("    with clz: %u\n", 1u << (32 - __builtin_clz(n - 1)));

        printf("\n  byte swap (endianness): 0x%08X -> 0x%08X\n",
               0x12345678u, __builtin_bswap32(0x12345678u));
        puts("    One instruction (BSWAP). This is what htonl/ntohl compile to.");
    }

    puts("\n=== SUBSETS VIA BITMASK ===");
    {
        const char *items[] = {"a", "b", "c"};
        int n = 3;
        printf("  all %d subsets of {a,b,c}, by counting 0..%d:\n", 1 << n, (1 << n) - 1);
        for (unsigned mask = 0; mask < (1u << n); mask++) {
            printf("    %u%u%u -> { ", (mask>>2)&1, (mask>>1)&1, mask&1);
            for (int i = 0; i < n; i++) if (mask & (1u << i)) printf("%s ", items[i]);
            puts("}");
        }
        puts("  Each BIT is an include/exclude decision, so counting from 0 to");
        puts("  2^n - 1 enumerates every subset. Simpler and faster than the");
        puts("  recursive version in module 10 — and it gives each subset an");
        puts("  INDEX, which is what makes bitmask DP possible.");
    }

    puts("\n=== THE RULES ===");
    puts("  1. UNSIGNED types for all bit work. Always.");
    puts("  2. Parenthesise: `&` and `|` bind LOOSER than `==`, so");
    puts("     `x & MASK == 0` parses as `x & (MASK == 0)`.");
    puts("  3. `x << n` with n >= the type's width is UNDEFINED.");
    puts("  4. Use __builtin_popcount / clz / ctz instead of hand-rolling.");
    puts("  5. Do NOT replace `x / 2` with `x >> 1` for readability reasons —");
    puts("     the compiler already does it, and for SIGNED values they differ");
    puts("     (-7/2 is -3, but -7>>1 is -4).");
    puts("  6. Bitfields (module 06) are more readable for in-memory flags, but");
    puts("     their bit ORDER is implementation-defined — for wire formats and");
    puts("     hardware registers, use explicit shifts and masks like these.");

    return 0;
}
