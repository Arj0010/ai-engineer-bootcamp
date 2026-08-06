/* 04_arena_allocator.c — the bump allocator: allocate fast, free everything at once.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 04_arena_allocator.c -o t && ./t
 *
 * The idea: grab one big block, then hand out pieces by moving a cursor
 * forward. There is no individual free — you reset the whole arena.
 *
 * That sounds like a limitation. It is usually exactly what you want:
 *   - a web server freeing everything at the end of a request
 *   - a game engine freeing everything at the end of a frame
 *   - a compiler freeing everything after a translation unit
 *   - a parser freeing its whole AST at once
 *
 * You get: allocation in ~5 instructions, zero fragmentation, zero per-block
 * metadata, perfect cache locality, and it is IMPOSSIBLE to leak or double-free.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ================================================================= *
 * THE ARENA
 * ================================================================= */
typedef struct {
    unsigned char *base;      /* the one big block */
    size_t         cap;       /* its size */
    size_t         offset;    /* the bump cursor: everything below is handed out */
    size_t         peak;      /* high-water mark, for tuning the capacity */
    size_t         allocs;    /* how many requests we served */
} Arena;

static bool arena_init(Arena *a, size_t cap)
{
    a->base = malloc(cap);
    if (a->base == NULL) return false;
    a->cap = cap; a->offset = 0; a->peak = 0; a->allocs = 0;
    return true;
}
static void arena_destroy(Arena *a)
{
    free(a->base);
    a->base = NULL; a->cap = a->offset = 0;
}

/* Round up to an alignment boundary. Alignment must be a power of two, which
 * lets us use the mask trick instead of a division. */
static size_t align_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

/* THE WHOLE ALLOCATOR. Align the cursor, check it fits, hand back the old
 * cursor, advance. That is it — no search, no metadata, no free list. */
static void *arena_alloc_aligned(Arena *a, size_t size, size_t align)
{
    size_t start = align_up(a->offset, align);
    if (start + size > a->cap) return NULL;        /* arena exhausted */

    void *p = a->base + start;
    a->offset = start + size;
    if (a->offset > a->peak) a->peak = a->offset;
    a->allocs++;
    return p;
}
static void *arena_alloc(Arena *a, size_t size)
{
    /* Default to max_align_t so any type is safely storable. */
    return arena_alloc_aligned(a, size, _Alignof(max_align_t));
}
static void *arena_calloc(Arena *a, size_t n, size_t size)
{
    if (n != 0 && size > SIZE_MAX / n) return NULL;
    void *p = arena_alloc(a, n * size);
    if (p) memset(p, 0, n * size);
    return p;
}
static char *arena_strdup(Arena *a, const char *s)
{
    size_t n = strlen(s) + 1;
    char *copy = arena_alloc_aligned(a, n, 1);     /* strings need no alignment */
    if (copy) memcpy(copy, s, n);
    return copy;
}

/* FREE EVERYTHING. One instruction. No traversal, no per-object cleanup. */
static void arena_reset(Arena *a) { a->offset = 0; }

/* ================================================================= *
 * SCRATCH MARKERS — an arena with LIFO partial frees.
 *
 * Save the cursor, do temporary work, restore the cursor. Everything
 * allocated in between disappears. This gives you a scratch buffer with
 * no bookkeeping at all.
 * ================================================================= */
typedef size_t ArenaMark;
static ArenaMark arena_mark(const Arena *a)      { return a->offset; }
static void      arena_release(Arena *a, ArenaMark m) { a->offset = m; }

static void arena_stats(const Arena *a, const char *label)
{
    printf("  %-28s used %6zu / %zu bytes (%.1f%%), peak %zu, %zu allocations\n",
           label, a->offset, a->cap,
           100.0 * (double)a->offset / (double)a->cap, a->peak, a->allocs);
}

/* ================================================================= *
 * A realistic use: parsing records into an arena. Note that NOTHING in
 * the parsing code has to think about ownership or cleanup.
 * ================================================================= */
typedef struct Node {
    const char  *key;
    int          value;
    struct Node *next;
} Node;

static Node *parse_into_arena(Arena *a, const char *const *pairs, size_t n)
{
    Node *head = NULL, *tail = NULL;
    for (size_t i = 0; i < n; i++) {
        Node *node = arena_alloc(a, sizeof *node);
        if (node == NULL) return head;              /* out of arena; keep what we have */
        node->key   = arena_strdup(a, pairs[i]);    /* the string lives here too */
        node->value = (int)strlen(pairs[i]);
        node->next  = NULL;
        if (tail) tail->next = node; else head = node;
        tail = node;
    }
    return head;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    Arena arena;
    if (!arena_init(&arena, 64 * 1024)) { perror("arena"); return 1; }

    puts("=== THE ARENA (BUMP) ALLOCATOR ===\n");
    printf("  one malloc of %zu bytes at %p\n", arena.cap, (void *)arena.base);
    arena_stats(&arena, "initial");

    puts("\n=== ALLOCATION IS A POINTER INCREMENT ===");
    {
        int    *a = arena_alloc(&arena, sizeof *a);
        double *b = arena_alloc(&arena, sizeof *b);
        char   *c = arena_alloc(&arena, 100);
        *a = 42; *b = 3.14; strcpy(c, "hello");

        printf("  int    at %p\n", (void *)a);
        printf("  double at %p  (+%td bytes)\n", (void *)b, (char *)b - (char *)a);
        printf("  char[100] at %p  (+%td bytes)\n", (void *)c, (char *)c - (char *)b);
        int *zeroed = arena_calloc(&arena, 4, sizeof *zeroed);
        printf("  arena_calloc(4, 4) -> %d %d %d %d (zeroed)\n",
               zeroed[0], zeroed[1], zeroed[2], zeroed[3]);
        arena_stats(&arena, "after 4 allocations");
        puts("  Consecutive allocations are CONTIGUOUS. No headers between them,");
        puts("  so no per-allocation overhead and excellent cache locality.");
        puts("  Compare with my_malloc, where every block carries a 16-byte header.");
    }

    puts("\n=== ALIGNMENT IS THE ONE SUBTLETY ===");
    {
        arena_reset(&arena);
        char   *one_byte = arena_alloc_aligned(&arena, 1, 1);
        double *needs_8  = arena_alloc(&arena, sizeof *needs_8);  /* max_align_t */
        printf("  1-byte alloc at offset %td\n", (char *)one_byte - (char *)arena.base);
        printf("  double  alloc at offset %td  <- cursor was rounded up\n",
               (char *)needs_8 - (char *)arena.base);
        printf("  ((uintptr_t)needs_8 %% %zu) == %zu  (0 means correctly aligned)\n",
               _Alignof(max_align_t),
               (size_t)((uintptr_t)needs_8 % _Alignof(max_align_t)));
        puts("  Handing back a misaligned pointer is undefined behaviour, and on");
        puts("  ARM/SPARC it is a hard fault. Every allocator must round up.");
        puts("  The cost is a few wasted bytes — that is INTERNAL FRAGMENTATION.");
    }

    puts("\n=== FREE EVERYTHING AT ONCE ===");
    {
        arena_reset(&arena);
        const char *words[] = {"alpha","beta","gamma","delta","epsilon","zeta"};
        Node *list = parse_into_arena(&arena, words, 6);

        printf("  parsed a linked list into the arena: ");
        for (Node *n = list; n; n = n->next) printf("%s(%d) ", n->key, n->value);
        puts("");
        arena_stats(&arena, "after parsing 6 nodes + strings");

        arena_reset(&arena);
        arena_stats(&arena, "after arena_reset()");
        puts("  Every node AND every string freed in ONE instruction (offset = 0).");
        puts("  No traversal, no per-node free, and it is impossible to leak or");
        puts("  double-free — there is no free() to get wrong.");
        puts("  Note the parsing code never mentions ownership at all.");
    }

    puts("\n=== SCRATCH MARKERS: LIFO partial frees ===");
    {
        arena_reset(&arena);
        void *permanent = arena_alloc(&arena, 256);
        (void)permanent;
        arena_stats(&arena, "after a permanent allocation");

        ArenaMark mark = arena_mark(&arena);            /* save the cursor */
        for (int i = 0; i < 10; i++) {
            char *scratch = arena_alloc(&arena, 512);
            snprintf(scratch, 512, "temporary buffer %d", i);
        }
        arena_stats(&arena, "after 10 scratch buffers");

        arena_release(&arena, mark);                    /* restore the cursor */
        arena_stats(&arena, "after arena_release(mark)");
        puts("  All ten scratch buffers gone; the permanent allocation untouched.");
        puts("  This is the pattern for recursive algorithms that need working");
        puts("  space: mark on entry, release on exit, no cleanup code at all.");
    }

    puts("\n=== SPEED: ARENA vs malloc ===");
    {
        const int N = 500000;
        clock_t t;

        Arena big;
        if (!arena_init(&big, (size_t)N * 64 + 4096)) { perror("arena"); return 1; }

        t = clock();
        for (int i = 0; i < N; i++) {
            void *p = arena_alloc(&big, 48);
            if (p) *(int *)p = i;
        }
        double t_arena = seconds_since(t);
        arena_reset(&big);                                /* "free" all N at once */
        double t_arena_free = 0.0;

        t = clock();
        void **ptrs = malloc((size_t)N * sizeof *ptrs);
        for (int i = 0; i < N; i++) {
            ptrs[i] = malloc(48);
            if (ptrs[i]) *(int *)ptrs[i] = i;
        }
        double t_malloc = seconds_since(t);
        t = clock();
        for (int i = 0; i < N; i++) free(ptrs[i]);
        double t_malloc_free = seconds_since(t);
        free(ptrs);
        arena_destroy(&big);

        printf("  %d allocations of 48 bytes:\n", N);
        printf("    arena  : %.4f s alloc + %.4f s reset  = %.4f s total\n",
               t_arena, t_arena_free, t_arena + t_arena_free);
        printf("    malloc : %.4f s alloc + %.4f s frees  = %.4f s total\n",
               t_malloc, t_malloc_free, t_malloc + t_malloc_free);
        if (t_arena > 0)
            printf("    arena is %.1fx faster end to end\n",
                   (t_malloc + t_malloc_free) / (t_arena + t_arena_free));
        puts("  The arena also used ZERO bytes of per-allocation metadata, while");
        puts("  malloc spent 16 bytes of header on each — 8 MB of pure overhead here.");
    }

    puts("\n=== WHEN TO USE AN ARENA ===");
    puts("  GOOD FIT — a clear point where everything can die together:");
    puts("    per-request memory in a server");
    puts("    per-frame memory in a game or renderer");
    puts("    a compiler's AST, freed after the translation unit");
    puts("    parsing: all the nodes and all the strings, one reset");
    puts("    scratch space in a recursive algorithm (with markers)");
    puts("");
    puts("  BAD FIT:");
    puts("    objects with wildly different lifetimes");
    puts("    long-running caches where individual entries expire");
    puts("    anything needing individual free() — that is what pools are for");
    puts("");
    puts("  THE TRADE-OFF:");
    puts("    + allocation is ~5 instructions, no search, no locking");
    puts("    + zero per-allocation metadata");
    puts("    + zero fragmentation");
    puts("    + perfect locality — consecutive allocations are adjacent");
    puts("    + cannot leak, cannot double-free");
    puts("    - no individual free");
    puts("    - you must size the arena, or chain blocks when it fills");
    puts("    - a long-lived object pins the entire arena");
    puts("");
    puts("  Real systems using exactly this: the Linux kernel's per-CPU arenas,");
    puts("  Nginx's per-request pools, Apache's apr_pool_t, Go's per-P caches,");
    puts("  and every fast compiler and interpreter written in the last 20 years.");

    arena_destroy(&arena);
    return 0;
}
