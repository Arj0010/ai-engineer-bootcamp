/* 01_types_and_sizes.c — what the types on THIS machine actually are.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_types_and_sizes.c -o t && ./t
 *
 * Run this on any other machine you can reach and compare. The differences are
 * the reason <stdint.h> exists.
 */
#include <stdio.h>
#include <limits.h>     /* INT_MAX, CHAR_BIT, ... integer limits          */
#include <float.h>      /* DBL_MAX, DBL_EPSILON, ... floating point limits */
#include <stdint.h>     /* int32_t, uintptr_t, ...  exact-width types      */
#include <stddef.h>     /* size_t, ptrdiff_t                               */
#include <stdbool.h>    /* bool, true, false                               */

/* _Alignof (C11) reports the address boundary a type must sit on.
 * A double usually needs an address divisible by 8. This drives struct
 * padding, which is module 06. */
#define SHOW(type, fmt, lo, hi)                                          \
    printf("  %-20s %2zu bytes  align %zu   " fmt " .. " fmt "\n",       \
           #type, sizeof(type), _Alignof(type), (lo), (hi))

int main(void)
{
    puts("=== how big is a byte here? ===");
    printf("  CHAR_BIT = %d  (bits per char; 8 everywhere you will ever go)\n", CHAR_BIT);
    printf("  char is %s by default on this compiler\n",
           (char)-1 < 0 ? "SIGNED" : "UNSIGNED");
    puts("  ^ this is implementation-defined. Never assume it.\n");

    puts("=== integer types ===");
    SHOW(char,               "%12lld", (long long)CHAR_MIN,  (long long)CHAR_MAX);
    SHOW(signed char,        "%12lld", (long long)SCHAR_MIN, (long long)SCHAR_MAX);
    SHOW(unsigned char,      "%12lld", 0LL,                  (long long)UCHAR_MAX);
    SHOW(short,              "%12lld", (long long)SHRT_MIN,  (long long)SHRT_MAX);
    SHOW(unsigned short,     "%12lld", 0LL,                  (long long)USHRT_MAX);
    SHOW(int,                "%12lld", (long long)INT_MIN,   (long long)INT_MAX);
    SHOW(unsigned int,       "%12lld", 0LL,                  (long long)UINT_MAX);
    SHOW(long,               "%12lld", (long long)LONG_MIN,  (long long)LONG_MAX);
    SHOW(long long,          "%12lld", LLONG_MIN,            LLONG_MAX);
    printf("  %-20s %2zu bytes  align %zu   0 .. %llu\n",
           "unsigned long long", sizeof(unsigned long long),
           _Alignof(unsigned long long), ULLONG_MAX);

    puts("\n=== exact-width types from <stdint.h> (use these for file/network data) ===");
    printf("  int8_t=%zu  int16_t=%zu  int32_t=%zu  int64_t=%zu\n",
           sizeof(int8_t), sizeof(int16_t), sizeof(int32_t), sizeof(int64_t));
    printf("  intptr_t=%zu (holds a pointer)   size_t=%zu   ptrdiff_t=%zu\n",
           sizeof(intptr_t), sizeof(size_t), sizeof(ptrdiff_t));

    puts("\n=== floating point ===");
    printf("  %-20s %2zu bytes  align %zu   ~%d decimal digits  max %g\n",
           "float", sizeof(float), _Alignof(float), FLT_DIG, (double)FLT_MAX);
    printf("  %-20s %2zu bytes  align %zu   ~%d decimal digits  max %g\n",
           "double", sizeof(double), _Alignof(double), DBL_DIG, DBL_MAX);
    printf("  %-20s %2zu bytes  align %zu   ~%d decimal digits\n",
           "long double", sizeof(long double), _Alignof(long double), LDBL_DIG);
    printf("  DBL_EPSILON = %g  (smallest e where 1.0 + e != 1.0)\n", DBL_EPSILON);

    puts("\n=== pointers and other ===");
    printf("  %-20s %2zu bytes\n", "void *",        sizeof(void *));
    printf("  %-20s %2zu bytes\n", "int *",         sizeof(int *));
    printf("  %-20s %2zu bytes  (all data pointers are the same size here)\n",
           "double *", sizeof(double *));
    printf("  %-20s %2zu bytes\n", "void (*)(void)", sizeof(void (*)(void)));
    printf("  %-20s %2zu bytes\n", "bool",          sizeof(bool));

    puts("\n=== the guarantees the STANDARD actually makes ===");
    puts("  sizeof(char) == 1                          always");
    puts("  char <= short <= int <= long <= long long   always");
    puts("  char   at least  8 bits");
    puts("  short  at least 16 bits");
    puts("  int    at least 16 bits   <-- NOT 32");
    puts("  long   at least 32 bits   <-- 4 bytes on Windows, 8 on Linux/macOS");
    puts("  long long at least 64 bits");
    puts("  Everything else is up to the implementation.");

    /* sizeof yields size_t, which is UNSIGNED. This is why the correct format
     * specifier is %zu and not %d. Printing a size_t with %d is UB on any
     * platform where they differ in size -- which is most 64-bit platforms. */
    size_t n = sizeof(int);
    printf("\nsizeof(int) printed correctly with %%zu: %zu\n", n);

    return 0;
}
