/* 02_malloc_basics.c — malloc, calloc, realloc, free: the idioms and the traps.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_malloc_basics.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * NOTE ON VALGRIND: this file reports ZERO leaks, but it DOES report
 * "Conditional jump or move depends on uninitialised value(s)". That is not a
 * defect — the "malloc DOES NOT ZERO" section below deliberately reads
 * malloc'd-but-unwritten memory so you can see the garbage. Valgrind catching
 * it is the lesson: this is the class of bug ASan misses and valgrind finds.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>

/* A growable int array: the amortised-O(1) append that every dynamic array
 * in every language is built on. Module 09 generalises this. */
typedef struct { int *data; size_t len, cap; } Vec;

static int vec_push(Vec *v, int value)
{
    if (v->len == v->cap) {
        size_t new_cap = (v->cap == 0) ? 4 : v->cap * 2;      /* DOUBLE, never +1 */

        /* Overflow check before multiplying. On a 64-bit box this is paranoia;
         * on a 32-bit one with a big count it is not. */
        if (new_cap > SIZE_MAX / sizeof *v->data) { errno = ENOMEM; return -1; }

        /* NEVER `v->data = realloc(v->data, ...)`. On failure realloc returns
         * NULL and the ORIGINAL BLOCK IS STILL ALLOCATED — assigning NULL over
         * v->data loses the only pointer to it and leaks the whole array. */
        int *tmp = realloc(v->data, new_cap * sizeof *tmp);
        if (tmp == NULL) return -1;                            /* v->data intact */
        v->data = tmp;
        v->cap  = new_cap;
    }
    v->data[v->len++] = value;
    return 0;
}
static void vec_free(Vec *v) { free(v->data); v->data = NULL; v->len = v->cap = 0; }

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE IDIOMATIC ALLOCATION ===");
    {
        size_t n = 10;
        /* sizeof *a, not sizeof(int): if a's type ever changes, this line
         * stays correct instead of silently under-allocating. */
        int *a = malloc(n * sizeof *a);
        if (a == NULL) { perror("malloc"); return 1; }         /* ALWAYS check */

        for (size_t i = 0; i < n; i++) a[i] = (int)(i * i);
        printf("  int *a = malloc(n * sizeof *a);  ->  ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        puts("");
        free(a);
        a = NULL;                     /* so a later use crashes instead of corrupting */

        puts("  Three rules in that one line:");
        puts("    1. sizeof *a  — survives a type change; sizeof(int) does not");
        puts("    2. no cast    — void* converts implicitly in C, and pre-C99 the");
        puts("                    cast could hide a missing #include <stdlib.h>");
        puts("    3. check NULL — every time, no exceptions");
    }

    puts("\n=== malloc DOES NOT ZERO. calloc DOES. ===");
    {
        int *m = malloc(8 * sizeof *m);
        int *c = calloc(8, sizeof *c);
        if (m && c) {
            printf("  malloc: ");
            for (int i = 0; i < 8; i++) printf("%d ", m[i]);
            puts("   <- garbage (whatever was there before)");
            printf("  calloc: ");
            for (int i = 0; i < 8; i++) printf("%d ", c[i]);
            puts("   <- guaranteed all zero");
        }
        free(m); free(c);
        puts("  Note: fresh pages from the OS often happen to be zero, so malloc");
        puts("  may LOOK zeroed on a first run. Never rely on it — reuse of freed");
        puts("  blocks is where the garbage shows up, usually in production.");
    }

    puts("\n=== calloc ALSO CHECKS FOR MULTIPLICATION OVERFLOW ===");
    {
        /* This is not a theoretical concern: it is a recurring CVE pattern. */
        /* volatile so the compiler cannot constant-fold the multiplication
         * and warn at compile time — in real code this count comes from a
         * file header or a network packet, and is unknowable at compile time. */
        volatile size_t huge_src = SIZE_MAX / 2;
        size_t huge = huge_src;
        printf("  n = SIZE_MAX/2 = %zu\n", huge);
        printf("  n * sizeof(int) computed in size_t = %zu   <- WRAPPED AROUND\n",
               huge * sizeof(int));
        puts("  malloc(n * sizeof(int)) would therefore allocate a TINY block,");
        puts("  and the loop that fills it writes gigabytes past the end.");

        void *p = calloc(huge, sizeof(int));
        printf("  calloc(n, sizeof(int)) -> %s   <- it detects the overflow\n",
               p == NULL ? "NULL (refused)" : "succeeded?!");
        free(p);
        puts("  RULE: when the count comes from outside your program — a file");
        puts("  header, a network packet, user input — use calloc, or check the");
        puts("  multiplication yourself: if (n > SIZE_MAX / size) refuse.");
    }

    puts("\n=== realloc: THREE TRAPS ===");
    {
        int *a = malloc(4 * sizeof *a);
        if (a == NULL) return 1;
        for (int i = 0; i < 4; i++) a[i] = i;
        printf("  original block at %p, contents ", (void *)a);
        for (int i = 0; i < 4; i++) printf("%d ", a[i]);

        /* TRAP 1: assigning the result over the original pointer. */
        int *tmp = realloc(a, 16 * sizeof *a);
        if (tmp == NULL) { free(a); return 1; }     /* a is STILL VALID here */
        a = tmp;
        printf("\n  after realloc to 16: block at %p, contents preserved: ",
               (void *)a);
        for (int i = 0; i < 4; i++) printf("%d ", a[i]);
        puts("");
        puts("  TRAP 1: `p = realloc(p, n)` leaks the whole block when realloc");
        puts("          fails, because the original is still allocated and you");
        puts("          just overwrote the only pointer to it with NULL.");

        /* TRAP 2: the block may move.
         * Note we record the ADDRESSES AS INTEGERS before reallocating. After
         * realloc moves a block, even READING the old pointer's value (never
         * mind dereferencing it) is undefined — the value is indeterminate.
         * GCC's -Wuse-after-free catches the naive version of this demo. */
        uintptr_t old_base     = (uintptr_t)a;
        uintptr_t old_interior = (uintptr_t)&a[2];
        int *moved = realloc(a, 4096 * sizeof *a);
        if (moved) {
            bool relocated = (uintptr_t)moved != old_base;
            printf("  TRAP 2: realloc to 4096 %s the block (0x%" PRIxPTR " -> %p)\n",
                   relocated ? "MOVED" : "kept", old_base, (void *)moved);
            a = moved;
            if (relocated) {
                puts("          Any pointer INTO the old block is now dangling.");
                printf("          The old &a[2] was 0x%" PRIxPTR ", which is freed memory.\n",
                       old_interior);
            }
            puts("          If you must keep interior references, store OFFSETS,");
            puts("          not pointers, and recompute them after every growth.");
        }
        free(a);

        puts("  TRAP 3: realloc(p, 0) is implementation-defined in C17 and");
        puts("          undefined in C23. Never use it to free. Call free().");
        puts("  Also useful: realloc(NULL, n) is exactly malloc(n), which lets");
        puts("  a growth loop start from a NULL pointer with no special case.");
    }

    puts("\n=== GROWTH STRATEGY: DOUBLE, NEVER +1 ===");
    {
        const int N = 200000;
        clock_t t;

        /* The wrong way: realloc on every append. Each one may copy everything. */
        t = clock();
        int *bad = NULL;
        size_t bad_n = 0;
        for (int i = 0; i < N; i++) {
            int *tmp = realloc(bad, (bad_n + 1) * sizeof *bad);
            if (!tmp) break;
            bad = tmp;
            bad[bad_n++] = i;
        }
        double t_bad = seconds_since(t);
        free(bad);

        /* The right way: double when full. */
        t = clock();
        Vec v = {0};
        for (int i = 0; i < N; i++) if (vec_push(&v, i) != 0) break;
        double t_good = seconds_since(t);
        size_t reallocs = 0;
        for (size_t c = 4; c < v.cap; c *= 2) reallocs++;

        printf("  %d appends, grow-by-one : %.4f s (%d reallocs)\n", N, t_bad, N);
        printf("  %d appends, doubling    : %.4f s (%zu reallocs)\n", N, t_good, reallocs);
        printf("  speedup: %.1fx\n", t_bad > 0 ? t_bad / (t_good > 0 ? t_good : 1e-9) : 0);
        printf("  final: len=%zu cap=%zu (%.0f%% used)\n",
               v.len, v.cap, 100.0 * (double)v.len / (double)v.cap);
        vec_free(&v);

        puts("  Doubling makes n appends cost O(n) TOTAL — amortised O(1) each.");
        puts("  Growing by one is O(n^2). glibc's realloc can sometimes extend in");
        puts("  place, which softens the difference here, but the asymptotics are");
        puts("  real and on other allocators the gap is enormous.");
        puts("  Trade-off: doubling wastes up to 50%% of the capacity. Growth");
        puts("  factors of 1.5 are a common compromise.");
    }

    puts("\n=== free ===");
    {
        int *p = malloc(16);
        free(p);
        p = NULL;
        free(p);          /* free(NULL) is EXPLICITLY a no-op. Always safe. */
        puts("  free(NULL) is a defined no-op — which is what makes the");
        puts("  goto-cleanup pattern work without per-resource if-guards.");
        puts("");
        puts("  After free(p), p is INDETERMINATE. You may not even READ its");
        puts("  value, let alone dereference it. Set it to NULL.");
        puts("");
        puts("  You may only free a pointer that malloc/calloc/realloc returned,");
        puts("  and exactly once. Never free:");
        puts("    - a pointer into the middle of a block  (free(p + 1))");
        puts("    - a stack address                       (free(&local))");
        puts("    - a string literal                      (free(\"abc\"))");
        puts("    - the same block twice");
        puts("  Each of those corrupts the allocator, which usually crashes");
        puts("  somewhere else entirely, minutes later.");
    }

    puts("\n=== free DOES NOT USUALLY RETURN MEMORY TO THE OS ===");
    {
        void *big = malloc(64 * 1024 * 1024);
        if (big) { memset(big, 1, 1024); free(big); }
        puts("  free() marks the block reusable in the allocator's own free list.");
        puts("  Your process's RSS often does not drop. That is not a leak —");
        puts("  it is the allocator keeping memory for the next request, which");
        puts("  is much cheaper than going back to the kernel.");
        puts("  glibc DOES return very large blocks, because it services those");
        puts("  with mmap directly (over MMAP_THRESHOLD, default 128 KB) and");
        puts("  munmaps them on free.");
    }

    puts("\n=== WHAT malloc COSTS ===");
    {
        const int N = 200000;
        clock_t t = clock();
        for (int i = 0; i < N; i++) { void *p = malloc(64); free(p); }
        double t_heap = seconds_since(t);

        t = clock();
        volatile long long sink = 0;
        for (int i = 0; i < N; i++) { char buf[64]; buf[0] = (char)i; sink += buf[0]; }
        double t_stack = seconds_since(t);

        printf("  %d malloc+free pairs : %.4f s (%.0f ns each)\n",
               N, t_heap, t_heap * 1e9 / N);
        printf("  %d stack buffers     : %.4f s (%.0f ns each)\n",
               N, t_stack, t_stack * 1e9 / N);
        puts("  A stack allocation is one instruction. malloc has to search a");
        puts("  free list, split blocks, and update metadata. In a hot loop that");
        puts("  difference is why arena and pool allocators exist (files 04, 06).");
    }

    puts("\n=== THE CHECKLIST ===");
    puts("  [ ] malloc(n * sizeof *p), never sizeof(type)");
    puts("  [ ] check every allocation for NULL");
    puts("  [ ] no cast on malloc");
    puts("  [ ] calloc when the count is externally controlled");
    puts("  [ ] tmp = realloc(p, n); if (tmp) p = tmp;");
    puts("  [ ] double the capacity, never +1");
    puts("  [ ] p = NULL after free");
    puts("  [ ] one owner per allocation, documented in the header");
    puts("  [ ] valgrind --leak-check=full before you believe it works");

    return 0;
}
