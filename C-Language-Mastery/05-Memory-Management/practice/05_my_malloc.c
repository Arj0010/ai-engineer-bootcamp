/* 05_my_malloc.c — a real, working allocator. This is where the heap stops
 * being magic.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 05_my_malloc.c -o my_malloc && ./my_malloc
 *
 * What it implements (roughly malloc circa 1979, and enough to run real code):
 *   - a BLOCK HEADER before each allocation, holding size + free flag
 *   - an IMPLICIT FREE LIST: walk header to header across the whole heap
 *   - FIRST FIT search
 *   - SPLITTING when the found block is much bigger than requested
 *   - COALESCING of adjacent free blocks on release
 *   - ALIGNMENT to max_align_t, so any type can be stored
 *
 * Questions this answers permanently:
 *   Q: How does free() know the size?  A: it reads the header at p - HEADER_SIZE.
 *   Q: Why is malloc slow?             A: it SEARCHES a list.
 *   Q: What is fragmentation?          A: run the visualiser below and watch.
 *
 * The heap comes from mmap (POSIX). On Windows you would use VirtualAlloc.
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/* ================================================================= *
 * THE BLOCK HEADER
 *
 * Every allocation looks like this in memory:
 *
 *     +-------------------+  <- the Header (bookkeeping, hidden from the user)
 *     | size   (bytes)    |
 *     | free   (flag)     |
 *     +-------------------+  <- the pointer WE RETURN to the caller
 *     | ...payload...     |
 *     |                   |
 *     +-------------------+  <- the next block's Header, immediately after
 *
 * The caller sees only the payload. free(p) recovers the header by stepping
 * BACKWARDS: (Header *)((char *)p - HEADER_SIZE). That is the entire trick,
 * and it is why you must pass free() exactly the pointer malloc() returned.
 * ================================================================= */
typedef struct Header {
    size_t size;        /* payload size in bytes, NOT including this header */
    bool   free;        /* is this block available? */
    /* An implicit free list needs no next pointer — the next header is at
     * (char *)payload + size. An explicit free list would add one here, and
     * that is the main upgrade real allocators make. */
} Header;

/* Align the header size up to the strictest alignment any type needs, so the
 * payload we hand back is correctly aligned for a long double, a pointer, or
 * anything else. Misaligned access is UB and on some CPUs a hard fault. */
#define ALIGNMENT   (_Alignof(max_align_t))
#define ALIGN_UP(n) (((n) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE ALIGN_UP(sizeof(Header))

/* The minimum payload worth splitting off. Below this, splitting produces a
 * block too small to ever satisfy a request — pure waste plus a header. */
#define MIN_SPLIT   ALIGN_UP(16)

static void  *heap_start = NULL;
static size_t heap_size  = 0;

/* Statistics, so we can SEE what the allocator is doing. */
static struct {
    size_t allocations, frees, splits, coalesces, bytes_requested, search_steps;
} stats;

/* ---------------------------------------------------------------- *
 * Ask the OS for the heap. Real malloc does this lazily and repeatedly
 * (sbrk for small heaps, mmap for large blocks). One arena is enough here.
 * ---------------------------------------------------------------- */
static bool heap_init(size_t size)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    heap_size = (size + page - 1) & ~(page - 1);        /* round up to pages */

    heap_start = mmap(NULL, heap_size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (heap_start == MAP_FAILED) { heap_start = NULL; return false; }

    /* One giant free block covering the whole arena. Every allocation from
     * here on carves pieces out of it. */
    Header *h = heap_start;
    h->size = heap_size - HEADER_SIZE;
    h->free = true;
    return true;
}

static void heap_destroy(void)
{
    if (heap_start) { munmap(heap_start, heap_size); heap_start = NULL; heap_size = 0; }
}

/* Payload pointer <-> header pointer. These two lines are the whole ABI. */
static Header *header_of(void *payload) { return (Header *)((char *)payload - HEADER_SIZE); }
static void   *payload_of(Header *h)    { return (char *)h + HEADER_SIZE; }

/* The next header is immediately after this block's payload. This adjacency
 * is what makes the list "implicit" — no pointers needed. */
static Header *next_header(Header *h)
{
    char *next = (char *)payload_of(h) + h->size;
    return (next < (char *)heap_start + heap_size) ? (Header *)next : NULL;
}

/* ---------------------------------------------------------------- *
 * ALLOCATE — first fit, with splitting.
 * ---------------------------------------------------------------- */
static void *my_malloc(size_t size)
{
    if (size == 0) return NULL;
    if (heap_start == NULL && !heap_init(1024 * 1024)) return NULL;

    size = ALIGN_UP(size);
    stats.allocations++;
    stats.bytes_requested += size;

    /* FIRST FIT: walk the blocks and take the first one big enough.
     * THIS LOOP IS WHY malloc IS NOT FREE. Best-fit would search the whole
     * heap for the tightest match (less waste, slower). Real allocators use
     * segregated free lists so this search is O(1) for common sizes. */
    for (Header *h = heap_start; h != NULL; h = next_header(h)) {
        stats.search_steps++;
        if (!h->free || h->size < size) continue;

        /* SPLIT if the leftover is big enough to be a usable block of its own.
         * Without splitting, a 4096-byte free block satisfying a 16-byte
         * request would waste 4080 bytes forever. */
        if (h->size >= size + HEADER_SIZE + MIN_SPLIT) {
            Header *rest = (Header *)((char *)payload_of(h) + size);
            rest->size = h->size - size - HEADER_SIZE;
            rest->free = true;
            h->size = size;
            stats.splits++;
        }
        h->free = false;
        return payload_of(h);
    }
    return NULL;                    /* out of memory — exactly what malloc does */
}

/* ---------------------------------------------------------------- *
 * FREE — mark, then coalesce forward.
 * ---------------------------------------------------------------- */
static void my_free(void *p)
{
    if (p == NULL) return;                     /* free(NULL) is a no-op */

    /* Recover the header by stepping backwards. This is why free() needs
     * exactly the pointer malloc() returned — one byte off and it reads
     * garbage as a size and destroys the heap. */
    Header *h = header_of(p);
    if (h->free) {
        fflush(stdout);        /* stderr is unbuffered; keep the two in order */
        fputs("  *** DOUBLE FREE DETECTED — refusing ***\n", stderr);
        return;
    }

    h->free = true;
    stats.frees++;

    /* COALESCE FORWARD: merge with the following block if it is also free.
     * Without this, freeing 100 adjacent 16-byte blocks leaves 100 separate
     * 16-byte holes and a 1000-byte request fails despite 1600 bytes free.
     * That is EXTERNAL FRAGMENTATION. */
    Header *next = next_header(h);
    while (next != NULL && next->free) {
        h->size += HEADER_SIZE + next->size;    /* absorb it, header and all */
        stats.coalesces++;
        next = next_header(h);
    }

    /* Coalescing BACKWARD needs either a boundary tag (a footer duplicating
     * the size, so you can find the previous header) or a doubly-linked free
     * list. We approximate it by scanning from the start — O(n), but it keeps
     * this implementation readable. Real allocators use boundary tags. */
    Header *prev = NULL;
    for (Header *c = heap_start; c != NULL && c != h; c = next_header(c)) prev = c;
    if (prev != NULL && prev->free) {
        prev->size += HEADER_SIZE + h->size;
        stats.coalesces++;
    }
}

/* ---------------------------------------------------------------- *
 * REALLOC — grow in place if we can, otherwise allocate and copy.
 * ---------------------------------------------------------------- */
static void *my_realloc(void *p, size_t size)
{
    if (p == NULL)   return my_malloc(size);       /* realloc(NULL,n) == malloc(n) */
    if (size == 0)   { my_free(p); return NULL; }

    Header *h = header_of(p);
    size_t new_size = ALIGN_UP(size);

    if (new_size <= h->size) return p;              /* shrinking: keep it simple */

    /* Try to absorb the next block if it is free and adjacent — this is how
     * real realloc often avoids the copy. */
    Header *next = next_header(h);
    if (next != NULL && next->free && h->size + HEADER_SIZE + next->size >= new_size) {
        h->size += HEADER_SIZE + next->size;

        /* Absorb only as much as we need: split the surplus back off, exactly
         * as my_malloc does. Skipping this step would silently swallow the
         * whole remaining heap into one allocation. */
        if (h->size >= new_size + HEADER_SIZE + MIN_SPLIT) {
            Header *rest = (Header *)((char *)payload_of(h) + new_size);
            rest->size = h->size - new_size - HEADER_SIZE;
            rest->free = true;
            h->size = new_size;
            stats.splits++;
        }
        return p;                                   /* grew IN PLACE, no copy */
    }

    /* Otherwise: allocate, copy, free. Note this MOVES the block, which is why
     * every pointer into the old block becomes dangling. */
    void *fresh = my_malloc(size);
    if (fresh == NULL) return NULL;                 /* original still valid */
    memcpy(fresh, p, h->size);
    my_free(p);
    return fresh;
}

static void *my_calloc(size_t n, size_t size)
{
    if (n != 0 && size > SIZE_MAX / n) return NULL;   /* overflow check */
    size_t total = n * size;
    void *p = my_malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

/* ---------------------------------------------------------------- *
 * VISUALISATION — this is the part that makes it click.
 * ---------------------------------------------------------------- */
static void heap_dump(const char *label)
{
    printf("\n  %s\n  ", label);
    size_t used = 0, freebytes = 0, blocks = 0, free_blocks = 0, largest_free = 0;

    for (Header *h = heap_start; h != NULL; h = next_header(h)) {
        blocks++;
        if (h->free) {
            free_blocks++;
            freebytes += h->size;
            if (h->size > largest_free) largest_free = h->size;
        } else {
            used += h->size;
        }
        /* One character per block, width proportional to size (capped). */
        int width = (int)(h->size / 64) + 1;
        if (width > 20) width = 20;
        for (int i = 0; i < width; i++) putchar(h->free ? '.' : '#');
        putchar('|');
    }
    printf("\n  %zu blocks (%zu free) | %zu bytes used, %zu free, largest free run %zu\n",
           blocks, free_blocks, used, freebytes, largest_free);
}

int main(void)
{
    puts("=== A WORKING malloc, FROM SCRATCH ===\n");
    printf("  alignment required : %zu bytes (_Alignof(max_align_t))\n", ALIGNMENT);
    printf("  sizeof(Header)     : %zu bytes\n", sizeof(Header));
    printf("  HEADER_SIZE        : %zu bytes (aligned up)\n", (size_t)HEADER_SIZE);
    puts("  Every allocation therefore costs its payload PLUS this header.");
    puts("  That is the per-allocation overhead people mean when they say");
    puts("  \"many small mallocs are wasteful\" — 1 million 8-byte allocations");
    printf("  cost %zu MB of headers alone.\n", (size_t)HEADER_SIZE * 1000000 / (1024*1024));

    if (!heap_init(64 * 1024)) { perror("mmap"); return 1; }
    printf("\n  heap: %zu bytes mmap'd at %p\n", heap_size, heap_start);
    heap_dump("initial state — one big free block");

    /* ---------------- basic allocation ---------------- */
    puts("\n=== ALLOCATING ===");
    void *a = my_malloc(100);
    void *b = my_malloc(200);
    void *c = my_malloc(50);
    void *d = my_malloc(500);
    printf("  a = my_malloc(100) -> %p (header at %p)\n", a, (void *)header_of(a));
    printf("  b = my_malloc(200) -> %p\n", b);
    printf("  c = my_malloc(50)  -> %p\n", c);
    printf("  d = my_malloc(500) -> %p\n", d);
    printf("  gap between a and b: %td bytes = 100 payload + %zu header\n",
           (char *)b - (char *)a, (size_t)HEADER_SIZE);
    heap_dump("after 4 allocations  (# = used, . = free)");

    /* Prove the memory actually works. */
    strcpy((char *)a, "the payload is ordinary, usable memory");
    printf("\n  wrote to a: \"%s\"\n", (char *)a);
    printf("  header says a's size is %zu bytes (requested 100, aligned up)\n",
           header_of(a)->size);
    puts("  THIS is how free() knows the size: it reads the header sitting");
    puts("  immediately before the pointer you hand it.");

    /* ---------------- fragmentation ---------------- */
    puts("\n=== FRAGMENTATION: free the middle blocks ===");
    my_free(b);
    heap_dump("after free(b) — a hole appears");
    my_free(d);
    heap_dump("after free(d) — two separate holes");
    puts("  There is plenty of free memory, but it is in DISCONNECTED pieces.");
    puts("  A request larger than any single hole fails even though the total");
    puts("  free space is ample. That is EXTERNAL FRAGMENTATION.");

    /* ---------------- coalescing ---------------- */
    puts("\n=== COALESCING: free an adjacent block and watch holes merge ===");
    my_free(c);
    heap_dump("after free(c) — c, b's hole and d's hole are now adjacent");
    printf("  coalesces performed so far: %zu\n", stats.coalesces);
    puts("  Merging adjacent free blocks is the only defence against");
    puts("  fragmentation. Without it a long-running process slowly dies.");

    my_free(a);
    heap_dump("after free(a) — everything merged back to one block");

    /* ---------------- splitting ---------------- */
    puts("\n=== SPLITTING: a huge free block satisfying a tiny request ===");
    printf("  before: one free block of %zu bytes\n", ((Header *)heap_start)->size);
    void *tiny = my_malloc(16);
    printf("  my_malloc(16) split it: block is now %zu bytes, remainder %zu\n",
           header_of(tiny)->size, next_header(header_of(tiny))->size);
    puts("  Without splitting, that 16-byte request would have consumed the");
    puts("  entire 64 KB block. That is INTERNAL FRAGMENTATION.");
    my_free(tiny);

    /* ---------------- realloc ---------------- */
    puts("\n=== REALLOC ===");
    {
        char *s = my_malloc(32);
        strcpy(s, "grow me");
        printf("  original at %p: \"%s\"\n", (void *)s, s);

        char *grown = my_realloc(s, 64);          /* next block is free -> in place */
        printf("  realloc to 64  at %p: \"%s\"  %s\n", (void *)grown, grown,
               grown == s ? "<- grew IN PLACE (absorbed the next free block)"
                          : "<- had to MOVE and copy");

        void *blocker = my_malloc(100);            /* now block the way */
        char *moved = my_realloc(grown, 4096);
        printf("  realloc to 4096 at %p: \"%s\"  %s\n", (void *)moved, moved,
               moved == grown ? "<- in place" : "<- MOVED, contents copied");
        puts("  This is exactly why realloc may invalidate your pointers, and");
        puts("  why `p = realloc(p, n)` leaks when it returns NULL.");
        my_free(moved); my_free(blocker);
    }

    /* ---------------- calloc ---------------- */
    puts("\n=== CALLOC ===");
    {
        int *z = my_calloc(8, sizeof *z);
        printf("  my_calloc(8, 4) -> ");
        for (int i = 0; i < 8; i++) printf("%d ", z[i]);
        puts("  (all zero)");
        printf("  my_calloc(SIZE_MAX/2, 8) -> %s (overflow refused)\n",
               my_calloc(SIZE_MAX / 2, 8) == NULL ? "NULL" : "succeeded?!");
        my_free(z);
    }

    /* ---------------- double free ---------------- */
    puts("\n=== DOUBLE FREE DETECTION ===");
    {
        void *p = my_malloc(64);
        my_free(p);
        printf("  freeing p a second time:\n");
        my_free(p);                               /* caught by the free flag */
        puts("  Real allocators detect this too, but the check is best-effort.");
        puts("  glibc aborts with 'double free or corruption'. The fix is on");
        puts("  your side: set p = NULL after freeing.");
    }

    /* ---------------- stats ---------------- */
    puts("\n=== WHAT THE ALLOCATOR DID ===");
    printf("  allocations     : %zu\n", stats.allocations);
    printf("  frees           : %zu\n", stats.frees);
    printf("  bytes requested : %zu\n", stats.bytes_requested);
    printf("  blocks split    : %zu\n", stats.splits);
    printf("  blocks coalesced: %zu\n", stats.coalesces);
    printf("  search steps    : %zu (%.1f per allocation)\n",
           stats.search_steps, (double)stats.search_steps / (double)stats.allocations);
    puts("  Those search steps are the cost of first-fit. Real allocators cut");
    puts("  it to O(1) with segregated free lists — one list per size class —");
    puts("  so a 32-byte request goes straight to the 32-byte list.");

    heap_destroy();

    puts("\n=== HOW REAL ALLOCATORS DIFFER ===");
    puts("  ptmalloc2 (glibc)  per-thread arenas; bins by size; fastbins for");
    puts("                     small blocks; mmap direct for >128 KB");
    puts("  jemalloc (FreeBSD, Rust)  size classes + per-thread caches; tuned");
    puts("                     hard against fragmentation");
    puts("  tcmalloc (Google)  thread-local free lists, near-zero contention");
    puts("  mimalloc (MS)      free-list sharding, very fast small allocations");
    puts("");
    puts("  All of them are this program plus:");
    puts("    - segregated free lists  -> O(1) instead of a linear search");
    puts("    - boundary tags          -> O(1) backward coalescing");
    puts("    - per-thread caches      -> no lock on the common path");
    puts("    - size classes           -> less internal fragmentation");
    puts("    - mmap for large blocks  -> memory actually returns to the OS");
    puts("");
    puts("  WHAT TO TAKE AWAY:");
    puts("    1. free() finds the size in a header just before your pointer.");
    puts("       That is why the pointer must be EXACTLY what malloc returned.");
    puts("    2. Every allocation costs a header. Small allocations are");
    puts("       proportionally very expensive.");
    puts("    3. malloc searches. That search is the cost.");
    puts("    4. Fragmentation is real and coalescing is the only cure.");
    puts("    5. If you know your allocation pattern, an arena or pool");
    puts("       allocator beats malloc by 10-100x (see files 04 and 06).");

    return 0;
}
