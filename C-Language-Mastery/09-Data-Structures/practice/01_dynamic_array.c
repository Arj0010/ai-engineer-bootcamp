/* 01_dynamic_array.c — the growable array. Every other language's list/vector.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 01_dynamic_array.c -o t && ./t
 *   valgrind --leak-check=full ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    int    *data;
    size_t  len;      /* elements in use   */
    size_t  cap;      /* elements allocated */
} Vec;

/* Statistics so we can SEE the amortised behaviour. */
static size_t total_reallocs = 0, total_elements_copied = 0;

static void vec_init(Vec *v) { v->data = NULL; v->len = 0; v->cap = 0; }
static void vec_free(Vec *v) { free(v->data); vec_init(v); }

/* Grow to at least `want`. DOUBLING is what makes append amortised O(1). */
static bool vec_reserve(Vec *v, size_t want)
{
    if (want <= v->cap) return true;

    size_t cap = v->cap ? v->cap : 4;
    while (cap < want) {
        if (cap > SIZE_MAX / 2) return false;      /* would overflow */
        cap *= 2;
    }
    if (cap > SIZE_MAX / sizeof *v->data) return false;

    /* Never `v->data = realloc(v->data, ...)`: on failure realloc returns NULL
     * and the ORIGINAL BLOCK IS STILL ALLOCATED, so that assignment leaks it. */
    int *tmp = realloc(v->data, cap * sizeof *tmp);
    if (tmp == NULL) return false;

    total_reallocs++;
    total_elements_copied += v->len;
    v->data = tmp;
    v->cap  = cap;
    return true;
}

static bool vec_push(Vec *v, int value)
{
    if (v->len == v->cap && !vec_reserve(v, v->len + 1)) return false;
    v->data[v->len++] = value;
    return true;
}

static bool vec_pop(Vec *v, int *out)
{
    if (v->len == 0) return false;
    v->len--;
    if (out) *out = v->data[v->len];
    return true;                          /* capacity is NOT reduced */
}

/* Insert at an index: everything after it shifts right. O(n). */
static bool vec_insert(Vec *v, size_t index, int value)
{
    if (index > v->len) return false;
    if (!vec_reserve(v, v->len + 1)) return false;

    /* memmove, not memcpy — the source and destination overlap. */
    memmove(v->data + index + 1, v->data + index,
            (v->len - index) * sizeof *v->data);
    v->data[index] = value;
    v->len++;
    return true;
}

/* Remove at an index, preserving order. O(n). */
static bool vec_remove(Vec *v, size_t index, int *out)
{
    if (index >= v->len) return false;
    if (out) *out = v->data[index];
    memmove(v->data + index, v->data + index + 1,
            (v->len - index - 1) * sizeof *v->data);
    v->len--;
    return true;
}

/* Remove at an index WITHOUT preserving order: swap in the last element. O(1).
 * When order does not matter this is dramatically better than vec_remove. */
static bool vec_swap_remove(Vec *v, size_t index, int *out)
{
    if (index >= v->len) return false;
    if (out) *out = v->data[index];
    v->data[index] = v->data[--v->len];
    return true;
}

/* Give back the unused capacity. */
static bool vec_shrink_to_fit(Vec *v)
{
    if (v->len == v->cap) return true;
    if (v->len == 0) { free(v->data); v->data = NULL; v->cap = 0; return true; }

    int *tmp = realloc(v->data, v->len * sizeof *tmp);
    if (tmp == NULL) return false;
    v->data = tmp;
    v->cap  = v->len;
    return true;
}

static void vec_print(const Vec *v, const char *label)
{
    printf("  %-22s [", label);
    for (size_t i = 0; i < v->len; i++)
        printf("%d%s", v->data[i], i + 1 < v->len ? ", " : "");
    printf("]  len=%zu cap=%zu\n", v->len, v->cap);
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE DYNAMIC ARRAY ===");
    puts("  Three fields: a pointer, a length, and a capacity.");
    puts("    len = how many elements you have put in");
    puts("    cap = how many fit before the next reallocation");
    puts("  Everything else follows from keeping those two apart.\n");

    Vec v;
    vec_init(&v);

    puts("=== GROWTH BY DOUBLING ===");
    {
        size_t last_cap = 0;
        for (int i = 1; i <= 20; i++) {
            vec_push(&v, i * i);
            if (v.cap != last_cap) {
                printf("  push #%2d -> capacity grew %2zu -> %2zu (copied %zu elements)\n",
                       i, last_cap, v.cap, last_cap);
                last_cap = v.cap;
            }
        }
        vec_print(&v, "after 20 pushes");
        printf("  %zu reallocations, %zu total element copies for 20 pushes\n",
               total_reallocs, total_elements_copied);
        puts("  Doubling means the copies form a geometric series:");
        puts("      4 + 8 + 16 + ... + n  <  2n");
        puts("  so n pushes cost O(n) TOTAL — amortised O(1) each.");
        puts("  Growing by ONE would cost 1+2+3+...+n = O(n^2).");
    }

    puts("\n=== INDEXING IS O(1) ===");
    printf("  v[0]=%d  v[10]=%d  v[19]=%d\n", v.data[0], v.data[10], v.data[19]);
    puts("  data[i] is one multiply and one add. That contiguity is the whole");
    puts("  reason to prefer an array over a linked list.");

    puts("\n=== INSERT AND REMOVE ===");
    {
        Vec w; vec_init(&w);
        for (int i = 1; i <= 6; i++) vec_push(&w, i * 10);
        vec_print(&w, "start");

        vec_insert(&w, 0, 999);
        vec_print(&w, "insert 999 at 0");
        puts("      ^ every element shifted right: O(n)");

        vec_insert(&w, 3, 555);
        vec_print(&w, "insert 555 at 3");

        int removed = 0;
        vec_remove(&w, 0, &removed);
        printf("  removed %d from index 0\n", removed);
        vec_print(&w, "after remove(0)");
        puts("      ^ shifted left: also O(n)");

        vec_swap_remove(&w, 1, &removed);
        printf("  swap_remove(1) took %d\n", removed);
        vec_print(&w, "after swap_remove(1)");
        puts("      ^ the LAST element moved into the hole: O(1), order lost.");
        puts("      When order does not matter, this is the one to use.");

        vec_free(&w);
    }

    puts("\n=== pop DOES NOT SHRINK THE CAPACITY ===");
    {
        int out;
        for (int i = 0; i < 15; i++) vec_pop(&v, &out);
        vec_print(&v, "after 15 pops");
        printf("  holding %zu ints (%zu bytes) for %zu elements\n",
               v.cap, v.cap * sizeof(int), v.len);
        vec_shrink_to_fit(&v);
        vec_print(&v, "after shrink_to_fit");
        puts("  Keeping the capacity is deliberate: a push/pop cycle at the");
        puts("  boundary would otherwise realloc on every single operation.");
        puts("  Shrink explicitly, and only when you know you are done growing.");
    }

    puts("\n=== THE POINTER-INVALIDATION TRAP ===");
    {
        Vec w; vec_init(&w);
        for (int i = 0; i < 4; i++) vec_push(&w, i);

        /* Record the address as an INTEGER: after a realloc that moves the
         * block, even reading the old POINTER's value is undefined. */
        uintptr_t old_addr = (uintptr_t)&w.data[0];
        printf("  &w.data[0] before growth: 0x%" PRIxPTR "\n", old_addr);

        for (int i = 0; i < 100; i++) vec_push(&w, i);      /* forces reallocs */
        printf("  &w.data[0] after growth : %p  %s\n", (void *)&w.data[0],
               (uintptr_t)&w.data[0] != old_addr ? "<- THE BLOCK MOVED" : "");
        puts("  ANY pointer you handed out before the growth now dangles.");
        puts("  RULES:");
        puts("    - never store long-lived pointers INTO a dynamic array");
        puts("    - return INDICES from your API, not element pointers");
        puts("    - or document loudly that any mutation invalidates them");
        puts("  This is exactly C++'s std::vector iterator-invalidation rule,");
        puts("  and it is the most common bug in hand-rolled containers.");
        vec_free(&w);
    }

    puts("\n=== ARRAY vs LINKED LIST: measured ===");
    {
        const int N = 2000000;
        clock_t t;

        Vec big; vec_init(&big);
        t = clock();
        for (int i = 0; i < N; i++) vec_push(&big, i);
        double t_push = seconds_since(t);

        t = clock();
        long long sum = 0;
        for (size_t i = 0; i < big.len; i++) sum += big.data[i];
        double t_scan = seconds_since(t);

        printf("  %d pushes            : %.4f s\n", N, t_push);
        printf("  full scan (sum=%lld): %.4f s (%.1f M elements/sec)\n",
               sum, t_scan, (double)N / t_scan / 1e6);
        printf("  memory: %zu bytes of data, %zu allocated (%.0f%% utilised)\n",
               big.len * sizeof(int), big.cap * sizeof(int),
               100.0 * (double)big.len / (double)big.cap);
        puts("  A linked list of the same 2M ints would use ~4x the memory");
        puts("  (8-byte pointer + padding per 4-byte int) and scan several times");
        puts("  slower, because every node is a potential cache miss.");
        puts("  DEFAULT TO THE DYNAMIC ARRAY. Reach for a list only when you");
        puts("  genuinely need O(1) insertion in the middle given a node pointer.");
        vec_free(&big);
    }

    puts("\n=== THE TRADE-OFF THAT DEFINES IT ===");
    puts("  + O(1) indexing, contiguous memory, cache- and SIMD-friendly");
    puts("  + amortised O(1) append; minimal per-element overhead");
    puts("  - O(n) insert/remove in the middle");
    puts("  - growth copies everything and INVALIDATES all pointers");
    puts("  - up to 50% of the capacity may be unused");
    puts("");
    puts("  Growth factor: 2 is standard. 1.5 wastes less and lets an allocator");
    puts("  reuse freed blocks more often (MSVC uses 1.5 for this reason).");

    vec_free(&v);
    return 0;
}
