/* 03_const_and_void.c — const placement, and void* as C's generic type.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_const_and_void.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* const on a parameter is a compiler-checked promise to the caller:
 * "I will not write through this pointer." Use it on every input. */
static long long sum(const int *a, size_t n)
{
    long long total = 0;
    for (size_t i = 0; i < n; i++) total += a[i];
    /* a[0] = 5;   <-- would not compile. That is the point. */
    return total;
}

/* ================================================================= *
 * Generic operations over void*, the way the standard library does it.
 * ================================================================= */

/* Swap any two objects of the same size. Bytes are bytes. */
static void generic_swap(void *a, void *b, size_t size)
{
    unsigned char *pa = a, *pb = b;
    for (size_t i = 0; i < size; i++) {
        unsigned char t = pa[i]; pa[i] = pb[i]; pb[i] = t;
    }
}

/* Linear search over an array of anything, using a caller-supplied comparator.
 * This signature is deliberately the same shape as bsearch(). */
static void *generic_find(const void *base, size_t n, size_t size,
                          const void *key,
                          int (*cmp)(const void *, const void *))
{
    const char *p = base;                    /* char* so arithmetic is in bytes */
    for (size_t i = 0; i < n; i++, p += size)
        if (cmp(p, key) == 0)
            return (void *)p;                /* found */
    return NULL;
}

/* Apply a function to every element — C's version of map/foreach. */
static void generic_foreach(void *base, size_t n, size_t size,
                            void (*fn)(void *elem, void *ctx), void *ctx)
{
    char *p = base;
    for (size_t i = 0; i < n; i++, p += size) fn(p, ctx);
}

static int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);        /* avoids the x - y overflow bug */
}
static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static void scale_double(void *elem, void *ctx)
{
    *(double *)elem *= *(const double *)ctx;
}

int main(void)
{
    puts("=== const PLACEMENT: read RIGHT TO LEFT from the name ===");
    {
        int a = 1, b = 2;

        const int *p1 = &a;         /* p1: pointer to const int */
        p1 = &b;                    /* moving the pointer: OK   */
        /* *p1 = 5;                    writing through it: ERROR */
        printf("  const int *p1        : *p1 is READ-ONLY, p1 can move.  *p1 = %d\n", *p1);

        int * const p2 = &a;        /* p2: const pointer to int */
        *p2 = 5;                    /* writing through it: OK   */
        /* p2 = &b;                    moving the pointer: ERROR */
        printf("  int * const p2       : p2 is FIXED, *p2 is writable.  *p2 = %d\n", *p2);

        const int * const p3 = &a;  /* neither */
        printf("  const int * const p3 : neither can change.           *p3 = %d\n", *p3);

        puts("\n  The rule: const applies to what is IMMEDIATELY LEFT of it;");
        puts("  if there is nothing to its left, to what is on its right. So");
        puts("      const int *p   and   int const *p    are IDENTICAL.");
        puts("  Reading right-to-left from the variable name always works:");
        puts("      int * const p  ->  p is a const pointer to int");
        puts("      const int * p  ->  p is a pointer to an int that is const");
    }

    puts("\n=== const IS A CONTRACT WITH YOUR CALLER ===");
    {
        int data[] = {1, 2, 3, 4, 5};
        printf("  sum(const int *, n) = %lld\n", sum(data, 5));
        puts("  Benefits: the compiler enforces it, readers can trust it, and the");
        puts("  optimiser may assume the data does not change under it.");
        puts("  Put const on EVERY pointer parameter you do not write through.");
    }

    puts("\n=== const IS SHALLOW ===");
    {
        typedef struct { int *values; size_t n; } Wrapper;
        int nums[3] = {1, 2, 3};
        Wrapper w = { nums, 3 };
        const Wrapper *cw = &w;

        /* cw->values = NULL;   <- ERROR: the POINTER member is const */
        cw->values[0] = 99;   /* LEGAL: what it points AT is not const */
        printf("  const Wrapper *cw; cw->values[0] = 99 -> nums[0] = %d\n", nums[0]);
        puts("  const on a struct pointer protects the struct's own bytes, not");
        puts("  whatever its pointer members reach. C has no deep const.");
    }

    puts("\n=== STRING LITERALS SHOULD ALWAYS BE const char * ===");
    {
        const char *msg = "read-only, in .rodata";
        printf("  const char *msg = \"%s\";\n", msg);
        puts("  char *msg = \"...\";  compiles (for now) but lets you write");
        puts("  msg[0] = 'X', which is UNDEFINED BEHAVIOUR and usually SIGSEGV.");
        puts("  Adding const turns a runtime crash into a compile error.");
    }

    puts("\n=== void *: THE GENERIC POINTER ===");
    {
        int    i = 42;
        double d = 3.14;
        char   s[] = "text";

        /* Any object pointer converts to void* and back, with no cast, losslessly. */
        void *anything;
        anything = &i; printf("  void* <- int*    then back: %d\n",  *(int *)anything);
        anything = &d; printf("  void* <- double* then back: %g\n",  *(double *)anything);
        anything = s;  printf("  void* <- char*   then back: %s\n",  (char *)anything);

        puts("\n  What you CANNOT do with a void*:");
        puts("    *p          dereference — the compiler has no idea how many bytes");
        puts("    p + 1       arithmetic  — no element size to scale by");
        puts("                (GCC allows it as an extension, treating it as char*;");
        puts("                 -Wpedantic warns. Cast to char* and mean it.)");
        puts("    sizeof(*p)  same reason");
    }

    puts("\n=== DO NOT CAST malloc IN C ===");
    {
        /* The idiomatic form. `sizeof *a` rather than `sizeof(int)` means the
         * line stays correct if you later change a's type. */
        int *a = malloc(10 * sizeof *a);
        if (a == NULL) { perror("malloc"); return 1; }
        for (int i = 0; i < 10; i++) a[i] = i * i;
        printf("  int *a = malloc(10 * sizeof *a);   a[9] = %d\n", a[9]);
        free(a);
        puts("  void* converts to int* implicitly, so the cast adds nothing.");
        puts("  Before C99 the cast could HIDE a missing #include <stdlib.h>,");
        puts("  which made malloc default to returning int — a real 64-bit crash.");
        puts("  (In C++ the cast IS required, which is where the habit comes from.)");
    }

    puts("\n=== GENERIC ALGORITHMS OVER void* ===");
    {
        int    ints[]    = {5, 3, 9, 1, 7};
        double doubles[] = {5.5, 3.3, 9.9, 1.1};
        char   strs[3][8] = {"beta", "alpha", "gamma"};

        puts("  generic_swap works on ANY type, given its size:");
        printf("    ints before: %d %d ...\n", ints[0], ints[1]);
        generic_swap(&ints[0], &ints[1], sizeof ints[0]);
        printf("    ints after : %d %d\n", ints[0], ints[1]);
        printf("    strs before: %s %s\n", strs[0], strs[1]);
        generic_swap(strs[0], strs[1], sizeof strs[0]);
        printf("    strs after : %s %s\n", strs[0], strs[1]);

        puts("\n  generic_find with a caller-supplied comparator:");
        int key_i = 9;
        int *found_i = generic_find(ints, 5, sizeof ints[0], &key_i, cmp_int);
        printf("    find 9 in ints    -> %s (index %td)\n",
               found_i ? "found" : "not found", found_i ? found_i - ints : -1);

        double key_d = 3.3;
        double *found_d = generic_find(doubles, 4, sizeof doubles[0], &key_d, cmp_double);
        printf("    find 3.3 in doubles-> %s (index %td)\n",
               found_d ? "found" : "not found", found_d ? found_d - doubles : -1);

        puts("\n  generic_foreach with a context pointer (C's closure substitute):");
        double factor = 10.0;
        generic_foreach(doubles, 4, sizeof doubles[0], scale_double, &factor);
        printf("    scaled by 10: ");
        for (int i = 0; i < 4; i++) printf("%.1f ", doubles[i]);
        puts("");
        puts("  The `void *ctx` parameter is how C passes captured state to a");
        puts("  callback. Every callback API you write should have one.");
    }

    puts("\n=== qsort AND bsearch: the standard library's own void* API ===");
    {
        int a[] = {42, 7, 19, 3, 88, 15};
        size_t n = sizeof a / sizeof a[0];

        qsort(a, n, sizeof a[0], cmp_int);
        printf("  sorted: ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        puts("");

        int key = 19;
        int *hit = bsearch(&key, a, n, sizeof a[0], cmp_int);
        printf("  bsearch(19) -> %s at index %td\n",
               hit ? "found" : "not found", hit ? hit - a : -1);

        puts("\n  Comparator rules:");
        puts("    return <0 if a sorts before b, 0 if equal, >0 if after");
        puts("    NEVER write `return *(int*)a - *(int*)b;` — that overflows for");
        puts("    large values and silently sorts wrong. Use (x > y) - (x < y).");
        puts("    The comparator must be a strict weak ordering, or qsort's");
        puts("    behaviour is undefined (it can run off the end of the array).");
    }

    puts("\n=== TYPE PUNNING: use memcpy, not a cast ===");
    {
        float f = 1.5f;
        uint32_t bits;
        memcpy(&bits, &f, sizeof bits);          /* correct */
        printf("  float 1.5f has the bit pattern 0x%08X\n", bits);
        puts("  int i = *(int *)&f;   <- violates STRICT ALIASING and is UB.");
        puts("  The compiler assumes a float* and an int* never point at the same");
        puts("  object, and reorders loads and stores accordingly. At -O2 this");
        puts("  really does produce wrong answers.");
        puts("  memcpy is the portable fix, and every compiler optimises it to");
        puts("  the same single instruction. A union is also legal in C (not C++).");
        puts("  char* and unsigned char* are exempt — they may alias anything,");
        puts("  which is exactly why memcpy itself is allowed to work.");
    }

    return 0;
}
