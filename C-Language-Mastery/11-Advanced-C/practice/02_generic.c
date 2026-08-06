/* 02_generic.c — _Generic (C11): compile-time type dispatch, at zero cost.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 02_generic.c -o t && ./t
 *
 * _Generic picks an expression based on the STATIC TYPE of a controlling
 * expression. It is resolved entirely at compile time — only the selected
 * branch is even compiled, so the others need not be valid for that type.
 *
 * This is C's answer to function overloading, and it is how <tgmath.h> makes
 * sqrt(x) dispatch to sqrtf, sqrt, or sqrtl.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ================================================================= *
 * 1. THE BASIC FORM
 * ================================================================= */
#define type_name(x) _Generic((x),          \
    _Bool:              "_Bool",            \
    char:               "char",             \
    signed char:        "signed char",      \
    unsigned char:      "unsigned char",    \
    short:              "short",            \
    unsigned short:     "unsigned short",   \
    int:                "int",              \
    unsigned int:       "unsigned int",     \
    long:               "long",             \
    unsigned long:      "unsigned long",    \
    long long:          "long long",        \
    unsigned long long: "unsigned long long",\
    float:              "float",            \
    double:             "double",           \
    long double:        "long double",      \
    char *:             "char *",           \
    const char *:       "const char *",     \
    void *:             "void *",           \
    int *:              "int *",            \
    default:            "unknown type")

/* ================================================================= *
 * 2. TYPE-SAFE OVERLOADING
 *
 * Three real functions, one name. The compiler picks; there is no runtime
 * dispatch, no function pointer, and no cost whatsoever.
 * ================================================================= */
static int    max_int   (int a, int b)       { return a > b ? a : b; }
static long   max_long  (long a, long b)     { return a > b ? a : b; }
static double max_double(double a, double b) { return a > b ? a : b; }

#define maximum(a, b) _Generic((a),   \
    int:    max_int,                  \
    long:   max_long,                 \
    double: max_double,               \
    float:  max_double)(a, b)

/* Contrast with the macro version, which evaluates its arguments twice. */
#define MAX_MACRO(a, b) ((a) > (b) ? (a) : (b))

/* ================================================================= *
 * 3. A TYPE-AWARE PRINT — the printf format chosen automatically
 * ================================================================= */
static void print_int    (int v)         { printf("%d",   v); }
static void print_uint   (unsigned v)    { printf("%u",   v); }
static void print_long   (long v)        { printf("%ld",  v); }
static void print_double (double v)      { printf("%g",   v); }
static void print_string (const char *v) { printf("\"%s\"", v); }
static void print_char   (char v)        { printf("'%c'", v); }
static void print_bool   (bool v)        { printf("%s", v ? "true" : "false"); }
static void print_ptr    (const void *v) { printf("%p", v); }

#define print_value(x) _Generic((x),  \
    bool:         print_bool,         \
    char:         print_char,         \
    int:          print_int,          \
    unsigned:     print_uint,         \
    long:         print_long,         \
    float:        print_double,       \
    double:       print_double,       \
    char *:       print_string,       \
    const char *: print_string,       \
    default:      print_ptr)(x)

#define println(x) do { print_value(x); putchar('\n'); } while (0)

/* ================================================================= *
 * 4. TYPE-DIRECTED BEHAVIOUR: the right absolute value, the right epsilon
 * ================================================================= */
#define absolute(x) _Generic((x),  \
    int:         abs,              \
    long:        labs,             \
    long long:   llabs,            \
    float:       fabsf,            \
    double:      fabs,             \
    long double: fabsl)(x)

/* Comparing floats needs a tolerance, and the right tolerance depends on
 * the type's precision. _Generic picks it. */
static bool near_float (float a, float b)  { return fabsf(a - b) < 1e-5f;  }
static bool near_double(double a, double b){ return fabs (a - b) < 1e-12;  }
static bool eq_int     (int a, int b)      { return a == b; }

#define nearly_equal(a, b) _Generic((a),  \
    float:  near_float,                   \
    double: near_double,                  \
    int:    eq_int)(a, b)

/* ================================================================= *
 * 5. A GENERIC SERIALISER — dispatch to the right size and format
 * ================================================================= */
#define type_size_info(x) _Generic((x),                                    \
    int8_t:   "int8_t   (1 byte,  -128..127)",                             \
    int16_t:  "int16_t  (2 bytes, -32768..32767)",                         \
    int32_t:  "int32_t  (4 bytes, +/-2.1e9)",                              \
    int64_t:  "int64_t  (8 bytes, +/-9.2e18)",                             \
    float:    "float    (4 bytes, ~7 digits)",                             \
    double:   "double   (8 bytes, ~16 digits)",                            \
    default:  "something else")

int main(void)
{
    puts("=== 1. WHAT _Generic DOES ===");
    puts("  It selects an expression based on the STATIC TYPE of its");
    puts("  controlling expression. Everything happens at COMPILE time.\n");
    {
        int          i = 1;
        unsigned     u = 1u;
        long         l = 1L;
        double       d = 1.0;
        float        f = 1.0f;
        char         c = 'x';
        char        *s = "text";
        const char  *cs = "const text";
        int         *pi = &i;
        void        *pv = &i;
        bool         b = true;

        printf("  %-14s -> %s\n", "1",           type_name(i));
        printf("  %-14s -> %s\n", "1u",          type_name(u));
        printf("  %-14s -> %s\n", "1L",          type_name(l));
        printf("  %-14s -> %s\n", "1.0",         type_name(d));
        printf("  %-14s -> %s\n", "1.0f",        type_name(f));
        printf("  %-14s -> %s\n", "'x'",         type_name(c));
        printf("  %-14s -> %s\n", "char *",      type_name(s));
        printf("  %-14s -> %s\n", "const char *",type_name(cs));
        printf("  %-14s -> %s\n", "int *",       type_name(pi));
        printf("  %-14s -> %s\n", "void *",      type_name(pv));
        printf("  %-14s -> %s\n", "bool",        type_name(b));

        puts("\n  GOTCHAS:");
        printf("    a string LITERAL has type char[N], which DECAYS to char *:\n");
        printf("      type_name(\"literal\") -> %s\n", type_name("literal"));
        printf("    an ARRAY also decays before matching:\n");
        int arr[10];
        printf("      type_name(int[10]) -> %s\n", type_name(arr));
        puts("    const qualifiers must be listed SEPARATELY: `char *` does");
        puts("    NOT match a `const char *` argument.");
        puts("    Only the SELECTED branch is compiled, so the others need not");
        puts("    even be valid for the argument's type.");
    }

    puts("\n=== 2. TYPE-SAFE OVERLOADING ===");
    {
        printf("  maximum(3, 7)         = %d      (dispatched to max_int)\n",
               maximum(3, 7));
        printf("  maximum(3L, 7L)       = %ld      (dispatched to max_long)\n",
               maximum(3L, 7L));
        printf("  maximum(3.5, 7.25)    = %g   (dispatched to max_double)\n",
               maximum(3.5, 7.25));

        puts("\n  vs the macro version:");
        int i = 5, j = 3;
        int r1 = maximum(i++, j);
        printf("    maximum(i++, j)   = %d, i is now %d   <- evaluated ONCE\n", r1, i);
        i = 5;
        int r2 = MAX_MACRO(i++, j);
        printf("    MAX_MACRO(i++, j) = %d, i is now %d   <- evaluated TWICE\n", r2, i);
        puts("");
        puts("  _Generic gives you real FUNCTIONS: single evaluation, type");
        puts("  checking on the parameters, a symbol the debugger can see, and");
        puts("  zero runtime cost — the selection happens at compile time.");
        puts("  This is strictly better than a function-like macro whenever the");
        puts("  set of types is known.");
    }

    puts("\n=== 3. A TYPE-AWARE print ===");
    {
        printf("  println(42)        -> "); println(42);
        printf("  println(42u)       -> "); println(42u);
        printf("  println(42L)       -> "); println(42L);
        printf("  println(3.14)      -> "); println(3.14);
        printf("  println(2.5f)      -> "); println(2.5f);
        printf("  println('A')       -> "); println('A');
        printf("  println(\"hello\")   -> "); println("hello");
        printf("  println(true)      -> "); println(true);
        int x = 0;
        printf("  println(&x)        -> "); println(&x);
        puts("");
        puts("  No format specifier to get wrong. printf's %d-with-a-double bug");
        puts("  (undefined behaviour, module 01) becomes impossible here, because");
        puts("  the format is chosen FROM the type rather than asserted by you.");
    }

    puts("\n=== 4. TYPE-DIRECTED BEHAVIOUR ===");
    {
        printf("  absolute(-5)      = %d\n",  absolute(-5));
        printf("  absolute(-5L)     = %ld\n", absolute(-5L));
        printf("  absolute(-5.5)    = %g\n",  absolute(-5.5));
        printf("  absolute(-5.5f)   = %g\n",  (double)absolute(-5.5f));
        puts("    Each dispatches to abs / labs / fabs / fabsf. Calling abs() on");
        puts("    a double silently truncates it — a real and common bug that");
        puts("    this construction makes impossible.");

        puts("\n  comparison with a type-appropriate tolerance:");
        printf("    nearly_equal(0.1f + 0.2f, 0.3f) = %s   (float, 1e-5 tolerance)\n",
               nearly_equal(0.1f + 0.2f, 0.3f) ? "true" : "false");
        printf("    nearly_equal(0.1 + 0.2, 0.3)    = %s   (double, 1e-12)\n",
               nearly_equal(0.1 + 0.2, 0.3) ? "true" : "false");
        printf("    nearly_equal(3, 3)              = %s   (int, exact ==)\n",
               nearly_equal(3, 3) ? "true" : "false");
        puts("    0.1 + 0.2 != 0.3 exactly (module 15 explains why), so floats");
        puts("    need a tolerance — and the RIGHT tolerance depends on the");
        puts("    type's precision. _Generic picks it for you.");
    }

    puts("\n=== 5. SIZE AND RANGE INFORMATION ===");
    {
        int8_t  a = 0; int16_t b = 0; int32_t c = 0; int64_t d = 0;
        float   e = 0; double  f = 0;
        printf("  %s\n", type_size_info(a));
        printf("  %s\n", type_size_info(b));
        printf("  %s\n", type_size_info(c));
        printf("  %s\n", type_size_info(d));
        printf("  %s\n", type_size_info(e));
        printf("  %s\n", type_size_info(f));
        puts("  Note int8_t is usually `signed char` and int32_t is usually");
        puts("  `int`, so listing BOTH in one _Generic is a duplicate-case");
        puts("  compile error. Typedefs are aliases, not distinct types.");
    }

    puts("\n=== WHERE _Generic IS ALREADY USED ===");
    puts("  <tgmath.h> is built entirely on it. That is how");
    puts("      sqrt(x)");
    puts("  calls sqrtf for a float, sqrt for a double, and sqrtl for a long");
    puts("  double — one name, three functions, chosen at compile time.");
    puts("");
    puts("  C11 also uses it for atomic_load, atomic_store, and friends in");
    puts("  <stdatomic.h>.");

    puts("\n=== LIMITS ===");
    puts("  - the type set must be KNOWN AT COMPILE TIME; it cannot dispatch on");
    puts("    a runtime value (that is what function pointers are for)");
    puts("  - it dispatches on ONE expression; multi-argument overloading needs");
    puts("    nested _Generics, which get ugly fast");
    puts("  - arrays decay and string literals become char*, so those cases need");
    puts("    care");
    puts("  - a type not listed and no `default` is a COMPILE ERROR — which is");
    puts("    usually what you want");
    puts("  - it does not generate CONTAINERS; for that you still need macro");
    puts("    templates or void* (module 09, file 12)");
    puts("");
    puts("  Use it for: overloaded math and utility functions, type-safe print");
    puts("  and compare helpers, and picking the right constant or tolerance");
    puts("  per type. It is the cleanest thing C has for this and it costs");
    puts("  nothing at run time.");

    return 0;
}
