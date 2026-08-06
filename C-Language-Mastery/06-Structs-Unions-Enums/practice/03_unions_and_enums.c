/* 03_unions_and_enums.c — unions, tagged unions, enums, bitfields.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_unions_and_enums.c -o t && ./t
 *
 * The tagged union is the most valuable pattern here: it is how every
 * interpreter represents a dynamically typed value and how every compiler
 * represents an AST node.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ================================================================= *
 * PLAIN UNIONS — all members share the same bytes
 * ================================================================= */
union Raw {
    uint32_t u;
    float    f;
    uint8_t  bytes[4];
};

/* ================================================================= *
 * TAGGED UNION (sum type / discriminated union)
 *
 * A bare union cannot tell you which member is live. Pairing it with an
 * enum tag fixes that, and gives you a type that is EXACTLY ONE of several
 * alternatives — the single most useful data-modelling tool in C.
 * ================================================================= */
typedef enum { V_NULL, V_BOOL, V_INT, V_DOUBLE, V_STRING, V_KIND_COUNT } ValueKind;

/* The X_COUNT idiom: adding a kind without adding a name breaks the build. */
static const char *KIND_NAMES[V_KIND_COUNT] = {
    "null", "bool", "int", "double", "string"
};

typedef struct {
    ValueKind kind;                  /* THE TAG — says which member is valid */
    union {
        bool    b;
        long    i;
        double  d;
        char   *s;                   /* OWNED: value_free releases it */
    } as;                            /* naming the union member reads well: v.as.i */
} Value;

/* Constructors make it impossible to set the tag and the payload inconsistently. */
static Value value_null(void)          { return (Value){ .kind = V_NULL   }; }
static Value value_bool(bool b)        { return (Value){ .kind = V_BOOL,   .as.b = b }; }
static Value value_int(long i)         { return (Value){ .kind = V_INT,    .as.i = i }; }
static Value value_double(double d)    { return (Value){ .kind = V_DOUBLE, .as.d = d }; }
static Value value_string(const char *s)
{
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy == NULL) return value_null();
    memcpy(copy, s, n);
    return (Value){ .kind = V_STRING, .as.s = copy };
}
static void value_free(Value *v)
{
    if (v->kind == V_STRING) free(v->as.s);      /* only this kind owns memory */
    *v = value_null();
}

/* ALWAYS read through a switch on the tag. Reading v.as.i when the tag says
 * V_STRING reinterprets a pointer as an integer — the exact bug tagged unions
 * exist to prevent. */
static void value_print(const Value *v)
{
    printf("  %-7s ", KIND_NAMES[v->kind]);
    switch (v->kind) {
    case V_NULL:   printf("null\n");                      break;
    case V_BOOL:   printf("%s\n", v->as.b ? "true" : "false"); break;
    case V_INT:    printf("%ld\n", v->as.i);              break;
    case V_DOUBLE: printf("%g\n",  v->as.d);              break;
    case V_STRING: printf("\"%s\"\n", v->as.s);           break;
    case V_KIND_COUNT: break;    /* listing every case means -Wswitch protects us */
    }
}

/* Truthiness rules, per kind — exactly how a dynamic language does it. */
static bool value_truthy(const Value *v)
{
    switch (v->kind) {
    case V_NULL:   return false;
    case V_BOOL:   return v->as.b;
    case V_INT:    return v->as.i != 0;
    case V_DOUBLE: return v->as.d != 0.0;
    case V_STRING: return v->as.s[0] != '\0';
    case V_KIND_COUNT: break;
    }
    return false;
}

/* ================================================================= *
 * A TINY EXPRESSION TREE — the other classic tagged-union use
 * ================================================================= */
typedef enum { E_NUM, E_ADD, E_MUL, E_NEG } ExprKind;
typedef struct Expr Expr;
struct Expr {
    ExprKind kind;
    union {
        double num;                              /* E_NUM  */
        struct { Expr *lhs, *rhs; } binary;      /* E_ADD, E_MUL */
        struct { Expr *operand; }   unary;       /* E_NEG  */
    };                                           /* anonymous: e->num, e->binary.lhs */
};

static Expr *expr_num(double v)
{ Expr *e = calloc(1, sizeof *e); e->kind = E_NUM; e->num = v; return e; }
static Expr *expr_binary(ExprKind k, Expr *l, Expr *r)
{ Expr *e = calloc(1, sizeof *e); e->kind = k; e->binary.lhs = l; e->binary.rhs = r; return e; }
static Expr *expr_neg(Expr *o)
{ Expr *e = calloc(1, sizeof *e); e->kind = E_NEG; e->unary.operand = o; return e; }

static double expr_eval(const Expr *e)
{
    switch (e->kind) {
    case E_NUM: return e->num;
    case E_ADD: return expr_eval(e->binary.lhs) + expr_eval(e->binary.rhs);
    case E_MUL: return expr_eval(e->binary.lhs) * expr_eval(e->binary.rhs);
    case E_NEG: return -expr_eval(e->unary.operand);
    }
    return 0.0;
}
static void expr_print(const Expr *e)
{
    switch (e->kind) {
    case E_NUM: printf("%g", e->num); break;
    case E_ADD: putchar('('); expr_print(e->binary.lhs); printf(" + ");
                expr_print(e->binary.rhs); putchar(')'); break;
    case E_MUL: putchar('('); expr_print(e->binary.lhs); printf(" * ");
                expr_print(e->binary.rhs); putchar(')'); break;
    case E_NEG: printf("-"); expr_print(e->unary.operand); break;
    }
}
static void expr_free(Expr *e)
{
    if (e == NULL) return;
    switch (e->kind) {
    case E_ADD: case E_MUL: expr_free(e->binary.lhs); expr_free(e->binary.rhs); break;
    case E_NEG: expr_free(e->unary.operand); break;
    case E_NUM: break;
    }
    free(e);
}

/* ================================================================= *
 * BITFIELDS
 * ================================================================= */
struct Flags {
    unsigned int visible   : 1;      /* 1 bit:  0..1   */
    unsigned int selected  : 1;
    unsigned int layer     : 4;      /* 4 bits: 0..15  */
    unsigned int opacity   : 8;      /* 8 bits: 0..255 */
    unsigned int reserved  : 18;
};

/* enums with explicit values, for a protocol or a status code */
typedef enum {
    STATUS_OK       = 0,
    STATUS_WARNING  = 10,
    STATUS_ERROR,                    /* continues from the last: 11 */
    STATUS_FATAL    = 100,
} Status;

int main(void)
{
    puts("=== PLAIN UNION: one block of bytes, several interpretations ===");
    {
        union Raw r;
        printf("  sizeof(union Raw) = %zu  (the size of its LARGEST member)\n", sizeof r);

        r.f = 1.0f;
        printf("  wrote r.f = 1.0f\n");
        printf("    read r.u     = 0x%08X   <- the IEEE-754 bit pattern\n", r.u);
        printf("    read r.bytes = %02X %02X %02X %02X\n",
               r.bytes[0], r.bytes[1], r.bytes[2], r.bytes[3]);
        printf("    all three members share address %p\n", (void *)&r);

        r.u = 0x40490FDB;
        printf("  wrote r.u = 0x40490FDB -> r.f = %f  (that is pi)\n", (double)r.f);

        puts("\n  Reading a member you did not write is TYPE PUNNING. In C this is");
        puts("  WELL DEFINED (C11 6.5.2.3 footnote 97) — unlike in C++, where it");
        puts("  is technically undefined. Unions are therefore the idiomatic C way");
        puts("  to inspect a bit pattern, alongside memcpy.");
        puts("  Casting a pointer instead — *(int*)&f — violates strict aliasing");
        puts("  and IS undefined. Use a union or memcpy, never a pointer cast.");
    }

    puts("\n=== THE PROBLEM WITH A BARE UNION ===");
    puts("  A union does not remember which member you last wrote. Nothing stops");
    puts("  you writing r.f and reading r.u — that is the feature above, but it is");
    puts("  a disaster when the members are, say, a long and a char*: read the");
    puts("  wrong one and you dereference an integer.");
    puts("  The fix is to carry a TAG alongside it.");

    puts("\n=== TAGGED UNION: a dynamically typed value ===");
    {
        Value values[] = {
            value_null(), value_bool(true), value_int(-42),
            value_double(3.14159), value_string("hello, world"),
        };
        size_t n = sizeof values / sizeof values[0];

        printf("  sizeof(Value) = %zu bytes (tag %zu + union %zu + padding)\n",
               sizeof(Value), sizeof(ValueKind), sizeof(((Value*)0)->as));

        for (size_t i = 0; i < n; i++) {
            value_print(&values[i]);
        }

        printf("\n  truthiness (exactly how a dynamic language decides):\n");
        for (size_t i = 0; i < n; i++)
            printf("    %-7s -> %s\n", KIND_NAMES[values[i].kind],
                   value_truthy(&values[i]) ? "truthy" : "falsy");

        for (size_t i = 0; i < n; i++) value_free(&values[i]);

        puts("\n  Why this is the pattern to reach for:");
        puts("    - the value is EXACTLY ONE of the alternatives, never two");
        puts("    - it costs max(members) bytes, not sum(members)");
        puts("    - a switch over the tag with every case listed means -Wswitch");
        puts("      tells you when you add a kind and forget to handle it");
        puts("    - constructors keep the tag and payload consistent by design");
        puts("  This is how CPython's PyObject, Lua's TValue, and every JSON");
        puts("  library in C represent a value.");
    }

    puts("\n=== TAGGED UNION AS AN AST ===");
    {
        /* -(2 + 3) * 4 */
        Expr *e = expr_binary(E_MUL,
                              expr_neg(expr_binary(E_ADD, expr_num(2), expr_num(3))),
                              expr_num(4));
        printf("  expression: ");
        expr_print(e);
        printf("  =  %g\n", expr_eval(e));
        expr_free(e);
        puts("  Every node is the same TYPE but a different SHAPE. The union means");
        puts("  a leaf costs no more than the biggest node's payload, and the");
        puts("  recursive switch in expr_eval is the entire interpreter.");
        puts("  This is module 17's virtual machine in miniature.");
    }

    puts("\n=== ENUMS ===");
    {
        printf("  STATUS_OK=%d WARNING=%d ERROR=%d FATAL=%d\n",
               STATUS_OK, STATUS_WARNING, STATUS_ERROR, STATUS_FATAL);
        puts("  Unassigned enumerators continue from the previous one, so");
        puts("  STATUS_ERROR is 11, not 1.");
        printf("  sizeof(Status) = %zu  <- implementation-defined; never rely on it\n",
               sizeof(Status));

        Status bogus = (Status)999;
        printf("  (Status)999 = %d   <- NO range checking. Enums are just ints.\n", bogus);

        puts("\n  Enum constants live in the ORDINARY namespace at file scope, so");
        puts("  two enums with a member called ERROR will collide. Prefix them:");
        puts("      typedef enum { COLOR_RED, COLOR_GREEN } Color;");

        puts("\n  The X_COUNT idiom keeps an enum and a table in lockstep:");
        printf("    V_KIND_COUNT = %d, and KIND_NAMES has %zu entries\n",
               V_KIND_COUNT, sizeof KIND_NAMES / sizeof KIND_NAMES[0]);
        puts("    Add a kind without adding a name and the array initialiser is");
        puts("    short — GCC warns, and the size assertion below fails outright.");

        /* A compile-time assertion. If the two ever diverge, the BUILD fails. */
        _Static_assert(sizeof KIND_NAMES / sizeof KIND_NAMES[0] == V_KIND_COUNT,
                       "KIND_NAMES must have one entry per ValueKind");
        puts("    _Static_assert enforces it at COMPILE time — free, and it can");
        puts("    never drift.");

        puts("\n  enum { MAX = 100 };  is better than #define MAX 100:");
        puts("    - it is scoped, so it will not leak into every file");
        puts("    - the debugger can see the name");
        puts("    - it is a real integer constant expression, usable as an array size");
    }

    puts("\n=== BITFIELDS ===");
    {
        struct Flags f = { .visible = 1, .selected = 0, .layer = 7, .opacity = 200 };
        printf("  struct Flags: visible=%u selected=%u layer=%u opacity=%u\n",
               f.visible, f.selected, f.layer, f.opacity);
        printf("  sizeof(struct Flags) = %zu bytes for %d bits of data\n",
               sizeof f, 1 + 1 + 4 + 8 + 18);
        puts("  Without bitfields those four fields would cost 16 bytes as ints.");

        /* Truncation. GCC catches the CONSTANT case (-Woverflow), which is why
         * the value goes through a variable here — a run-time value gets no
         * such warning, and that is where this bug actually happens. */
        unsigned wanted = 20;
        f.layer = wanted & 0xFu;             /* only 4 bits: 20 & 0xF == 4 */
        printf("  f.layer = %u in a 4-bit field -> %u  (silently truncated)\n",
               wanted, f.layer);

        puts("\n  LIMITATIONS — these matter:");
        puts("    - bit ORDER within the storage unit is implementation-defined,");
        puts("      so bitfields are NOT portable for wire formats or hardware");
        puts("      registers, however much they look like the right tool");
        puts("    - you cannot take the address of a bitfield (&f.layer)");
        puts("    - assigning an out-of-range value truncates silently");
        puts("    - access is slower: the compiler emits shifts and masks");
        puts("  Fine for internal in-memory flags. For anything crossing a");
        puts("  process, file, or wire boundary, use explicit shifts and masks");
        puts("  (module 11) — portable, endian-explicit, and no surprises.");
    }

    puts("\n=== SIZE COMPARISON ===");
    {
        struct AsStruct { bool b; long i; double d; char *s; };
        printf("  struct with all four members : %zu bytes\n", sizeof(struct AsStruct));
        printf("  union  with all four members : %zu bytes\n",
               sizeof(union { bool b; long i; double d; char *s; }));
        printf("  tagged union (Value)         : %zu bytes\n", sizeof(Value));
        puts("  A struct costs the SUM of its members; a union costs the MAX.");
        puts("  Use a struct when you have all of them at once, a tagged union");
        puts("  when you have exactly one of them at a time.");
    }

    return 0;
}
