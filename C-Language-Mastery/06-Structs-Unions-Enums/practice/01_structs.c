/* 01_structs.c — declaring, initialising, copying, and nesting your own types.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_structs.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ---- the three declaration forms ---- */

/* 1. A tagged struct. The type's name is `struct Point`. */
struct PointTagged { int x, y; };

/* 2. typedef of an anonymous struct. Short, but it CANNOT refer to itself,
 *    so it is useless for linked structures. */
typedef struct { int x, y; } PointAnon;

/* 3. Tagged AND typedef'd — the form to prefer. Usable as both `struct Point`
 *    and `Point`, and self-referential. Every list/tree node needs this. */
typedef struct Point { int x, y; } Point;

typedef struct Node {
    int          value;
    struct Node *next;      /* needs the TAG: `Node *next` is not yet in scope */
} Node;

/* ---- nesting ---- */
typedef struct { double x, y; }        Vec2;
typedef struct { Vec2 min, max; }      Rect;      /* by value: one flat object */
typedef struct {
    char   name[32];        /* an inline array — copied with the struct       */
    char  *description;     /* a POINTER — the struct copy SHARES the target  */
    Rect   bounds;
    int    id;
} Entity;

/* Anonymous struct/union members (C11): access the inner fields directly. */
typedef struct {
    int kind;
    struct { double x, y, z; };      /* no member name — v.x, not v.pos.x */
} Vertex;

static void print_point(const char *label, Point p)
{
    printf("  %-28s (%d, %d)\n", label, p.x, p.y);
}

/* Structs can be RETURNED by value. At -O2 small ones come back in registers,
 * so this is free — no hidden allocation, unlike returning a pointer. */
static Point point_add(Point a, Point b) { return (Point){a.x + b.x, a.y + b.y}; }

/* Arrays cannot be assigned or returned. WRAP THEM IN A STRUCT and they can. */
typedef struct { int v[4]; } Vec4;
static Vec4 vec4_scale(Vec4 a, int k)
{
    for (int i = 0; i < 4; i++) a.v[i] *= k;    /* `a` is our own copy */
    return a;
}

int main(void)
{
    puts("=== DECLARATION FORMS ===");
    {
        struct PointTagged a = {1, 2};
        PointAnon          b = {3, 4};
        Point              c = {5, 6};
        struct Point       d = {7, 8};        /* the same type as c */

        printf("  struct PointTagged a = {1,2}  -> (%d,%d)\n", a.x, a.y);
        printf("  PointAnon          b = {3,4}  -> (%d,%d)\n", b.x, b.y);
        printf("  Point              c = {5,6}  -> (%d,%d)\n", c.x, c.y);
        printf("  struct Point       d = {7,8}  -> (%d,%d)  (same type as c)\n", d.x, d.y);
        puts("  Prefer `typedef struct Point { ... } Point;` — it gives you both");
        puts("  spellings and it can refer to itself, which linked structures need.");
    }

    puts("\n=== INITIALISATION ===");
    {
        Point positional = {1, 2};
        Point designated = {.y = 20, .x = 10};   /* C99 — order does not matter */
        Point partial    = {.x = 5};             /* y is ZERO, guaranteed */
        Point zeroed     = {0};                  /* the "all zero" idiom */
        Point compound   = (Point){99, 99};      /* a compound literal (C99) */

        print_point("positional {1, 2}",        positional);
        print_point("designated {.y=20,.x=10}", designated);
        print_point("partial    {.x=5}",        partial);
        print_point("zeroed     {0}",           zeroed);
        print_point("compound   (Point){99,99}",compound);

        puts("  Any member you omit is ZERO-initialised — guaranteed by the standard.");
        puts("  PREFER DESIGNATED INITIALISERS: they survive field reordering, they");
        puts("  make omissions visible, and they document what each value means.");
        puts("  A compound literal (Point){x,y} is an unnamed object you can pass");
        puts("  straight to a function or assign — very handy.");
    }

    puts("\n=== STRUCTS ARE ASSIGNABLE AND COPYABLE (arrays are not) ===");
    {
        Point a = {1, 2};
        Point b = a;              /* a full member-wise copy */
        b.x = 99;
        printf("  a = (%d,%d), b = a then b.x = 99 -> b = (%d,%d)\n", a.x, a.y, b.x, b.y);
        printf("  a is untouched: independent objects\n");

        Point sum = point_add(a, b);
        printf("  point_add returns a struct BY VALUE -> (%d,%d)\n", sum.x, sum.y);

        Vec4 v = {{1, 2, 3, 4}};
        Vec4 scaled = vec4_scale(v, 10);
        printf("  Vec4 wraps an array so it CAN be copied and returned: ");
        for (int i = 0; i < 4; i++) printf("%d ", scaled.v[i]);
        printf("(original untouched: ");
        for (int i = 0; i < 4; i++) printf("%d ", v.v[i]);
        puts(")");
        puts("  A bare `int a[4]` cannot be assigned, passed by value, or returned.");
        puts("  Wrapping it in a struct is the standard workaround.");
    }

    puts("\n=== SHALLOW COPY: the trap with pointer members ===");
    {
        Entity e1 = {0};
        strcpy(e1.name, "original");
        e1.description = malloc(64);
        strcpy(e1.description, "shared heap text");
        e1.bounds = (Rect){{0, 0}, {10, 10}};
        e1.id = 1;

        Entity e2 = e1;                 /* member-wise copy */
        strcpy(e2.name, "copy");        /* name is an ARRAY -> copied, independent */
        e2.description[0] = 'S';        /* description is a POINTER -> SHARED! */

        printf("  e1.name = \"%s\", e2.name = \"%s\"   <- arrays are copied\n",
               e1.name, e2.name);
        printf("  e1.description = \"%s\"\n", e1.description);
        printf("  e2.description = \"%s\"   <- SAME buffer, both changed\n", e2.description);
        printf("  e1.description == e2.description ? %s\n",
               e1.description == e2.description ? "YES — they alias" : "no");
        puts("  `=` on a struct is a SHALLOW copy: it duplicates the pointer, not");
        puts("  the pointee. Two owners of one allocation is a double-free waiting");
        puts("  to happen. If a struct owns heap memory, write a deep entity_copy().");
        free(e1.description);           /* free ONCE, not twice */
    }

    puts("\n=== NESTING ===");
    {
        Entity e = {
            .name  = "player",
            .id    = 42,
            .bounds = { .min = {0.0, 0.0}, .max = {100.0, 50.0} },
        };
        printf("  e.name = %s, e.id = %d\n", e.name, e.id);
        printf("  e.bounds.min = (%.1f, %.1f), e.bounds.max = (%.1f, %.1f)\n",
               e.bounds.min.x, e.bounds.min.y, e.bounds.max.x, e.bounds.max.y);
        printf("  e.description = %p (omitted -> NULL, guaranteed)\n",
               (void *)e.description);
        printf("  sizeof(Entity) = %zu bytes, all inline — ONE object, no pointers\n",
               sizeof e);
        puts("  Nesting BY VALUE keeps everything in one contiguous block: one");
        puts("  allocation, one free, and no indirection when you read a field.");
    }

    puts("\n=== ANONYMOUS MEMBERS (C11) ===");
    {
        Vertex v = { .kind = 1, .x = 1.0, .y = 2.0, .z = 3.0 };
        printf("  v.x = %.1f, v.y = %.1f, v.z = %.1f  <- no intermediate name\n",
               v.x, v.y, v.z);
        puts("  An unnamed struct or union member's fields are promoted into the");
        puts("  parent's namespace. Useful for tagged unions (see 03_unions_and_enums.c).");
    }

    puts("\n=== ARROW vs DOT ===");
    {
        Point  p  = {1, 2};
        Point *pp = &p;
        printf("  p.x   = %d   (a struct)\n", p.x);
        printf("  pp->x = %d   (through a pointer)\n", pp->x);
        printf("  (*pp).x = %d (exactly what -> means)\n", (*pp).x);
        puts("  -> exists because (*p).x needs parentheses: . binds tighter than *,");
        puts("  so *p.x would parse as *(p.x). The arrow removes the trap.");
    }

    puts("\n=== ARRAYS OF STRUCTS vs STRUCT OF ARRAYS ===");
    {
        /* AoS — Array of Structs. Natural, and right when you touch whole objects. */
        typedef struct { float x, y, z; int id; } ParticleAoS;
        ParticleAoS aos[4] = {{1,1,1,1},{2,2,2,2},{3,3,3,3},{4,4,4,4}};

        /* SoA — Struct of Arrays. Right when you touch ONE field across many
         * objects: the field is then contiguous and SIMD-friendly. */
        struct { float x[4], y[4], z[4]; int id[4]; } soa = {
            {1,2,3,4}, {1,2,3,4}, {1,2,3,4}, {1,2,3,4}
        };

        float sum_aos = 0, sum_soa = 0;
        for (int i = 0; i < 4; i++) sum_aos += aos[i].x;   /* strides by 16 bytes */
        for (int i = 0; i < 4; i++) sum_soa += soa.x[i];   /* strides by 4 bytes  */
        printf("  AoS: sum of x = %.0f  (each x is %zu bytes from the last)\n",
               sum_aos, sizeof(ParticleAoS));
        printf("  SoA: sum of x = %.0f  (each x is %zu bytes from the last)\n",
               sum_soa, sizeof(float));
        puts("  Summing one field: SoA reads 4 useful floats per cache line, AoS");
        puts("  reads 1 useful float and 12 wasted bytes. SoA also vectorises.");
        puts("  Touching whole objects: AoS wins, one cache line per object.");
        puts("  Choose by ACCESS PATTERN. This is the core idea behind");
        puts("  data-oriented design. Module 13 measures the difference.");
    }

    puts("\n=== A SELF-REFERENTIAL STRUCT ===");
    {
        /* This is why you need the TAG form: `struct Node *next` must name a
         * type that does not exist yet. `Node *next` would not compile. */
        Node c = {3, NULL};
        Node b = {2, &c};
        Node a = {1, &b};
        printf("  list: ");
        for (Node *n = &a; n != NULL; n = n->next) printf("%d -> ", n->value);
        puts("NULL");
        puts("  A struct may contain a POINTER to its own type, but never an");
        puts("  INSTANCE of it — that would need infinite size. This one rule");
        puts("  is why every linked structure in C is built from pointers.");
    }

    return 0;
}
