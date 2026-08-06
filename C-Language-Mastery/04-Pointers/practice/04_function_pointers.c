/* 04_function_pointers.c — callbacks, dispatch tables, vtables, state machines.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 04_function_pointers.c -o t && ./t
 *
 * Function pointers are how C gets polymorphism. Everything OO-shaped in C —
 * the kernel's file_operations, every plugin system, qsort — is this file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ================================================================= *
 * 1. THE BASICS
 * ================================================================= */
static int add(int a, int b) { return a + b; }
static int sub(int a, int b) { return a - b; }
static int mul(int a, int b) { return a * b; }
static int divide(int a, int b) { return b ? a / b : 0; }

/* ALWAYS typedef function pointer types. `int (*)(int,int)` in a parameter
 * list is unreadable; `BinaryOp` is not. */
typedef int (*BinaryOp)(int, int);

/* ================================================================= *
 * 2. DISPATCH TABLE — replaces an if/else chain with a lookup
 * ================================================================= */
typedef struct {
    char        symbol;
    const char *name;
    BinaryOp    fn;
} Operation;

static const Operation OPERATIONS[] = {
    {'+', "add",      add},
    {'-', "subtract", sub},
    {'*', "multiply", mul},
    {'/', "divide",   divide},
};
static const size_t N_OPERATIONS = sizeof OPERATIONS / sizeof OPERATIONS[0];

static const Operation *lookup_op(char symbol)
{
    for (size_t i = 0; i < N_OPERATIONS; i++)
        if (OPERATIONS[i].symbol == symbol) return &OPERATIONS[i];
    return NULL;
}

/* ================================================================= *
 * 3. CALLBACKS WITH CONTEXT — C's substitute for a closure
 *
 * A callback that cannot carry state is nearly useless. The universal
 * fix is a `void *ctx` parameter that the caller supplies and the
 * callback casts back. Every good C callback API has one.
 * ================================================================= */
typedef void (*Visitor)(int value, void *ctx);

static void for_each(const int *a, size_t n, Visitor visit, void *ctx)
{
    for (size_t i = 0; i < n; i++) visit(a[i], ctx);
}

/* Three different callbacks, three different context types. */
static void accumulate(int v, void *ctx) { *(long long *)ctx += v; }
static void count_over(int v, void *ctx)
{
    struct { int threshold; int count; } *c = ctx;
    if (v > c->threshold) c->count++;
}
static void print_value(int v, void *ctx)
{
    printf("%s%d", (const char *)ctx, v);
}

/* Filter: a predicate decides what survives. */
typedef bool (*Predicate)(int value, void *ctx);
static size_t filter(const int *src, size_t n, int *dst, Predicate keep, void *ctx)
{
    size_t out = 0;
    for (size_t i = 0; i < n; i++)
        if (keep(src[i], ctx)) dst[out++] = src[i];
    return out;
}
static bool is_even(int v, void *ctx)     { (void)ctx; return v % 2 == 0; }
static bool exceeds(int v, void *ctx)     { return v > *(const int *)ctx; }

/* Map + reduce, the same idea. */
typedef int (*Mapper)(int value, void *ctx);
static int scale_by(int v, void *ctx) { return v * *(const int *)ctx; }
static void map_in_place(int *a, size_t n, Mapper f, void *ctx)
{
    for (size_t i = 0; i < n; i++) a[i] = f(a[i], ctx);
}

/* ================================================================= *
 * 4. A VTABLE — "objects" in C
 *
 * A struct of function pointers, shared by all instances of a type,
 * plus a self-pointer. This is literally how C++ implements virtual
 * functions, and how the Linux kernel does polymorphism.
 * ================================================================= */
struct Shape;
typedef struct {
    const char *type_name;
    double (*area)(const struct Shape *self);
    double (*perimeter)(const struct Shape *self);
    void   (*describe)(const struct Shape *self);
} ShapeVTable;

typedef struct Shape {
    const ShapeVTable *vt;          /* the "class" — shared, not per-instance */
    union {                          /* the instance data */
        struct { double r; }        circle;
        struct { double w, h; }     rect;
        struct { double a, b, c; }  triangle;
    } data;
} Shape;

static double circle_area(const Shape *s)      { return 3.14159265358979 * s->data.circle.r * s->data.circle.r; }
static double circle_perim(const Shape *s)     { return 2 * 3.14159265358979 * s->data.circle.r; }
static double rect_area(const Shape *s)        { return s->data.rect.w * s->data.rect.h; }
static double rect_perim(const Shape *s)       { return 2 * (s->data.rect.w + s->data.rect.h); }
static double tri_perim(const Shape *s)        { return s->data.triangle.a + s->data.triangle.b + s->data.triangle.c; }
static double tri_area(const Shape *s)
{
    double a = s->data.triangle.a, b = s->data.triangle.b, c = s->data.triangle.c;
    double p = (a + b + c) / 2;                       /* Heron's formula */
    double v = p * (p - a) * (p - b) * (p - c);
    /* integer-free sqrt via Newton's method, to avoid needing -lm here */
    if (v <= 0) return 0;
    double x = v;
    for (int i = 0; i < 40; i++) x = 0.5 * (x + v / x);
    return x;
}
/* A default implementation shared by every shape — inheritance, effectively. */
static void generic_describe(const Shape *s)
{
    printf("    %-9s area %8.3f  perimeter %8.3f\n",
           s->vt->type_name, s->vt->area(s), s->vt->perimeter(s));
}

static const ShapeVTable CIRCLE_VT   = {"circle",   circle_area, circle_perim, generic_describe};
static const ShapeVTable RECT_VT     = {"rect",     rect_area,   rect_perim,   generic_describe};
static const ShapeVTable TRIANGLE_VT = {"triangle", tri_area,    tri_perim,    generic_describe};

static Shape make_circle(double r)   { Shape s = {&CIRCLE_VT, {.circle = {r}}};      return s; }
static Shape make_rect(double w, double h) { Shape s = {&RECT_VT, {.rect = {w, h}}}; return s; }
static Shape make_triangle(double a, double b, double c)
{ Shape s = {&TRIANGLE_VT, {.triangle = {a, b, c}}}; return s; }

/* ================================================================= *
 * 5. A STATE MACHINE built from function pointers
 * ================================================================= */
typedef enum { ST_START, ST_WORD, ST_NUMBER, ST_DONE, ST_COUNT } State;
typedef State (*StateFn)(char c, int *word_count, int *number_count);

static State st_start(char c, int *w, int *n)
{
    if (c == '\0') return ST_DONE;
    if (c >= '0' && c <= '9') { (*n)++; return ST_NUMBER; }
    if (c != ' ')             { (*w)++; return ST_WORD;   }
    return ST_START;
}
static State st_word(char c, int *w, int *n)
{
    (void)w; (void)n;
    if (c == '\0') return ST_DONE;
    return (c == ' ') ? ST_START : ST_WORD;
}
static State st_number(char c, int *w, int *n)
{
    (void)w; (void)n;
    if (c == '\0') return ST_DONE;
    return (c == ' ') ? ST_START : ST_NUMBER;
}
static const StateFn STATE_TABLE[ST_COUNT] = {
    [ST_START]  = st_start,
    [ST_WORD]   = st_word,
    [ST_NUMBER] = st_number,
    [ST_DONE]   = NULL,
};

static int cmp_int_asc (const void *a, const void *b)
{ int x = *(const int*)a, y = *(const int*)b; return (x > y) - (x < y); }
static int cmp_int_desc(const void *a, const void *b) { return cmp_int_asc(b, a); }
static int cmp_str(const void *a, const void *b)
{ return strcmp(*(const char *const *)a, *(const char *const *)b); }

int main(void)
{
    puts("=== 1. DECLARING AND CALLING ===");
    {
        int (*op)(int, int) = add;      /* `add` decays to &add automatically */
        printf("  int (*op)(int,int) = add;\n");
        printf("    op(7, 3)    = %d\n", op(7, 3));
        printf("    (*op)(7, 3) = %d   <- also legal; both forms mean the same\n", (*op)(7, 3));
        BinaryOp from_name = add, from_address = &add;
        printf("    `add` and `&add` give the same pointer: %s\n",
               (from_name == from_address) ? "yes" : "no");
        puts("    A function name decays to its address, exactly like an array name.");

        BinaryOp typed = mul;           /* far more readable with a typedef */
        printf("  BinaryOp typed = mul;  typed(7, 3) = %d\n", typed(7, 3));

        printf("  sizeof(BinaryOp) = %zu bytes\n", sizeof(BinaryOp));
        puts("  NOTE: converting between a function pointer and void* is NOT");
        puts("  guaranteed by ISO C (they may have different sizes). It works on");
        puts("  POSIX because dlsym requires it, but -Wpedantic will object.");
    }

    puts("\n=== 2. DISPATCH TABLE instead of if/else ===");
    {
        const char *exprs = "+-*/%";
        for (const char *c = exprs; *c; c++) {
            const Operation *op = lookup_op(*c);
            if (op) printf("  20 %c 6 = %-4d (%s)\n", *c, op->fn(20, 6), op->name);
            else    printf("  20 %c 6 -> unsupported operator\n", *c);
        }
        puts("  Adding an operator now means adding one row to the table.");
        puts("  No if/else chain to edit, and the table is const so it lives");
        puts("  in .rodata. With dense integer keys you can index it directly");
        puts("  for O(1) dispatch — that is how bytecode interpreters work.");
    }

    puts("\n=== 3. CALLBACKS WITH A void* CONTEXT ===");
    {
        int data[] = {4, 17, 8, 23, 15, 42, 9};
        size_t n = sizeof data / sizeof data[0];

        printf("  data: ");
        for_each(data, n, print_value, (void *)" ");
        puts("");

        long long total = 0;
        for_each(data, n, accumulate, &total);
        printf("  accumulate  -> sum = %lld\n", total);

        struct { int threshold; int count; } counter = {15, 0};
        for_each(data, n, count_over, &counter);
        printf("  count_over  -> %d values exceed %d\n", counter.count, counter.threshold);

        puts("  The SAME for_each drives three different behaviours. The void*");
        puts("  carries whatever state the callback needs — C has no closures,");
        puts("  so this parameter is not optional in any real API.");

        int out[16];
        size_t kept = filter(data, n, out, is_even, NULL);
        printf("  filter(is_even)  -> ");
        for (size_t i = 0; i < kept; i++) printf("%d ", out[i]);

        int limit = 15;
        kept = filter(data, n, out, exceeds, &limit);
        printf("\n  filter(> %d)      -> ", limit);
        for (size_t i = 0; i < kept; i++) printf("%d ", out[i]);

        int factor = 3;
        int copy[8]; memcpy(copy, data, sizeof data);
        map_in_place(copy, n, scale_by, &factor);
        printf("\n  map(* %d)         -> ", factor);
        for (size_t i = 0; i < n; i++) printf("%d ", copy[i]);
        puts("");
    }

    puts("\n=== 4. qsort: the standard library's callback API ===");
    {
        int a[] = {42, 7, 19, 3, 88, 15};
        size_t n = sizeof a / sizeof a[0];

        qsort(a, n, sizeof a[0], cmp_int_asc);
        printf("  ascending : ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);

        qsort(a, n, sizeof a[0], cmp_int_desc);
        printf("\n  descending: ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        puts("\n  cmp_int_desc is just cmp_int_asc with the arguments swapped.");

        /* Sorting an array of POINTERS: the comparator receives a pointer to
         * an ELEMENT, and each element is itself a char*. Hence char *const *. */
        const char *names[] = {"delta", "alpha", "charlie", "bravo"};
        qsort(names, 4, sizeof names[0], cmp_str);
        printf("  strings   : ");
        for (int i = 0; i < 4; i++) printf("%s ", names[i]);
        puts("\n  Note the comparator's argument type: const char *const *, not");
        puts("  const char *. qsort hands you the ADDRESS OF the element.");
    }

    puts("\n=== 5. A VTABLE: polymorphism in C ===");
    {
        Shape shapes[] = {
            make_circle(2.0),
            make_rect(3.0, 4.0),
            make_triangle(3.0, 4.0, 5.0),
        };
        size_t n = sizeof shapes / sizeof shapes[0];

        puts("  the same loop over three different types:");
        double total_area = 0;
        for (size_t i = 0; i < n; i++) {
            shapes[i].vt->describe(&shapes[i]);      /* virtual dispatch */
            total_area += shapes[i].vt->area(&shapes[i]);
        }
        printf("  total area: %.3f\n", total_area);
        printf("  each Shape is %zu bytes; the vtable is shared, not copied\n",
               sizeof(Shape));
        puts("  This IS how C++ virtual functions work: an object carries a");
        puts("  pointer to a per-CLASS table of function pointers, and a call");
        puts("  becomes 'load the table, load the slot, call it'.");
        puts("  generic_describe is shared by all three vtables — inheritance");
        puts("  of a default implementation, with no language support needed.");
    }

    puts("\n=== 6. A STATE MACHINE AS A TABLE OF FUNCTIONS ===");
    {
        const char *input = "hello 42 world 7 abc";
        State s = ST_START;
        int words = 0, numbers = 0;

        for (const char *c = input; ; c++) {
            if (s == ST_DONE) break;
            s = STATE_TABLE[s](*c, &words, &numbers);
            if (*c == '\0') break;
        }
        printf("  \"%s\"\n", input);
        printf("  -> %d words, %d numbers\n", words, numbers);
        puts("  Adding a state means adding a function and a table row. The");
        puts("  driver loop never changes. This scales far better than a");
        puts("  switch that grows a case per state per event.");
    }

    puts("\n=== SYNTAX REFERENCE ===");
    puts("  int (*f)(int, int);           f is a POINTER to a function");
    puts("  int *g(int, int);             g is a FUNCTION returning int*");
    puts("  int (*arr[10])(int);          an array of 10 function pointers");
    puts("  int (*(*h)(int))(double);     h returns a pointer to a function...");
    puts("  typedef int (*Op)(int,int);   ...just use a typedef. Always.");
    puts("");
    puts("  A NULL function pointer is a real hazard: calling it jumps to");
    puts("  address 0. Check before calling optional callbacks:");
    puts("      if (cb != NULL) cb(arg);");

    return 0;
}
