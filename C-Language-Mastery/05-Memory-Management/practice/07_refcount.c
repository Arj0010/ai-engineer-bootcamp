/* 07_refcount.c — reference counting: shared ownership without a garbage collector.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 07_refcount.c -o t -lpthread && ./t
 *   valgrind --leak-check=full ./t          # zero leaks, and it proves the scheme
 *
 * Single ownership (one owner calls free) covers most C code. When several
 * independent parts of a program need to keep the same object alive and none
 * of them knows which will finish last, you need SHARED ownership.
 *
 * Reference counting is the answer C uses. It is also what Python, Swift,
 * std::shared_ptr, and COM use. Its one real weakness — reference CYCLES —
 * is demonstrated at the end, along with the standard fix.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>

/* ================================================================= *
 * A REFERENCE-COUNTED BUFFER
 *
 * The count is part of the object. retain() adds an owner, release()
 * removes one, and the last release destroys it.
 * ================================================================= */
typedef struct {
    atomic_int  refcount;    /* atomic so it is safe across threads */
    size_t      size;
    char       *name;        /* an owned sub-allocation, freed by the destructor */
    unsigned char data[];    /* flexible array member: payload in the SAME block */
} Buffer;

static size_t live_buffers = 0;      /* so we can prove nothing leaks */

/* CREATE — the caller receives ONE reference and must eventually release it. */
static Buffer *buffer_create(const char *name, size_t size)
{
    /* One allocation for the header AND the payload, thanks to the flexible
     * array member. Half the allocations, and the data is adjacent to the
     * metadata — one cache line often covers both. */
    Buffer *b = malloc(sizeof *b + size);
    if (b == NULL) return NULL;

    size_t n = strlen(name) + 1;
    b->name = malloc(n);
    if (b->name == NULL) { free(b); return NULL; }
    memcpy(b->name, name, n);

    atomic_init(&b->refcount, 1);        /* the creator holds the first reference */
    b->size = size;
    memset(b->data, 0, size);
    live_buffers++;
    printf("    [create ] %-10s refcount=1\n", b->name);
    return b;
}

/* RETAIN — "I am also an owner now." Returns the object for convenient chaining. */
static Buffer *buffer_retain(Buffer *b)
{
    if (b == NULL) return NULL;
    int old = atomic_fetch_add(&b->refcount, 1);
    printf("    [retain ] %-10s refcount %d -> %d\n", b->name, old, old + 1);
    return b;
}

/* RELEASE — "I am done." The LAST release frees the object.
 *
 * atomic_fetch_sub returns the value BEFORE the subtraction, so `old == 1`
 * means we just took it to zero and we are the one who must destroy it.
 * Exactly one thread can observe that, which is what makes this safe. */
static void buffer_release(Buffer *b)
{
    if (b == NULL) return;
    int old = atomic_fetch_sub(&b->refcount, 1);
    if (old > 1) {
        printf("    [release] %-10s refcount %d -> %d\n", b->name, old, old - 1);
        return;
    }
    printf("    [DESTROY] %-10s refcount reached 0 — freeing\n", b->name);
    free(b->name);        /* destroy owned sub-resources first... */
    free(b);              /* ...then the object itself */
    live_buffers--;
}

static int buffer_count(const Buffer *b) { return atomic_load(&b->refcount); }

/* ================================================================= *
 * Consumers that each take their own reference.
 * ================================================================= */
typedef struct { const char *label; Buffer *buf; } Consumer;

static void consumer_attach(Consumer *c, const char *label, Buffer *b)
{
    c->label = label;
    c->buf   = buffer_retain(b);       /* I intend to keep this alive */
}
static void consumer_detach(Consumer *c)
{
    buffer_release(c->buf);            /* I no longer need it */
    c->buf = NULL;
}

/* ================================================================= *
 * THE CYCLE PROBLEM — and the weak-reference fix.
 * ================================================================= */
typedef struct Parent Parent;
typedef struct Child  Child;

struct Parent { int rc; const char *name; Child *child; };   /* STRONG ref to child */
struct Child  { int rc; const char *name; Parent *parent; }; /* strong ref -> CYCLE */

static int cyclic_live = 0;

static Parent *parent_new(const char *n) { Parent *p = calloc(1, sizeof *p); p->rc = 1; p->name = n; cyclic_live++; return p; }
static Child  *child_new (const char *n) { Child  *c = calloc(1, sizeof *c); c->rc = 1; c->name = n; cyclic_live++; return c; }

static void parent_release(Parent *p);
static void child_release(Child *c);

static void parent_release(Parent *p)
{
    if (p == NULL || --p->rc > 0) return;
    printf("    freeing parent %s\n", p->name);
    if (p->child) child_release(p->child);
    free(p); cyclic_live--;
}
static void child_release(Child *c)
{
    if (c == NULL || --c->rc > 0) return;
    printf("    freeing child %s\n", c->name);
    if (c->parent) parent_release(c->parent);
    free(c); cyclic_live--;
}

int main(void)
{
    puts("=== WHY SHARED OWNERSHIP ===");
    puts("  Single ownership works when ONE piece of code decides when an object");
    puts("  dies. It breaks when several independent parts hold the same object");
    puts("  and none of them knows which will finish last:");
    puts("    - a cache entry also held by two in-flight requests");
    puts("    - a texture referenced by several scene nodes");
    puts("    - a parsed config shared by every subsystem");
    puts("  Free it too early: use-after-free. Never free it: a leak.\n");

    puts("=== THE PROTOCOL ===");
    puts("  create()  -> you receive ONE reference, count = 1");
    puts("  retain()  -> count + 1, you are now also an owner");
    puts("  release() -> count - 1; at zero, the object destroys itself");
    puts("  INVARIANT: every retain is matched by exactly one release, and");
    puts("  create counts as a retain.\n");

    puts("=== A SHARED BUFFER ===");
    {
        Buffer *shared = buffer_create("shared", 1024);
        if (shared == NULL) return 1;
        memcpy(shared->data, "payload", 8);

        Consumer a, b, c;
        consumer_attach(&a, "renderer", shared);
        consumer_attach(&b, "physics",  shared);
        consumer_attach(&c, "audio",    shared);
        printf("  three consumers attached; refcount = %d\n", buffer_count(shared));

        /* The CREATOR can now let go. The object survives — this is the whole
         * point. With single ownership, this free would strand three users. */
        printf("  creator releases its own reference:\n");
        buffer_release(shared);
        printf("  object is still alive (refcount %d) and still readable: \"%s\"\n",
               buffer_count(a.buf), (const char *)a.buf->data);

        printf("  consumers finish, in an arbitrary order:\n");
        consumer_detach(&b);
        consumer_detach(&a);
        consumer_detach(&c);          /* the last one out frees it */
        printf("  live buffers: %zu\n", live_buffers);
        puts("  Nobody had to know they were last. The count knew.");
    }

    puts("\n=== FLEXIBLE ARRAY MEMBER: header and payload in ONE allocation ===");
    {
        Buffer *b = buffer_create("single", 64);
        printf("  struct is %zu bytes; the allocation was %zu bytes\n",
               sizeof *b, sizeof *b + 64);
        printf("  b       = %p\n", (void *)b);
        printf("  b->data = %p  (+%td bytes — immediately after the header)\n",
               (void *)b->data, (char *)b->data - (char *)b);
        puts("  `unsigned char data[];` as the LAST member declares a flexible");
        puts("  array member (C99). sizeof does not count it; you allocate");
        puts("  sizeof(T) + n extra bytes and index data[0..n-1].");
        puts("  Benefits: one malloc instead of two, one free, no second pointer");
        puts("  to dereference, and the data shares a cache line with the header.");
        buffer_release(b);
    }

    puts("\n=== WHY THE COUNT IS ATOMIC ===");
    puts("  With a plain int, `count--` is three steps: load, subtract, store.");
    puts("  Two threads releasing at once can both load 2, both store 1, and");
    puts("  the object is never freed — or worse, both see 0 and both free it.");
    puts("  atomic_fetch_sub is a single indivisible operation, and it returns");
    puts("  the OLD value, so exactly one caller can observe the 1 -> 0");
    puts("  transition. That caller, and only that caller, destroys the object.");
    puts("  (The object's CONTENTS still need their own lock; the refcount only");
    puts("   protects the object's LIFETIME.)");

    puts("\n=== THE CYCLE PROBLEM ===");
    {
        printf("  parent -> child (strong), child -> parent (strong)\n");
        Parent *p = parent_new("P");
        Child  *c = child_new("C");
        p->child  = c;  c->rc++;         /* parent holds a strong ref to child  */
        c->parent = p;  p->rc++;         /* child holds a strong ref to parent  */
        printf("  after linking: parent rc=%d, child rc=%d, live=%d\n",
               p->rc, c->rc, cyclic_live);

        printf("  dropping BOTH external references:\n");
        parent_release(p);
        child_release(c);
        printf("  live objects still allocated: %d   <- LEAKED\n", cyclic_live);
        puts("  Each object's count never reaches zero, because the other one is");
        puts("  still holding a reference. They keep each other alive forever.");
        puts("  Reference counting cannot collect cycles. Full stop.");

        /* Deliberately leaked to make the point; break it manually so valgrind
         * stays clean and the demo does not lie about its own cleanliness. */
        p->child = NULL; c->parent = NULL;
        free(p); free(c); cyclic_live -= 2;
        printf("  (manually broken and freed so this program leaks nothing: live=%d)\n",
               cyclic_live);
    }

    puts("\n=== FIXING CYCLES: WEAK REFERENCES ===");
    puts("  Make ONE direction non-owning. The classic rule:");
    puts("    parent -> child   STRONG  (the parent keeps the child alive)");
    puts("    child  -> parent  WEAK    (a raw pointer, no count, no ownership)");
    puts("  The child must then treat its parent pointer as possibly dangling —");
    puts("  in practice the parent clears it in its own destructor.");
    puts("");
    puts("  This is exactly std::weak_ptr, Swift's `weak`, and Python's weakref.");
    puts("  Python additionally ships a cycle-detecting GC on top of refcounting,");
    puts("  because in a dynamic language cycles are unavoidable.");

    puts("\n=== COSTS AND ALTERNATIVES ===");
    puts("  Reference counting costs:");
    puts("    - an atomic increment/decrement on every share (contended cache line)");
    puts("    - 4-8 bytes per object");
    puts("    - cannot collect cycles");
    puts("    - destruction can cascade unpredictably (freeing one object frees");
    puts("      a thousand) — a latency spike in real-time code");
    puts("");
    puts("  Reach for it ONLY when lifetimes genuinely cannot be determined");
    puts("  statically. In order of preference:");
    puts("    1. stack / automatic lifetime           — free, impossible to get wrong");
    puts("    2. single ownership, documented         — covers most real C");
    puts("    3. arena: everything dies together      — module 05, file 04");
    puts("    4. reference counting                   — genuinely shared lifetimes");
    puts("    5. a tracing garbage collector          — you are writing a runtime");

    printf("\n  final check: live_buffers = %zu, cyclic_live = %d (both must be 0)\n",
           live_buffers, cyclic_live);
    return 0;
}
