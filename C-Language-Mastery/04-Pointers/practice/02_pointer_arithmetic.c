/* 02_pointer_arithmetic.c — arithmetic in units of the pointed-to TYPE.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_pointer_arithmetic.c -o t && ./t
 */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>   /* PRIuPTR — the portable format macro for uintptr_t */

typedef struct { int id; double score; char tag[8]; } Record;

int main(void)
{
    puts("=== p + 1 ADVANCES BY sizeof(*p) BYTES, NOT 1 ===");
    {
        char   ca[4]; int    ia[4]; double da[4]; Record ra[4];
        char   *pc = ca; int *pi = ia; double *pd = da; Record *pr = ra;

        printf("  %-10s sizeof(*p)=%2zu   p=%p  p+1=%p  delta=%td\n",
               "char *",   sizeof *pc, (void *)pc, (void *)(pc + 1), (pc + 1) - pc);
        printf("  %-10s sizeof(*p)=%2zu   p=%p  p+1=%p  delta=%td bytes\n",
               "int *",    sizeof *pi, (void *)pi, (void *)(pi + 1),
               (char *)(pi + 1) - (char *)pi);
        printf("  %-10s sizeof(*p)=%2zu   p=%p  p+1=%p  delta=%td bytes\n",
               "double *", sizeof *pd, (void *)pd, (void *)(pd + 1),
               (char *)(pd + 1) - (char *)pd);
        printf("  %-10s sizeof(*p)=%2zu   p=%p  p+1=%p  delta=%td bytes\n",
               "Record *", sizeof *pr, (void *)pr, (void *)(pr + 1),
               (char *)(pr + 1) - (char *)pr);
        puts("  The compiler multiplies by the element size for you. This is the");
        puts("  ONLY reason a[i] can work without you writing a[i * sizeof(int)].");
    }

    puts("\n=== a[i] IS DEFINED AS *(a + i) ===");
    {
        int a[6] = {10, 20, 30, 40, 50, 60};
        int *p = a;

        printf("  a[3]        = %d\n", a[3]);
        printf("  *(a + 3)    = %d\n", *(a + 3));
        printf("  p[3]        = %d\n", p[3]);
        printf("  *(p + 3)    = %d\n", *(p + 3));
        printf("  3[a]        = %d   <- legal, since a[3] IS *(a+3) IS *(3+a)\n", 3[a]);
        puts("  Addition commutes, so the subscript syntax does too. Never write");
        puts("  it that way, but knowing why it works means you understand [].");
    }

    puts("\n=== POINTER DIFFERENCE GIVES ELEMENT COUNT ===");
    {
        int a[10];
        int *first = &a[0], *last = &a[9];
        printf("  &a[9] - &a[0] = %td elements   (NOT %td bytes)\n",
               last - first, (char *)last - (char *)first);
        printf("  the type of the difference is ptrdiff_t, printed with %%td\n");
        puts("  p - q is only defined when BOTH point into the same array.");
        puts("  Subtracting unrelated pointers is undefined behaviour, even");
        puts("  though it will happily produce a number.");
    }

    puts("\n=== ONE PAST THE END IS SPECIAL AND LEGAL ===");
    {
        int a[5] = {1, 2, 3, 4, 5};
        int *begin = a;
        int *end   = a + 5;              /* legal to COMPUTE and COMPARE */

        printf("  begin = %p, end = %p, end - begin = %td\n",
               (void *)begin, (void *)end, end - begin);
        printf("  traversal: ");
        for (int *p = begin; p != end; p++) printf("%d ", *p);
        puts("");
        puts("  *end is UNDEFINED BEHAVIOUR — the address is valid, the object is not.");
        puts("  a + 6 is undefined merely to COMPUTE. The standard grants exactly");
        puts("  one past the end, and that is what makes the loop above legal.");
        puts("  (This is also why C++ iterators are modelled on pointers.)");
    }

    puts("\n=== FOUR WAYS TO WALK AN ARRAY ===");
    {
        int a[6] = {1, 2, 3, 4, 5, 6};
        size_t n = sizeof a / sizeof a[0];

        printf("  index         : ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);

        printf("\n  pointer+index : ");
        int *p = a;
        for (size_t i = 0; i < n; i++) printf("%d ", p[i]);

        printf("\n  moving pointer: ");
        for (int *q = a; q < a + n; q++) printf("%d ", *q);

        printf("\n  begin/end     : ");
        for (int *q = a, *e = a + n; q != e; ) printf("%d ", *q++);

        puts("\n  All identical after optimisation. Use indices for clarity;");
        puts("  use moving pointers when the element size is awkward or when");
        puts("  writing generic code over void*.");
    }

    puts("\n=== WALKING BACKWARDS ===");
    {
        int a[5] = {1, 2, 3, 4, 5};
        printf("  signed index  : ");
        for (int i = 4; i >= 0; i--) printf("%d ", a[i]);

        printf("\n  unsigned index: ");
        /* `i >= 0` is ALWAYS true for unsigned — infinite loop. This is the idiom: */
        for (size_t i = 5; i-- > 0; ) printf("%zu:%d ", i, a[i]);

        printf("\n  pointer       : ");
        for (int *p = a + 5; p-- != a; ) printf("%d ", *p);
        puts("\n  `p-- != a` decrements AFTER comparing, so it never goes below a.");
    }

    puts("\n=== WHAT IS AND IS NOT ALLOWED ===");
    puts("  p + n, p - n      yes, within the array (+1 past the end)");
    puts("  p - q             yes, same array only -> ptrdiff_t");
    puts("  p++ p-- p += n    yes");
    puts("  p < q, p == q     yes, same array only");
    puts("  p + q             NO — adding two addresses is meaningless");
    puts("  p * 2, p / 2      NO");
    puts("  arithmetic on void*  NO in ISO C (GCC allows it as an extension,");
    puts("                       treating it as char*; -Wpedantic objects)");
    puts("  arithmetic on a function pointer  NO");

    puts("\n=== CASTING TO char * TO GET BYTE ARITHMETIC ===");
    {
        Record r = {7, 98.5, "alpha"};
        /* offsetof tells you where a member sits within its struct. */
        printf("  sizeof(Record)          = %zu\n", sizeof(Record));
        printf("  offsetof(Record, id)    = %zu\n", offsetof(Record, id));
        printf("  offsetof(Record, score) = %zu   <- note the padding gap\n",
               offsetof(Record, score));
        printf("  offsetof(Record, tag)   = %zu\n", offsetof(Record, tag));

        /* Reaching a member through raw byte arithmetic — this is exactly what
         * the compiler emits for r.score, and what generic containers do. */
        const char *base = (const char *)&r;
        double score_via_bytes;
        memcpy(&score_via_bytes, base + offsetof(Record, score), sizeof score_via_bytes);
        printf("  r.score via byte offset = %.1f  (matches r.score = %.1f)\n",
               score_via_bytes, r.score);
    }

    puts("\n=== GENERIC ARRAY WALKING (how qsort does it) ===");
    {
        /* With only a void*, an element size, and a count, you can walk any
         * array of any type. Cast to char* so arithmetic is in BYTES. */
        Record recs[3] = { {1, 90.0, "a"}, {2, 75.5, "b"}, {3, 88.25, "c"} };
        void  *base = recs;
        size_t elem_size = sizeof recs[0], count = 3;

        printf("  walking %zu elements of %zu bytes generically:\n", count, elem_size);
        for (size_t i = 0; i < count; i++) {
            const Record *r = (const Record *)((const char *)base + i * elem_size);
            printf("    [%zu] id=%d score=%.2f tag=%s\n", i, r->id, r->score, r->tag);
        }
        puts("  This is the whole mechanism behind qsort, bsearch, and every");
        puts("  type-agnostic container you will write in module 09.");
    }

    puts("\n=== POINTERS ARE NOT INTEGERS ===");
    {
        int x = 0;
        int *p = &x;
        /* uintptr_t is the only integer type guaranteed to round-trip a pointer. */
        uintptr_t as_int = (uintptr_t)p;
        int *back = (int *)as_int;
        printf("  (uintptr_t)p = %" PRIuPTR "\n", as_int);
        printf("  round-trip preserved the pointer: %s\n", back == p ? "yes" : "no");
        puts("  Converting pointer -> integer -> pointer is only guaranteed with");
        puts("  uintptr_t/intptr_t from <stdint.h>. Casting to int truncates on");
        puts("  64-bit systems and is a real portability bug.");
    }

    return 0;
}
