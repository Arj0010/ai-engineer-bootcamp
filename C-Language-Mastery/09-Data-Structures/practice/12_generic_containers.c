/* 12_generic_containers.c — three ways to write a container that works for
 * ANY type, in a language with no generics.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 12_generic_containers.c -o t && ./t
 *
 *   1. void* + element size    runtime generic  (what qsort does)
 *   2. macro templates         compile-time     (what most C libraries do)
 *   3. INTRUSIVE links         no allocation    (what the Linux kernel does)
 *
 * Each has a different trade-off between type safety, performance, and how
 * bad the code looks. All three are used in real, serious C.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

/* ================================================================= *
 * APPROACH 1: void* + ELEMENT SIZE
 *
 * The container knows nothing about the type — only how many bytes each
 * element occupies. It copies bytes with memcpy and delegates comparison
 * and destruction to callbacks.
 *
 * This is exactly how qsort, bsearch and lfind work.
 * ================================================================= */
typedef struct {
    void   *data;          /* a raw byte buffer */
    size_t  elem_size;     /* how big one element is */
    size_t  len, cap;
} AnyVec;

static bool anyvec_init(AnyVec *v, size_t elem_size, size_t cap)
{
    v->data = malloc(elem_size * cap);
    if (v->data == NULL) return false;
    v->elem_size = elem_size; v->len = 0; v->cap = cap;
    return true;
}
static void anyvec_free(AnyVec *v) { free(v->data); memset(v, 0, sizeof *v); }

/* Cast to char* so pointer arithmetic is in BYTES. */
static void *anyvec_at(const AnyVec *v, size_t i)
{
    return (char *)v->data + i * v->elem_size;
}

static bool anyvec_push(AnyVec *v, const void *elem)
{
    if (v->len == v->cap) {
        size_t cap = v->cap * 2;
        void *tmp = realloc(v->data, cap * v->elem_size);
        if (tmp == NULL) return false;
        v->data = tmp; v->cap = cap;
    }
    memcpy(anyvec_at(v, v->len), elem, v->elem_size);   /* a byte-wise COPY */
    v->len++;
    return true;
}

static void anyvec_sort(AnyVec *v, int (*cmp)(const void *, const void *))
{
    qsort(v->data, v->len, v->elem_size, cmp);
}
static void *anyvec_find(const AnyVec *v, const void *key,
                         int (*cmp)(const void *, const void *))
{
    for (size_t i = 0; i < v->len; i++) {
        void *e = anyvec_at(v, i);
        if (cmp(e, key) == 0) return e;
    }
    return NULL;
}
static void anyvec_foreach(const AnyVec *v, void (*fn)(void *, void *), void *ctx)
{
    for (size_t i = 0; i < v->len; i++) fn(anyvec_at(v, i), ctx);
}

/* ================================================================= *
 * APPROACH 2: MACRO TEMPLATES
 *
 * Generate a real, typed struct and real, typed functions for each type.
 * You get full type checking, direct member access, and no indirection —
 * at the cost of code bloat and truly awful error messages.
 *
 * This is how stb_ds, klib, and most serious single-header C libraries work.
 * ================================================================= */
#define DEFINE_VEC(TYPE, NAME)                                                 \
    typedef struct { TYPE *data; size_t len, cap; } NAME;                      \
                                                                               \
    static bool NAME##_init(NAME *v, size_t cap) {                             \
        v->data = malloc(cap * sizeof *v->data);                               \
        if (v->data == NULL) return false;                                     \
        v->len = 0; v->cap = cap; return true;                                 \
    }                                                                          \
    static void NAME##_free(NAME *v) { free(v->data); memset(v, 0, sizeof *v); }\
                                                                               \
    static bool NAME##_push(NAME *v, TYPE value) {                             \
        if (v->len == v->cap) {                                                \
            size_t cap = v->cap * 2;                                           \
            TYPE *tmp = realloc(v->data, cap * sizeof *tmp);                   \
            if (tmp == NULL) return false;                                     \
            v->data = tmp; v->cap = cap;                                       \
        }                                                                      \
        v->data[v->len++] = value;   /* a TYPED assignment, not memcpy */      \
        return true;                                                           \
    }                                                                          \
    static bool NAME##_pop(NAME *v, TYPE *out) {                               \
        if (v->len == 0) return false;                                         \
        if (out) *out = v->data[--v->len]; else v->len--;                      \
        return true;                                                           \
    }                                                                          \
    static TYPE NAME##_sum(const NAME *v) {                                    \
        TYPE total = 0;                                                        \
        for (size_t i = 0; i < v->len; i++) total += v->data[i];               \
        return total;                                                          \
    }

DEFINE_VEC(int,    IntVec)
DEFINE_VEC(double, DblVec)
DEFINE_VEC(long,   LongVec)

/* ================================================================= *
 * APPROACH 3: INTRUSIVE CONTAINERS
 *
 * Instead of the container holding your data, YOUR STRUCT holds the
 * container's link. Given a pointer to the link, container_of walks
 * backwards to the enclosing object using its known byte offset.
 *
 * Consequences:
 *   - ZERO allocations for the list itself; the nodes are inside your objects
 *   - an object can be on SEVERAL lists at once (one link field per list)
 *   - one list implementation for every type in the program
 *   - removal is O(1) given just the link
 *
 * This is the Linux kernel's list_head, and it is why the kernel has exactly
 * one linked-list implementation for the entire codebase.
 * ================================================================= */
typedef struct ListHead { struct ListHead *prev, *next; } ListHead;

#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

static void list_init(ListHead *h) { h->prev = h->next = h; }   /* a ring */
static void list_add_tail(ListHead *head, ListHead *node)
{
    node->prev = head->prev;
    node->next = head;
    head->prev->next = node;
    head->prev = node;
}
static void list_del(ListHead *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = node;
}
static bool list_empty(const ListHead *h) { return h->next == h; }

#define LIST_FOREACH(pos, head) \
    for (ListHead *pos = (head)->next; pos != (head); pos = pos->next)

/* Safe against the current node being removed inside the loop body. */
#define LIST_FOREACH_SAFE(pos, tmp, head)                    \
    for (ListHead *pos = (head)->next, *tmp = pos->next;     \
         pos != (head); pos = tmp, tmp = pos->next)

/* A user struct that lives on TWO lists at once. */
typedef struct {
    char     name[16];
    int      priority;
    ListHead all_link;      /* membership in the "everything" list */
    ListHead ready_link;    /* membership in the "ready to run" list */
} Job;

/* ---- callbacks for approach 1 ---- */
typedef struct { double x, y; } Point;

static int cmp_int(const void *a, const void *b)
{ int x = *(const int *)a, y = *(const int *)b; return (x > y) - (x < y); }
static int cmp_point_x(const void *a, const void *b)
{
    const Point *p = a, *q = b;
    return (p->x > q->x) - (p->x < q->x);
}
static int cmp_str(const void *a, const void *b)
{ return strcmp(*(const char *const *)a, *(const char *const *)b); }

static void print_int(void *e, void *ctx)   { (void)ctx; printf("%d ", *(int *)e); }
static void print_point(void *e, void *ctx) { (void)ctx; Point *p = e; printf("(%.1f,%.1f) ", p->x, p->y); }
static void print_str(void *e, void *ctx)   { (void)ctx; printf("%s ", *(char **)e); }

int main(void)
{
    puts("=== C HAS NO GENERICS. THERE ARE THREE ANSWERS. ===\n");

    /* ------------------------------------------------------------- */
    puts("=== APPROACH 1: void* + ELEMENT SIZE (runtime generic) ===");
    {
        /* the SAME container code, three different types */
        AnyVec ints, points, strings;
        anyvec_init(&ints,    sizeof(int),    4);
        anyvec_init(&points,  sizeof(Point),  4);
        anyvec_init(&strings, sizeof(char *), 4);

        int iv[] = {42, 7, 19, 3, 88};
        for (int i = 0; i < 5; i++) anyvec_push(&ints, &iv[i]);

        Point pv[] = {{3,1},{1,2},{2,3}};
        for (int i = 0; i < 3; i++) anyvec_push(&points, &pv[i]);

        char *sv[] = {"delta", "alpha", "charlie"};
        for (int i = 0; i < 3; i++) anyvec_push(&strings, &sv[i]);

        printf("  ints    : "); anyvec_foreach(&ints, print_int, NULL);
        anyvec_sort(&ints, cmp_int);
        printf("-> sorted: "); anyvec_foreach(&ints, print_int, NULL); puts("");

        printf("  points  : "); anyvec_foreach(&points, print_point, NULL);
        anyvec_sort(&points, cmp_point_x);
        printf("-> by x  : "); anyvec_foreach(&points, print_point, NULL); puts("");

        printf("  strings : "); anyvec_foreach(&strings, print_str, NULL);
        anyvec_sort(&strings, cmp_str);
        printf("-> sorted: "); anyvec_foreach(&strings, print_str, NULL); puts("");

        int key = 19;
        int *found = anyvec_find(&ints, &key, cmp_int);
        printf("  find(19) -> %s\n", found ? "found" : "not found");

        anyvec_free(&ints); anyvec_free(&points); anyvec_free(&strings);

        puts("");
        puts("  + ONE implementation, compiled once, works for every type");
        puts("  + the type can be chosen at RUN time");
        puts("  - NO TYPE SAFETY: anyvec_push(&ints, &some_double) compiles fine");
        puts("    and corrupts memory");
        puts("  - every access is a memcpy and a cast");
        puts("  - a function-pointer call per comparison, which cannot be inlined");
        puts("  - the caller must pass a pointer to everything, even an int");
        puts("");
        puts("  This is exactly qsort's design, and qsort is measurably slower");
        puts("  than a type-specific sort for that last reason alone.");
    }

    /* ------------------------------------------------------------- */
    puts("\n=== APPROACH 2: MACRO TEMPLATES (compile-time generic) ===");
    {
        IntVec  iv; IntVec_init(&iv, 4);
        DblVec  dv; DblVec_init(&dv, 4);
        LongVec lv; LongVec_init(&lv, 4);

        for (int i = 1; i <= 5; i++)  IntVec_push(&iv, i * 10);
        for (int i = 1; i <= 5; i++)  DblVec_push(&dv, i * 1.5);
        for (int i = 1; i <= 5; i++)  LongVec_push(&lv, i * 1000000L);

        printf("  IntVec  : ");
        for (size_t i = 0; i < iv.len; i++) printf("%d ", iv.data[i]);
        printf("  sum = %d\n", IntVec_sum(&iv));

        printf("  DblVec  : ");
        for (size_t i = 0; i < dv.len; i++) printf("%.1f ", dv.data[i]);
        printf("  sum = %.1f\n", DblVec_sum(&dv));

        printf("  LongVec : ");
        for (size_t i = 0; i < lv.len; i++) printf("%ld ", lv.data[i]);
        printf("  sum = %ld\n", LongVec_sum(&lv));

        int    iout = 0;
        double dout = 0.0;
        long   lout = 0;
        IntVec_pop(&iv, &iout);
        DblVec_pop(&dv, &dout);
        LongVec_pop(&lv, &lout);
        printf("  pops -> %d, %.1f, %ld   (TYPED returns, no cast anywhere)\n",
               iout, dout, lout);

        IntVec_free(&iv); DblVec_free(&dv); LongVec_free(&lv);

        puts("");
        puts("  + FULL TYPE SAFETY: IntVec_push(&iv, 3.7) is a compile error");
        puts("  + direct member access — v.data[i] with no cast, no memcpy");
        puts("  + everything inlines; identical speed to hand-written code");
        puts("  - CODE BLOAT: one full copy of every function per type. Three");
        puts("    instantiations here produced 15 functions; unused ones even");
        puts("    trigger -Wunused-function until you call them or mark them");
        puts("    inline.");
        puts("  - a compile error inside the macro points at the DEFINE line,");
        puts("    which is genuinely painful to debug");
        puts("  - the container definition itself is unreadable");
        puts("  - no debugger visibility into the macro body");
        puts("");
        puts("  This is what stb_ds, klib and most real C libraries use, because");
        puts("  the type safety and the speed are worth the ugliness.");
    }

    /* ------------------------------------------------------------- */
    puts("\n=== APPROACH 3: INTRUSIVE LINKS (the kernel's answer) ===");
    {
        /* The lists are just heads on the stack. NOTHING is allocated. */
        ListHead all_jobs, ready_jobs;
        list_init(&all_jobs);
        list_init(&ready_jobs);

        Job jobs[5] = {
            {"compile", 3, {0}, {0}}, {"test",    1, {0}, {0}},
            {"deploy",  5, {0}, {0}}, {"lint",    2, {0}, {0}},
            {"package", 4, {0}, {0}},
        };

        /* Every job goes on the "all" list; only some are "ready".
         * The SAME object is on TWO lists at once, via two link fields. */
        for (int i = 0; i < 5; i++) {
            list_add_tail(&all_jobs, &jobs[i].all_link);
            if (jobs[i].priority <= 3) list_add_tail(&ready_jobs, &jobs[i].ready_link);
        }

        printf("  all jobs   : ");
        LIST_FOREACH(pos, &all_jobs) {
            Job *j = CONTAINER_OF(pos, Job, all_link);
            printf("%s(p%d) ", j->name, j->priority);
        }
        puts("");

        printf("  ready jobs : ");
        LIST_FOREACH(pos, &ready_jobs) {
            Job *j = CONTAINER_OF(pos, Job, ready_link);
            printf("%s(p%d) ", j->name, j->priority);
        }
        puts("");
        puts("      THE SAME five objects, on two independent lists, with zero");
        puts("      allocations. A non-intrusive list would need 8 heap nodes.");

        /* Removing while iterating — the reason FOREACH_SAFE exists. */
        printf("  removing every ready job with priority > 1:\n    ");
        LIST_FOREACH_SAFE(pos, tmp, &ready_jobs) {
            Job *j = CONTAINER_OF(pos, Job, ready_link);
            if (j->priority > 1) { printf("removing %s ", j->name); list_del(pos); }
        }
        puts("");
        printf("  ready jobs now: ");
        LIST_FOREACH(pos, &ready_jobs) printf("%s ", CONTAINER_OF(pos, Job, ready_link)->name);
        puts("");
        printf("  all jobs still: ");
        LIST_FOREACH(pos, &all_jobs) printf("%s ", CONTAINER_OF(pos, Job, all_link)->name);
        puts("");
        puts("      Removing from one list left the other untouched, and the");
        puts("      objects themselves were never moved or freed.");
        printf("  ready list empty? %s\n", list_empty(&ready_jobs) ? "yes" : "no");

        puts("");
        printf("  HOW container_of WORKS: offsetof(Job, ready_link) = %zu\n",
               offsetof(Job, ready_link));
        puts("      Given a ListHead* at some address, subtract that offset and");
        puts("      you have the enclosing Job*. It is pure address arithmetic,");
        puts("      resolved at COMPILE time, and costs literally nothing.");
        puts("");
        puts("  + ZERO allocation for the container itself");
        puts("  + an object can be on many lists at once");
        puts("  + O(1) removal given only the link — no search for the object");
        puts("  + ONE list implementation for the entire program");
        puts("  - the object must be modified to carry the link");
        puts("  - you cannot put a type you do not control on the list");
        puts("  - container_of is unpleasant to read until it becomes reflex");
        puts("");
        puts("  This is Linux's list_head, verbatim. Every kernel data structure");
        puts("  uses it, which is why the kernel has ONE linked list rather than");
        puts("  one per type.");
    }

    puts("\n=== CHOOSING ===");
    puts("                    void*         MACRO        INTRUSIVE");
    puts("  type safety       none          full         full");
    puts("  speed             slowest       fastest      fastest");
    puts("  code size         smallest      largest      smallest");
    puts("  allocations       per element   per element  NONE");
    puts("  readability       fair          poor         poor at first");
    puts("  debuggability     good          bad          good");
    puts("  multi-membership  no            no           YES");
    puts("");
    puts("  IN PRACTICE:");
    puts("    - void*      for a one-off, or when the type is a runtime choice,");
    puts("                 or to match an existing API like qsort");
    puts("    - macros     for a reusable library where speed and type safety");
    puts("                 matter — this is the mainstream answer");
    puts("    - intrusive  for systems code, embedded work, or anywhere");
    puts("                 allocation is expensive or forbidden");
    puts("");
    puts("  C11's _Generic (module 11) adds a fourth option for DISPATCH — it");
    puts("  can pick the right typed function based on an argument's type — but");
    puts("  it does not generate the containers themselves. You still need one");
    puts("  of these three underneath.");

    return 0;
}
