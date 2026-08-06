/* 04_function_design.c — pass-by-value, out-parameters, error conventions, varargs.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 04_function_design.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>

/* ================================================================= *
 * 1. EVERYTHING IS PASS-BY-VALUE
 * ================================================================= */

static void takes_a_copy(int x)      { x = 99; (void)x; }   /* caller unaffected */
static void takes_an_address(int *x) { *x = 99; }           /* modifies the caller's object */

/* Even the pointer is passed by value. Reassigning the POINTER inside the
 * function does nothing to the caller's pointer. */
static int other_object = 7;
static void reassigns_the_pointer(int *p)  { p = &other_object; (void)p; }
/* To change the caller's POINTER you need a pointer TO the pointer. */
static void reassigns_via_pp(int **pp)     { *pp = &other_object; }

/* A struct is copied wholesale — cheap for small ones, wasteful for big ones. */
typedef struct { double x, y, z; char label[32]; } Big;
static void by_value(Big b)        { b.x = -1.0; (void)b; }        /* copies 56 bytes */
static void by_pointer(Big *b)     { b->x = -1.0; }                /* copies 8 bytes  */
static double read_only(const Big *b) { return b->x; }             /* const = promise */

/* ================================================================= *
 * 2. RETURNING MORE THAN ONE VALUE
 * ================================================================= */

/* Option A: out-parameters. The C standard library's own style
 * (strtol, fread, sscanf). Return the STATUS, write the RESULTS. */
static bool divide(int a, int b, int *quotient, int *remainder)
{
    if (b == 0) return false;                /* the caller must check */
    if (quotient)  *quotient  = a / b;       /* tolerate NULL for "don't care" */
    if (remainder) *remainder = a % b;
    return true;
}

/* Option B: return a small struct by value. Clear, and free at -O2 because
 * the compiler returns it in registers. */
typedef struct { int min, max; double mean; } Stats;
static Stats analyse(const int *a, size_t n)
{
    Stats s = {0, 0, 0.0};
    if (n == 0) return s;
    s.min = s.max = a[0];
    long long sum = 0;
    for (size_t i = 0; i < n; i++) {
        if (a[i] < s.min) s.min = a[i];
        if (a[i] > s.max) s.max = a[i];
        sum += a[i];
    }
    s.mean = (double)sum / (double)n;
    return s;
}

/* ================================================================= *
 * 3. ERROR-REPORTING CONVENTIONS — pick one and be consistent
 * ================================================================= */

/* (a) Sentinel return value. Works only when one value can be reserved. */
static int find_index(const int *a, size_t n, int target)
{
    for (size_t i = 0; i < n; i++)
        if (a[i] == target) return (int)i;
    return -1;                                    /* the sentinel */
}

/* (b) Negative errno-style codes. Scales to many distinct errors.
 *     This is the Linux kernel convention. */
#define ERR_NULL_ARG   (-1)
#define ERR_EMPTY      (-2)
#define ERR_OVERFLOW   (-3)
static int sum_checked(const int *a, size_t n, long long *out)
{
    if (a == NULL || out == NULL) return ERR_NULL_ARG;
    if (n == 0)                   return ERR_EMPTY;
    long long sum = 0;
    for (size_t i = 0; i < n; i++) sum += a[i];
    *out = sum;
    return 0;                                     /* 0 == success */
}
static const char *err_str(int code)
{
    switch (code) {
    case 0:              return "ok";
    case ERR_NULL_ARG:   return "null argument";
    case ERR_EMPTY:      return "empty input";
    case ERR_OVERFLOW:   return "overflow";
    default:             return "unknown error";
    }
}

/* (c) NULL for pointer-returning functions, with the reason in errno. */
static char *duplicate_string(const char *s)
{
    if (s == NULL) { errno = EINVAL; return NULL; }
    size_t n = strlen(s) + 1;
    char *copy = malloc(n);
    if (copy == NULL) { errno = ENOMEM; return NULL; }   /* malloc already set it */
    memcpy(copy, s, n);
    return copy;                                  /* CALLER MUST free() */
}

/* ================================================================= *
 * 4. VARIADIC FUNCTIONS
 * ================================================================= */

/* varargs carry NO type or count information at run time. You must supply
 * both out of band — here, an explicit count. printf uses the format string,
 * which is why a wrong specifier is undefined behaviour. */
static long long sum_n(int count, ...)
{
    va_list ap;
    va_start(ap, count);              /* `count` = the last NAMED parameter */
    long long total = 0;
    for (int i = 0; i < count; i++)
        total += va_arg(ap, int);     /* the type MUST match what was passed */
    va_end(ap);                       /* required */
    return total;
}

/* A sentinel-terminated variant, the strategy execv() uses. */
static size_t total_length(const char *first, ...)
{
    if (first == NULL) return 0;
    size_t total = strlen(first);
    va_list ap;
    va_start(ap, first);
    for (const char *s = va_arg(ap, const char *); s != NULL;
         s = va_arg(ap, const char *))
        total += strlen(s);
    va_end(ap);
    return total;
}

/* Forwarding varargs: you cannot call printf(fmt, ...) directly — you must
 * use the v-prefixed variant that takes a va_list. Every printf-family
 * function has one: vprintf, vsnprintf, vfprintf. */
static void log_msg(const char *level, const char *fmt, ...)
{
    fprintf(stderr, "[%s] ", level);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);        /* NOT fprintf(stderr, fmt, ap) */
    va_end(ap);
    fputc('\n', stderr);
}

/* ================================================================= *
 * 5. static inline vs macro
 * ================================================================= */
static inline int max_i(int a, int b) { return a > b ? a : b; }
#define MAX_MACRO(a, b) ((a) > (b) ? (a) : (b))

int main(void)
{
    puts("=== 1. EVERYTHING IS PASS-BY-VALUE ===");
    {
        int n = 1;
        takes_a_copy(n);       printf("  after takes_a_copy(n)      : n = %d  (unchanged)\n", n);
        takes_an_address(&n);  printf("  after takes_an_address(&n) : n = %d  (changed)\n", n);

        int a = 1, *p = &a;
        reassigns_the_pointer(p);
        printf("  after reassigns_the_pointer(p): p still points at %d\n", *p);
        reassigns_via_pp(&p);
        printf("  after reassigns_via_pp(&p)    : p now points at %d\n", *p);
        puts("  The pointer itself is a value. To change it, pass ITS address.");

        Big b = {1.0, 2.0, 3.0, "sample"};
        by_value(b);    printf("  by_value  : b.x = %.1f (copied %zu bytes, discarded)\n",
                               b.x, sizeof b);
        by_pointer(&b); printf("  by_pointer: b.x = %.1f (copied %zu bytes)\n",
                               b.x, sizeof(Big *));
        printf("  read_only : %.1f  <- const promises the callee will not write\n",
               read_only(&b));
    }

    puts("\n=== 2. RETURNING MULTIPLE VALUES ===");
    {
        int q, r;
        if (divide(17, 5, &q, &r)) printf("  17 / 5 = %d remainder %d\n", q, r);
        if (!divide(17, 0, &q, &r)) puts("  17 / 0 correctly refused (returned false)");
        if (divide(17, 5, &q, NULL)) printf("  quotient only: %d (passed NULL for remainder)\n", q);

        int data[] = {5, -3, 12, 8, 0, -7, 21};
        Stats s = analyse(data, sizeof data / sizeof data[0]);
        printf("  analyse() returned a struct: min=%d max=%d mean=%.2f\n",
               s.min, s.max, s.mean);
    }

    puts("\n=== 3. ERROR CONVENTIONS ===");
    {
        int data[] = {5, -3, 12};
        size_t n = sizeof data / sizeof data[0];
        printf("  (a) sentinel  : find_index(12) = %d, find_index(99) = %d\n",
               find_index(data, n, 12), find_index(data, n, 99));

        long long total;
        printf("  (b) error code: sum_checked(ok)    -> %s, total = ",
               err_str(sum_checked(data, n, &total)));
        printf("%lld\n", total);
        printf("  (b) error code: sum_checked(NULL)  -> %s\n",
               err_str(sum_checked(NULL, n, &total)));
        printf("  (b) error code: sum_checked(n = 0) -> %s\n",
               err_str(sum_checked(data, 0, &total)));

        char *copy = duplicate_string("owned by the caller");
        if (copy != NULL) { printf("  (c) NULL+errno: got \"%s\"\n", copy); free(copy); }
        errno = 0;
        if (duplicate_string(NULL) == NULL)
            printf("  (c) NULL+errno: refused NULL, errno = %d (%s)\n", errno, strerror(errno));
        puts("  Document ownership in the header: who frees what. C will not do it for you.");
    }

    puts("\n=== 4. VARIADIC FUNCTIONS ===");
    printf("  sum_n(4, 10,20,30,40)         = %lld\n", sum_n(4, 10, 20, 30, 40));
    printf("  sum_n(0)                      = %lld\n", sum_n(0));
    printf("  total_length(\"ab\",\"cde\",NULL) = %zu\n",
           total_length("ab", "cde", (const char *)NULL));
    log_msg("INFO",  "forwarded through vfprintf: %d items, %.1f%% done", 42, 87.5);
    log_msg("ERROR", "no arguments at all");
    puts("  varargs carry no type info. The count or a sentinel must come from");
    puts("  somewhere else — for printf, that is the format string, which is");
    puts("  exactly why a mismatched %-specifier is undefined behaviour.");
    puts("  Also: arguments smaller than int are PROMOTED. va_arg(ap, char) is");
    puts("  always wrong; use va_arg(ap, int). Likewise float becomes double.");

    puts("\n=== 5. static inline BEATS a function-like macro ===");
    {
        /* Note we take the result into a variable BEFORE printing it. Writing
         * printf("%d %d", max_i(i++, j), i) would itself be undefined
         * behaviour — i is modified and read in one unsequenced expression. */
        int i = 5, j = 3;
        int r1 = max_i(i++, j);
        printf("  max_i(i++, j)     = %d, i is now %d  <- i++ evaluated ONCE\n", r1, i);

        i = 5;
        int r2 = MAX_MACRO(i++, j);
        printf("  MAX_MACRO(i++, j) = %d, i is now %d  <- i++ evaluated TWICE\n", r2, i);
        puts("    the macro expands to ((i++) > (j) ? (i++) : (j)) — two increments,");
        puts("    and the returned value is not even the max of the original operands.");
        puts("  A macro also has no type checking and no debugger symbol.");
        puts("  Use `static inline` in headers; reach for macros only when you");
        puts("  need type-independence (see _Generic in module 11).");
    }

    puts("\n=== DESIGN RULES ===");
    puts("  1. One job per function. If you need `and` to describe it, split it.");
    puts("  2. Take `const T *` for inputs you will not modify — it is a");
    puts("     compiler-checked contract and it enables optimisations.");
    puts("  3. Prefer caller-allocated buffers over returning malloc'd memory:");
    puts("         void fmt(char *out, size_t outsz, ...)");
    puts("     The caller then controls the lifetime and there is nothing to leak.");
    puts("  4. Always pass the buffer SIZE alongside the buffer pointer.");
    puts("  5. Validate arguments at public entry points; assert internally.");
    puts("  6. Mark everything not in a header `static`.");

    /* silence the unused-function warning for a demo helper */
    (void)fabs(0.0);
    return 0;
}
