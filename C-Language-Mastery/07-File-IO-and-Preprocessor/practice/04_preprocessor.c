/* 04_preprocessor.c — macros: every pitfall, and the patterns worth keeping.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 04_preprocessor.c -o t && ./t
 *
 * See what the preprocessor actually produced:
 *   gcc -E 04_preprocessor.c | grep -A2 'BROKEN_SQUARE result'
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ================================================================= *
 * PITFALL 1: unparenthesised PARAMETERS
 * ================================================================= */
#define BROKEN_SQUARE(x)  x * x            /* BROKEN */
#define OK_SQUARE(x)      ((x) * (x))      /* correct */

/* ================================================================= *
 * PITFALL 2: unparenthesised BODY
 * ================================================================= */
#define BROKEN_DOUBLE(x)  (x) * 2          /* BROKEN */
#define OK_DOUBLE(x)      ((x) * 2)        /* correct */

/* ================================================================= *
 * PITFALL 3: an argument used more than once
 * ================================================================= */
#define MAX_MACRO(a, b)   ((a) > (b) ? (a) : (b))   /* still evaluates twice */
static inline int max_i(int a, int b) { return a > b ? a : b; }   /* the fix */

/* ================================================================= *
 * PITFALL 4: multiple statements
 * ================================================================= */
#define BROKEN_SWAP(a, b) { int t = (a); (a) = (b); (b) = t; }
#define OK_SWAP(a, b)     do { int t = (a); (a) = (b); (b) = t; } while (0)

/* ================================================================= *
 * STRINGIFY (#) AND TOKEN PASTE (##)
 * ================================================================= */
#define STR(x)      #x            /* does NOT expand x first */
#define XSTR(x)     STR(x)        /* expands x, THEN stringifies */
#define CAT(a, b)   a##b
#define XCAT(a, b)  CAT(a, b)

#define VERSION 42

/* Generate a family of near-identical functions from one template. This is
 * genuine C generics, and it is what module 09's containers use. */
#define DEFINE_MIN(TYPE, SUFFIX) \
    static TYPE min_##SUFFIX(TYPE a, TYPE b) { return a < b ? a : b; }
DEFINE_MIN(int,    i)
DEFINE_MIN(double, d)
DEFINE_MIN(long,   l)

/* ================================================================= *
 * USEFUL MACROS YOU SHOULD ACTUALLY WRITE
 * ================================================================= */

/* Array length — only valid where the ARRAY is in scope, never on a
 * parameter. The comma trick makes it a compile error if you pass a pointer
 * (sizeof(void*) is not divisible by sizeof(*p) in general... but the honest
 * answer is: rely on -Wsizeof-pointer-div, which GCC and Clang both have). */
#define ARRAY_LEN(a)  (sizeof(a) / sizeof((a)[0]))

/* Debug logging that vanishes entirely in a release build. Note the do/while
 * and the ##__VA_ARGS__ GNU extension for the zero-argument case.
 * (C23 has __VA_OPT__ for this, portably.) */
#ifdef NDEBUG
#  define LOG(fmt, ...) ((void)0)
#else
#  define LOG(fmt, ...) \
      fprintf(stderr, "[%s:%d %s] " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

/* A compile-time assertion. Costs nothing and can never drift. */
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)

/* Compile-time-checked bounds. */
#define CLAMP(v, lo, hi)  ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/* Mark a parameter deliberately unused, silencing -Wunused-parameter. */
#define UNUSED(x) ((void)(x))

/* Offset of a member, and the reverse: given a member pointer, find the
 * containing struct. This is THE linked-list trick used throughout the
 * Linux kernel (container_of). */
#define CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#include <stddef.h>

typedef struct { int id; char name[16]; } Item;

int main(void)
{
    puts("=== PITFALL 1: PARENTHESISE THE PARAMETERS ===");
    {
        int a = 2, b = 3;
        printf("  #define BROKEN_SQUARE(x)  x * x\n");
        printf("  BROKEN_SQUARE(a + b) expands to  a + b * a + b  = %d\n",
               BROKEN_SQUARE(a + b));
        printf("  OK_SQUARE(a + b)     expands to ((a+b) * (a+b)) = %d  <- correct\n",
               OK_SQUARE(a + b));
        puts("  The preprocessor pastes TEXT. It has no idea `a + b` is one value.");
    }

    puts("\n=== PITFALL 2: PARENTHESISE THE WHOLE BODY ===");
    {
        printf("  #define BROKEN_DOUBLE(x)  (x) * 2\n");
        printf("  100 / BROKEN_DOUBLE(5) expands to 100 / (5) * 2 = %d\n",
               100 / BROKEN_DOUBLE(5));
        printf("  100 / OK_DOUBLE(5)     expands to 100 / ((5)*2) = %d  <- correct\n",
               100 / OK_DOUBLE(5));
        puts("  Surrounding context binds to the LAST token of your expansion.");
    }

    puts("\n=== PITFALL 3: ARGUMENTS EVALUATED MORE THAN ONCE ===");
    {
        int i = 5, j = 3;
        int r1 = max_i(i++, j);
        printf("  max_i(i++, j)     = %d, i is now %d   <- evaluated ONCE\n", r1, i);

        i = 5;
        int r2 = MAX_MACRO(i++, j);
        printf("  MAX_MACRO(i++, j) = %d, i is now %d   <- evaluated TWICE\n", r2, i);
        puts("    expands to ((i++) > (j) ? (i++) : (j)) — two increments, and");
        puts("    the result is not even the max of the original operands.");
        puts("  The same trap with any side effect: MAX(f(), g()) calls each twice.");
        puts("  FIX: static inline. Type-checked, single evaluation, debuggable.");
        puts("  Use a macro only when you genuinely need type-independence, and");
        puts("  then prefer _Generic (module 11).");
    }

    puts("\n=== PITFALL 4: MULTI-STATEMENT MACROS ===");
    {
        int x = 1, y = 2;
        OK_SWAP(x, y);
        printf("  OK_SWAP(x, y) -> x=%d y=%d\n", x, y);
        puts("");
        puts("  Why do { ... } while (0) and not just { ... }:");
        puts("      if (cond)");
        puts("          BROKEN_SWAP(a, b);      /* expands to { ... }; */");
        puts("      else                        /* the stray ; ENDS the if */");
        puts("          other();                /* 'else without a previous if' */");
        puts("  do/while(0) is a single STATEMENT that still needs a semicolon,");
        puts("  so it behaves exactly like a function call everywhere.");
        puts("  It also gives the macro its own scope for temporaries.");
    }

    puts("\n=== STRINGIFY (#) AND PASTE (##) ===");
    {
        printf("  STR(hello world)  -> \"%s\"\n", STR(hello world));
        printf("  STR(VERSION)      -> \"%s\"   <- # does NOT expand its argument\n",
               STR(VERSION));
        printf("  XSTR(VERSION)     -> \"%s\"        <- the two-level idiom expands first\n",
               XSTR(VERSION));
        printf("  STR(1 + 2)        -> \"%s\"     (text, not 3)\n", STR(1 + 2));

        int myvar = 99;
        printf("  CAT(my, var)      -> %d   (pasted into the identifier `myvar`)\n",
               CAT(my, var));

        printf("\n  DEFINE_MIN generated three real functions:\n");
        printf("    min_i(3, 7)     = %d\n", min_i(3, 7));
        printf("    min_d(3.5, 2.5) = %g\n", min_d(3.5, 2.5));
        printf("    min_l(9L, 4L)   = %ld\n", min_l(9L, 4L));
        puts("  Template-style code generation. Every generic container in C");
        puts("  before _Generic was built exactly this way.");
    }

    puts("\n=== PREDEFINED MACROS ===");
    {
        printf("  __FILE__         = %s\n", __FILE__);
        printf("  __LINE__         = %d\n", __LINE__);
        printf("  __func__         = %s   (C99 — a variable, not a macro)\n", __func__);
        printf("  __DATE__ __TIME__= %s %s\n", __DATE__, __TIME__);
        printf("  __STDC_VERSION__ = %ldL   ", __STDC_VERSION__);
        #if __STDC_VERSION__ >= 201710L
            puts("(C17 or later)");
        #elif __STDC_VERSION__ >= 201112L
            puts("(C11)");
        #else
            puts("(C99 or earlier)");
        #endif
        #ifdef __GNUC__
            printf("  __GNUC__         = %d (GCC-compatible compiler)\n", __GNUC__);
        #endif
    }

    puts("\n=== CONDITIONAL COMPILATION ===");
    {
        #ifdef NDEBUG
            puts("  NDEBUG is defined: assertions and LOG are compiled OUT");
        #else
            puts("  NDEBUG is NOT defined: assertions and LOG are active");
        #endif
        LOG("a debug message with a value: %d", 42);
        puts("  Build with -DNDEBUG and both vanish — zero runtime cost, and the");
        puts("  arguments are not even evaluated.");
        puts("");
        puts("  assert() is the same mechanism from <assert.h>. Use it for things");
        puts("  that CANNOT happen if your code is correct (internal invariants),");
        puts("  NOT for validating input — input errors must be handled, and");
        puts("  -DNDEBUG would remove the check.");
        assert(1 + 1 == 2);
    }

    puts("\n=== MACROS WORTH KEEPING ===");
    {
        int nums[] = {5, 3, 9, 1, 7};
        printf("  ARRAY_LEN(nums)   = %zu\n", ARRAY_LEN(nums));
        printf("  CLAMP(150, 0, 100)= %d\n", CLAMP(150, 0, 100));
        printf("  CLAMP(-5, 0, 100) = %d\n", CLAMP(-5, 0, 100));

        STATIC_ASSERT(sizeof(int) >= 4, "this code assumes int is at least 32 bits");
        puts("  STATIC_ASSERT fires at COMPILE time — free, and it cannot drift.");

        /* container_of: given a pointer to a MEMBER, recover the struct. */
        Item item = {7, "widget"};
        char *name_ptr = item.name;
        Item *recovered = CONTAINER_OF(name_ptr, Item, name);
        printf("  CONTAINER_OF(&item.name, Item, name) -> id=%d name=%s\n",
               recovered->id, recovered->name);
        puts("  This is how the Linux kernel does intrusive linked lists: the");
        puts("  node is embedded in your struct, and container_of walks back from");
        puts("  the node to the object. One list implementation, any type.");

        UNUSED(nums);
    }

    puts("\n=== INCLUDE GUARDS ===");
    puts("      #ifndef MYHEADER_H");
    puts("      #define MYHEADER_H");
    puts("      ...contents...");
    puts("      #endif");
    puts("  Without them, a header included twice redefines its types and the");
    puts("  build fails. #pragma once is shorter and every real compiler supports");
    puts("  it, but it is not ISO C — use guards in code that must be portable.");

    puts("\n=== WHEN TO USE THE PREPROCESSOR AT ALL ===");
    puts("  YES: include guards; conditional compilation for platforms and");
    puts("       debug builds; anything needing __FILE__/__LINE__; X-macros");
    puts("       (see 05_xmacros.c); generating families of functions.");
    puts("  NO : constants        -> enum or static const");
    puts("       simple functions -> static inline");
    puts("       type-generic ops -> _Generic (C11, module 11)");
    puts("");
    puts("  The preprocessor does not understand types, scope, or evaluation.");
    puts("  Every one of its pitfalls comes from that. Use the language where");
    puts("  the language can do the job.");

    return 0;
}
