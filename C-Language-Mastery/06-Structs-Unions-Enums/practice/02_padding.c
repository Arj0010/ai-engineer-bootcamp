/* 02_padding.c — why sizeof(struct) is bigger than the sum of its members.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_padding.c -o t && ./t
 *
 * This is the single highest-leverage micro-optimisation in C: reordering
 * struct members costs nothing and routinely shrinks data by 30-50%.
 */
#include <stdio.h>
#include <stddef.h>     /* offsetof */
#include <stdint.h>
#include <string.h>
#include <stdalign.h>

/* Members in a careless order. */
struct Bad {
    char   a;        /* offset 0                    */
                     /* 3 bytes PADDING             */
    int    b;        /* offset 4  (needs 4-alignment) */
    char   c;        /* offset 8                    */
                     /* 3 bytes TAIL PADDING        */
};                   /* total 12 bytes for 6 bytes of data */

/* Same members, decreasing size. */
struct Good {
    int    b;        /* offset 0 */
    char   a;        /* offset 4 */
    char   c;        /* offset 5 */
                     /* 2 bytes tail padding */
};                   /* total 8 bytes */

/* A realistic example: a record type that most people would write like this. */
struct RecordBad {
    char     flag;        /*  1 */
    double   value;       /*  8, needs 8-alignment -> 7 bytes of padding before it */
    char     code;        /*  1 */
    int32_t  id;          /*  4, needs 4-alignment -> 3 bytes of padding before it */
    char     initial;     /*  1 */
    void    *ptr;         /*  8, needs 8-alignment -> 7 bytes of padding before it */
};

struct RecordGood {
    double   value;       /*  8 */
    void    *ptr;         /*  8 */
    int32_t  id;          /*  4 */
    char     flag;        /*  1 */
    char     code;        /*  1 */
    char     initial;     /*  1 */
                          /*  1 byte tail padding */
};

/* Packed: all padding removed. ONLY for matching an external binary layout. */
struct __attribute__((packed)) Packed {
    char    a;
    int     b;
    char    c;
};

/* Over-aligned: force this onto its own cache line. Used to stop two threads'
 * variables sharing a line (FALSE SHARING — see module 12). */
struct CacheLineAligned {
    /* alignas goes on the MEMBER (a declaration specifier), not on the struct
     * tag. Raising a member's alignment raises the whole struct's. */
    alignas(64) long counter;
};

static void dump_layout(const char *name, size_t total, size_t data_bytes,
                        const char *members, const size_t *offsets,
                        const size_t *sizes, size_t n)
{
    printf("\n  %s — sizeof = %zu bytes (%zu bytes of real data, %zu padding)\n",
           name, total, data_bytes, total - data_bytes);
    printf("    ");
    size_t cursor = 0;
    for (size_t i = 0; i < n; i++) {
        while (cursor < offsets[i]) { putchar('.'); cursor++; }   /* padding */
        for (size_t k = 0; k < sizes[i]; k++) { putchar(members[i]); cursor++; }
    }
    while (cursor < total) { putchar('.'); cursor++; }            /* tail padding */
    printf("\n    (letters = members, dots = PADDING)\n");
}

int main(void)
{
    puts("=== ALIGNMENT: every type must sit on a boundary ===");
    printf("  char     size %zu  align %zu\n", sizeof(char),   alignof(char));
    printf("  short    size %zu  align %zu\n", sizeof(short),  alignof(short));
    printf("  int      size %zu  align %zu\n", sizeof(int),    alignof(int));
    printf("  long     size %zu  align %zu\n", sizeof(long),   alignof(long));
    printf("  double   size %zu  align %zu\n", sizeof(double), alignof(double));
    printf("  void *   size %zu  align %zu\n", sizeof(void *), alignof(void *));
    printf("  max_align_t          align %zu  <- the strictest any type needs\n",
           alignof(max_align_t));
    puts("  An int's ADDRESS must be a multiple of 4, a double's a multiple of 8.");
    puts("  On x86 a misaligned access is merely slow; on some ARM and SPARC");
    puts("  configurations it is a hard fault. The compiler guarantees alignment");
    puts("  by inserting PADDING.");

    puts("\n=== THE SAME THREE MEMBERS, TWO ORDERS ===");
    {
        size_t bad_off[]  = {offsetof(struct Bad, a),  offsetof(struct Bad, b),  offsetof(struct Bad, c)};
        size_t bad_sz[]   = {sizeof(char), sizeof(int), sizeof(char)};
        size_t good_off[] = {offsetof(struct Good, b), offsetof(struct Good, a), offsetof(struct Good, c)};
        size_t good_sz[]  = {sizeof(int), sizeof(char), sizeof(char)};

        dump_layout("struct Bad  { char a; int b; char c; }",
                    sizeof(struct Bad), 6, "abc", bad_off, bad_sz, 3);
        printf("    offsets: a=%zu b=%zu c=%zu\n",
               offsetof(struct Bad, a), offsetof(struct Bad, b), offsetof(struct Bad, c));
        puts("    a at 0, then 3 bytes of padding so b lands on a multiple of 4,");
        puts("    then c at 8, then 3 bytes of TAIL padding so that in an array");
        puts("    the NEXT element's b is also 4-aligned.");

        dump_layout("struct Good { int b; char a; char c; }",
                    sizeof(struct Good), 6, "bac", good_off, good_sz, 3);
        printf("    offsets: b=%zu a=%zu c=%zu\n",
               offsetof(struct Good, b), offsetof(struct Good, a), offsetof(struct Good, c));

        printf("\n  Same data, %zu bytes vs %zu bytes — a %.0f%% saving from",
               sizeof(struct Bad), sizeof(struct Good),
               100.0 * (1.0 - (double)sizeof(struct Good) / (double)sizeof(struct Bad)));
        puts(" reordering alone.");
    }

    puts("\n=== A REALISTIC RECORD ===");
    {
        printf("  struct RecordBad  = %zu bytes\n", sizeof(struct RecordBad));
        printf("    flag    at %2zu\n", offsetof(struct RecordBad, flag));
        printf("    value   at %2zu   <- 7 bytes of padding before it\n", offsetof(struct RecordBad, value));
        printf("    code    at %2zu\n", offsetof(struct RecordBad, code));
        printf("    id      at %2zu   <- 3 bytes of padding before it\n", offsetof(struct RecordBad, id));
        printf("    initial at %2zu\n", offsetof(struct RecordBad, initial));
        printf("    ptr     at %2zu   <- 7 bytes of padding before it\n", offsetof(struct RecordBad, ptr));

        printf("\n  struct RecordGood = %zu bytes  (members in decreasing size)\n",
               sizeof(struct RecordGood));
        printf("    value=%zu ptr=%zu id=%zu flag=%zu code=%zu initial=%zu\n",
               offsetof(struct RecordGood, value), offsetof(struct RecordGood, ptr),
               offsetof(struct RecordGood, id), offsetof(struct RecordGood, flag),
               offsetof(struct RecordGood, code), offsetof(struct RecordGood, initial));

        size_t saved = sizeof(struct RecordBad) - sizeof(struct RecordGood);
        printf("\n  %zu bytes saved PER RECORD. Over 10 million records that is %zu MB,\n",
               saved, saved * 10000000 / (1024 * 1024));
        puts("  and far more importantly it is that much less cache pressure.");
        printf("  Records per 64-byte cache line: %zu (bad) vs %zu (good)\n",
               64 / sizeof(struct RecordBad), 64 / sizeof(struct RecordGood));
        puts("  THE RULE: declare members in DECREASING order of size.");
        puts("  Compile with -Wpadded to have GCC point out every gap.");
    }

    puts("\n=== PACKED STRUCTS: only for external formats ===");
    {
        printf("  struct Bad (normal) = %zu bytes\n", sizeof(struct Bad));
        printf("  struct Packed       = %zu bytes  (all padding removed)\n",
               sizeof(struct Packed));
        printf("  Packed offsets: a=%zu b=%zu c=%zu  <- b is NOT 4-aligned\n",
               offsetof(struct Packed, a), offsetof(struct Packed, b),
               offsetof(struct Packed, c));
        puts("  Use ONLY when you must match a byte-exact external layout:");
        puts("  a file header, a network packet, a hardware register block.");
        puts("  Costs: every member access may become a slow unaligned load, and");
        puts("  TAKING THE ADDRESS of a packed member yields a misaligned pointer,");
        puts("  which is undefined behaviour to dereference normally.");
        puts("  The safer alternative for wire formats: read bytes explicitly and");
        puts("  assemble the fields with shifts (module 11). It is portable, it");
        puts("  handles endianness, and it never traps.");
    }

    puts("\n=== OVER-ALIGNMENT: alignas ===");
    {
        printf("  struct CacheLineAligned: size %zu, align %zu\n",
               sizeof(struct CacheLineAligned), alignof(struct CacheLineAligned));
        puts("  Forcing 64-byte alignment puts this object on its own cache line.");
        puts("  That prevents FALSE SHARING: two threads writing to different");
        puts("  variables that happen to share a line, each invalidating the");
        puts("  other's copy. It can cost 10x in throughput. See module 12.");
    }

    puts("\n=== WHY YOU MUST NEVER memcmp TWO STRUCTS ===");
    {
        struct Bad x, y;
        memset(&x, 0xAA, sizeof x);        /* fill EVERYTHING, padding included */
        memset(&y, 0x55, sizeof y);        /* fill with something different     */
        x.a = y.a = 1;                     /* now make the real MEMBERS equal   */
        x.b = y.b = 2;
        x.c = y.c = 3;

        printf("  members are identical: a=%d b=%d c=%d in both\n", x.a, x.b, x.c);
        printf("  memcmp(&x, &y, sizeof x) == 0 ? %s   <- WRONG ANSWER\n",
               memcmp(&x, &y, sizeof x) == 0 ? "yes" : "NO");
        printf("  field-by-field comparison: %s   <- correct\n",
               (x.a == y.a && x.b == y.b && x.c == y.c) ? "equal" : "different");
        puts("  The padding bytes differ, and memcmp compares them. Padding is");
        puts("  never initialised by the compiler and holds whatever was there.");
        puts("  ALWAYS compare structs field by field. Write an equals() function.");
        puts("");
        puts("  The same trap applies to:");
        puts("    - using a struct as a hash-table key by hashing its bytes");
        puts("    - fwrite'ing a struct and expecting reproducible output");
        puts("    - memcmp'ing for change detection");
        puts("  (memset(&s, 0, sizeof s) DOES zero the padding, which is why");
        puts("   zeroing first makes some of these usable — but do not rely on it,");
        puts("   because a later member assignment may leave padding indeterminate.)");
    }

    puts("\n=== ARRAYS OF STRUCTS: where tail padding earns its keep ===");
    {
        struct Bad arr[4];
        printf("  sizeof(struct Bad)     = %zu\n", sizeof(struct Bad));
        printf("  sizeof(struct Bad[4])  = %zu (exactly 4x — no gaps BETWEEN elements)\n",
               sizeof arr);
        printf("  &arr[1] - &arr[0]      = %td bytes\n",
               (char *)&arr[1] - (char *)&arr[0]);
        puts("  Tail padding is what guarantees this: each element starts at a");
        puts("  multiple of the struct's alignment, so arr[i].b is always aligned.");
        puts("  Without it, arr[1].b would land on an odd boundary.");
    }

    puts("\n=== HOW THE COMPILER DECIDES ===");
    puts("  1. Each member is placed at the next offset that satisfies ITS alignment.");
    puts("  2. Padding is inserted before a member if the cursor is misaligned.");
    puts("  3. The struct's own alignment = the LARGEST alignment among its members.");
    puts("  4. The struct's SIZE is rounded up to a multiple of that alignment");
    puts("     (tail padding), so arrays work.");
    puts("");
    puts("  A C compiler may NOT reorder your members — the standard guarantees");
    puts("  declaration order. (Rust and Go do reorder, which is why they have no");
    puts("  equivalent of this problem.) In C, the ordering is YOUR job.");

    return 0;
}
