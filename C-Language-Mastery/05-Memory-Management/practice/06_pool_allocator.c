/* 06_pool_allocator.c — fixed-size pool with an INTRUSIVE free list.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 06_pool_allocator.c -o t && ./t
 *
 * When you allocate many objects of ONE size — list nodes, particles, network
 * connections, AST nodes — a pool beats malloc on every axis:
 *
 *   alloc  : pop the free-list head   ~3 instructions, O(1)
 *   free   : push onto the free list  ~3 instructions, O(1)
 *   overhead: ZERO bytes per object
 *   fragmentation: impossible — every slot is interchangeable
 *
 * THE TRICK that makes overhead zero: while a slot is FREE, nobody is using
 * its bytes, so we store the "next free slot" pointer INSIDE the slot itself.
 * The free list costs nothing because it lives in memory that is idle anyway.
 * This is exactly how the Linux kernel's slab allocator works.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* ================================================================= *
 * THE POOL
 * ================================================================= */
typedef struct FreeSlot { struct FreeSlot *next; } FreeSlot;

typedef struct {
    unsigned char *memory;      /* one block holding all the slots */
    FreeSlot      *free_list;   /* head of the singly-linked list of free slots */
    size_t         slot_size;   /* bytes per slot (>= sizeof(FreeSlot)) */
    size_t         capacity;    /* how many slots */
    size_t         in_use;      /* how many are handed out right now */
    size_t         peak;        /* high-water mark */
} Pool;

static bool pool_init(Pool *p, size_t object_size, size_t capacity)
{
    /* A slot must be big enough to hold a FreeSlot pointer while it is free,
     * and aligned enough for whatever the user stores in it. */
    size_t align = _Alignof(max_align_t);
    size_t slot  = object_size < sizeof(FreeSlot) ? sizeof(FreeSlot) : object_size;
    slot = (slot + align - 1) & ~(align - 1);

    p->memory = malloc(slot * capacity);
    if (p->memory == NULL) return false;

    p->slot_size = slot;
    p->capacity  = capacity;
    p->in_use    = 0;
    p->peak      = 0;

    /* Thread every slot onto the free list, in reverse so the list comes out
     * in ascending address order — which keeps early allocations sequential
     * in memory and therefore cache-friendly. */
    p->free_list = NULL;
    for (size_t i = capacity; i-- > 0; ) {
        FreeSlot *s = (FreeSlot *)(p->memory + i * slot);
        s->next = p->free_list;
        p->free_list = s;
    }
    return true;
}

static void pool_destroy(Pool *p)
{
    free(p->memory);
    p->memory = NULL; p->free_list = NULL; p->capacity = p->in_use = 0;
}

/* ALLOCATE: pop the head of the free list. Three instructions, no search. */
static void *pool_alloc(Pool *p)
{
    if (p->free_list == NULL) return NULL;        /* pool exhausted */
    FreeSlot *slot = p->free_list;
    p->free_list = slot->next;                    /* advance the head */
    p->in_use++;
    if (p->in_use > p->peak) p->peak = p->in_use;
    return slot;                                  /* the caller now owns these bytes */
}

/* FREE: push onto the head of the free list. Also three instructions. */
static void pool_free(Pool *p, void *obj)
{
    if (obj == NULL) return;
    FreeSlot *slot = obj;
    slot->next = p->free_list;                    /* reuse the object's own bytes */
    p->free_list = slot;
    p->in_use--;
}

/* Free every slot at once, without walking the objects. */
static void pool_reset(Pool *p)
{
    p->free_list = NULL;
    for (size_t i = p->capacity; i-- > 0; ) {
        FreeSlot *s = (FreeSlot *)(p->memory + i * p->slot_size);
        s->next = p->free_list;
        p->free_list = s;
    }
    p->in_use = 0;
}

/* Is this pointer one of ours, and is it slot-aligned? A pool can validate
 * a free() in a way malloc cannot. */
static bool pool_owns(const Pool *p, const void *obj)
{
    const unsigned char *q = obj;
    if (q < p->memory || q >= p->memory + p->slot_size * p->capacity) return false;
    return ((size_t)(q - p->memory) % p->slot_size) == 0;
}

static size_t pool_free_count(const Pool *p)
{
    size_t n = 0;
    for (const FreeSlot *s = p->free_list; s != NULL; s = s->next) n++;
    return n;
}

/* ================================================================= *
 * A realistic user: linked-list nodes, the classic many-small-objects case.
 * ================================================================= */
typedef struct Particle {
    float x, y, vx, vy;
    int   life;
} Particle;

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== FIXED-SIZE POOL ALLOCATOR ===\n");

    Pool pool;
    if (!pool_init(&pool, sizeof(Particle), 1000)) { perror("pool"); return 1; }

    printf("  object   : Particle, %zu bytes\n", sizeof(Particle));
    printf("  slot     : %zu bytes (rounded up to _Alignof(max_align_t) = %zu)\n",
           pool.slot_size, _Alignof(max_align_t));
    if (pool.slot_size > sizeof(Particle))
        printf("             note the %zu wasted bytes per slot: aligning to\n"
               "             max_align_t is the safe GENERIC choice. A pool for a\n"
               "             known type should use _Alignof(Particle) = %zu instead\n"
               "             and get the slot down to %zu bytes.\n",
               pool.slot_size - sizeof(Particle), _Alignof(Particle),
               sizeof(Particle));
    printf("  capacity : %zu slots = %zu bytes, in ONE malloc\n",
           pool.capacity, pool.slot_size * pool.capacity);
    printf("  free list: %zu slots available\n", pool_free_count(&pool));
    puts("\n  Per-object metadata: ZERO bytes. malloc would spend ~16 bytes of");
    printf("  header on each — %zu KB of pure overhead for %zu objects.\n",
           16 * pool.capacity / 1024, pool.capacity);

    puts("\n=== THE INTRUSIVE FREE LIST ===");
    {
        printf("  free_list head -> %p\n", (void *)pool.free_list);
        printf("  its ->next     -> %p  (%td bytes later = exactly one slot)\n",
               (void *)pool.free_list->next,
               (char *)pool.free_list->next - (char *)pool.free_list);
        puts("  The 'next' pointer lives INSIDE the free slot. While a slot is");
        puts("  free nobody is using its bytes, so the list is free of charge.");
        puts("  The moment we hand the slot out, those bytes become the object's.");
    }

    puts("\n=== ALLOCATE AND FREE ===");
    {
        Particle *a = pool_alloc(&pool);
        Particle *b = pool_alloc(&pool);
        Particle *c = pool_alloc(&pool);
        *a = (Particle){1, 1, 0.5f, 0.5f, 100};
        *b = (Particle){2, 2, 1.0f, 0.0f, 200};
        *c = (Particle){3, 3, 0.0f, 1.0f, 300};

        printf("  a=%p  b=%p  c=%p\n", (void *)a, (void *)b, (void *)c);
        printf("  contiguous: b - a = %td bytes, c - b = %td bytes\n",
               (char *)b - (char *)a, (char *)c - (char *)b);
        printf("  in_use=%zu free=%zu\n", pool.in_use, pool_free_count(&pool));
        puts("  Objects come out ADJACENT in memory — iterating them is a linear");
        puts("  scan, which is the best case for the prefetcher. malloc gives no");
        puts("  such guarantee.");

        pool_free(&pool, b);
        printf("\n  after pool_free(b): in_use=%zu free=%zu\n",
               pool.in_use, pool_free_count(&pool));
        printf("  free_list head is now %p — b's slot, back at the front\n",
               (void *)pool.free_list);

        Particle *d = pool_alloc(&pool);
        printf("  next pool_alloc returned %p  %s\n", (void *)d,
               d == b ? "<- b's exact slot, reused immediately" : "");
        puts("  LIFO reuse is deliberate: the most recently freed slot is the");
        puts("  most likely to still be in cache.");
        pool_free(&pool, a); pool_free(&pool, c); pool_free(&pool, d);
    }

    puts("\n=== A POOL CAN VALIDATE free(), WHICH malloc CANNOT ===");
    {
        Particle *p = pool_alloc(&pool);
        int stack_object;
        printf("  pool_owns(a pool pointer)   = %s\n", pool_owns(&pool, p) ? "yes" : "no");
        printf("  pool_owns(a stack address)  = %s\n", pool_owns(&pool, &stack_object) ? "yes" : "no");
        printf("  pool_owns(pool pointer + 1) = %s   <- misaligned, not a slot start\n",
               pool_owns(&pool, (char *)p + 1) ? "yes" : "no");
        pool_free(&pool, p);
        puts("  Because every slot is at a known offset, a pool can cheaply reject");
        puts("  a bogus free. glibc's free() has no such luxury and simply");
        puts("  corrupts itself.");
    }

    puts("\n=== EXHAUSTION IS GRACEFUL ===");
    {
        pool_reset(&pool);
        size_t got = 0;
        while (pool_alloc(&pool) != NULL) got++;
        printf("  allocated %zu of %zu slots, then pool_alloc returned NULL\n",
               got, pool.capacity);
        puts("  A pool has a hard, known ceiling. In embedded and real-time work");
        puts("  that is a FEATURE: memory use is bounded and provable, with no");
        puts("  fragmentation and no unpredictable allocator pauses.");
        pool_reset(&pool);
    }

    puts("\n=== SPEED: POOL vs malloc ===");
    {
        const int N = 1000000;
        clock_t t;

        Pool big;
        if (!pool_init(&big, sizeof(Particle), N)) { perror("pool"); return 1; }

        /* Pool: allocate all, then free all. */
        void **slots = malloc((size_t)N * sizeof *slots);
        t = clock();
        for (int i = 0; i < N; i++) slots[i] = pool_alloc(&big);
        double t_pool_alloc = seconds_since(t);
        t = clock();
        for (int i = 0; i < N; i++) pool_free(&big, slots[i]);
        double t_pool_free = seconds_since(t);

        /* malloc: the same workload. */
        t = clock();
        for (int i = 0; i < N; i++) slots[i] = malloc(sizeof(Particle));
        double t_malloc_alloc = seconds_since(t);
        t = clock();
        for (int i = 0; i < N; i++) free(slots[i]);
        double t_malloc_free = seconds_since(t);

        printf("  %d objects of %zu bytes:\n", N, sizeof(Particle));
        printf("    pool  : %.4f s alloc  %.4f s free  = %.4f s\n",
               t_pool_alloc, t_pool_free, t_pool_alloc + t_pool_free);
        printf("    malloc: %.4f s alloc  %.4f s free  = %.4f s\n",
               t_malloc_alloc, t_malloc_free, t_malloc_alloc + t_malloc_free);
        if (t_pool_alloc + t_pool_free > 0)
            printf("    pool is %.1fx faster\n",
                   (t_malloc_alloc + t_malloc_free) / (t_pool_alloc + t_pool_free));
        printf("    memory: pool %zu MB vs malloc ~%zu MB (16-byte headers)\n",
               (size_t)N * big.slot_size / (1024*1024),
               (size_t)N * (sizeof(Particle) + 16) / (1024*1024));

        free(slots);
        pool_destroy(&big);
    }

    puts("\n=== ITERATION LOCALITY: the hidden win ===");
    {
        const int N = 200000;
        Pool p2;
        pool_init(&p2, sizeof(Particle), N);

        Particle **pool_objs   = malloc((size_t)N * sizeof *pool_objs);
        Particle **malloc_objs = malloc((size_t)N * sizeof *malloc_objs);
        for (int i = 0; i < N; i++) {
            pool_objs[i]   = pool_alloc(&p2);
            malloc_objs[i] = malloc(sizeof(Particle));
            /* interleave an unrelated allocation so malloc's blocks scatter */
            void *noise = malloc(24); (void)noise;
            pool_objs[i]->x = malloc_objs[i]->x = (float)i;
        }

        clock_t t = clock();
        double sum_pool = 0;
        for (int r = 0; r < 20; r++) for (int i = 0; i < N; i++) sum_pool += pool_objs[i]->x;
        double t_pool = seconds_since(t);

        t = clock();
        double sum_malloc = 0;
        for (int r = 0; r < 20; r++) for (int i = 0; i < N; i++) sum_malloc += malloc_objs[i]->x;
        double t_malloc = seconds_since(t);

        printf("  20 passes over %d objects:\n", N);
        printf("    pool-allocated  : %.4f s (sum %.0f)\n", t_pool, sum_pool);
        printf("    malloc-allocated: %.4f s (sum %.0f)\n", t_malloc, sum_malloc);
        if (t_pool > 0) printf("    %.2fx difference, purely from memory layout\n",
                               t_malloc / t_pool);
        puts("  Same objects, same arithmetic. The pool's objects are contiguous,");
        puts("  so each cache line brings in several of them and the prefetcher");
        puts("  can predict the stride. Module 13 goes into this properly.");

        for (int i = 0; i < N; i++) free(malloc_objs[i]);
        free(pool_objs); free(malloc_objs);
        pool_destroy(&p2);
    }

    puts("\n=== ARENA vs POOL vs malloc: choosing ===");
    puts("                  ARENA           POOL            malloc");
    puts("  object sizes    any             ONE fixed       any");
    puts("  alloc cost      ~5 instr        ~3 instr        ~100s of cycles");
    puts("  free cost       n/a             ~3 instr        ~100s of cycles");
    puts("  individual free NO              YES             yes");
    puts("  overhead/object 0               0               ~16 bytes");
    puts("  fragmentation   none            none            yes");
    puts("  capacity        fixed/chained   fixed           dynamic");
    puts("");
    puts("  Use an ARENA when everything dies at the same moment.");
    puts("  Use a POOL when you churn many objects of one type.");
    puts("  Use malloc when sizes and lifetimes are genuinely irregular.");
    puts("");
    puts("  Real examples: Linux slab/slub allocator (pools of kernel objects),");
    puts("  game engines' entity pools, database page pools, connection pools.");

    pool_destroy(&pool);
    return 0;
}
