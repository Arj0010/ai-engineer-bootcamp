/* main.c — a client of the library.
 *
 * It includes ONLY the public header. It cannot see src/internal.h, cannot
 * call ml_log or ml_sqrt_newton, and does not know that stats.c and vector.c
 * are separate files. That separation is the point of the whole exercise.
 */
#include "mathlib.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    printf("=== %s ===\n\n", mathlib_version());
    printf("  version macros from the header: %d.%d\n\n",
           MATHLIB_VERSION_MAJOR, MATHLIB_VERSION_MINOR);

    /* g_mathlib_verbose is declared `extern` in mathlib.h and DEFINED in
     * stats.c. We are writing to the library's own variable across a
     * translation-unit boundary — that is what external linkage means. */
    g_mathlib_verbose = 1;
    puts("  (verbose logging on — [mathlib] lines below come from stats.c)");

    puts("\n=== STATISTICS ===");
    {
        double data[] = {23.5, 67.1, 12.8, 45.0, 89.3, 34.7, 56.2, 78.9, 41.1, 29.4};
        size_t n = sizeof data / sizeof data[0];

        Stats s;
        if (stats_compute(data, n, &s)) {
            printf("    count    = %zu\n", s.count);
            printf("    mean     = %.4f\n", s.mean);
            printf("    variance = %.4f\n", s.variance);
            printf("    stddev   = %.4f\n", s.stddev);
            printf("    min/max  = %.2f / %.2f\n", s.min, s.max);
        }

        /* stats_median SORTS its input — documented in the header. Work on a
         * copy when the caller's order matters. */
        double copy[16];
        memcpy(copy, data, sizeof data);
        printf("    median   = %.4f (computed on a copy; original order kept)\n",
               stats_median(copy, n));

        printf("    refusing bad input: stats_compute(NULL, 0, &s) -> %s\n",
               stats_compute(NULL, 0, &s) ? "true" : "false");
    }

    puts("\n=== VECTORS ===");
    {
        Vec3 a = {3.0, 4.0, 0.0};
        Vec3 b = {1.0, 0.0, 2.0};

        Vec3 sum   = vec3_add(a, b);
        Vec3 diff  = vec3_sub(a, b);
        Vec3 cross = vec3_cross(a, b);
        Vec3 unit  = vec3_normalise(a);

        printf("    a          = (%.1f, %.1f, %.1f)\n", a.x, a.y, a.z);
        printf("    b          = (%.1f, %.1f, %.1f)\n", b.x, b.y, b.z);
        printf("    a + b      = (%.1f, %.1f, %.1f)\n", sum.x, sum.y, sum.z);
        printf("    a - b      = (%.1f, %.1f, %.1f)\n", diff.x, diff.y, diff.z);
        printf("    a . b      = %.1f\n", vec3_dot(a, b));
        printf("    a x b      = (%.1f, %.1f, %.1f)\n", cross.x, cross.y, cross.z);
        printf("    |a|        = %.4f  (3-4-5 triangle, so 5)\n", vec3_length(a));
        printf("    normalise a= (%.4f, %.4f, %.4f), length %.6f\n",
               unit.x, unit.y, unit.z, vec3_length(unit));

        /* vec3_is_zero is `static inline` in the header: this file compiled
         * its own private copy, and at -O2 there is no call at all. */
        Vec3 zero = {0, 0, 0};
        printf("    vec3_is_zero(zero) = %s  (static inline, from the header)\n",
               vec3_is_zero(zero) ? "true" : "false");
        Vec3 nz = vec3_normalise(zero);
        printf("    normalise a zero vector -> (%.1f, %.1f, %.1f), handled safely\n",
               nz.x, nz.y, nz.z);
    }

    g_mathlib_verbose = 0;

    puts("\n=== WHAT JUST HAPPENED AT LINK TIME ===");
    puts("  main.c   was compiled knowing only mathlib.h. Every call to");
    puts("           stats_compute or vec3_add became a reference to an");
    puts("           UNDEFINED symbol in main.o.");
    puts("  stats.c  and vector.c were compiled independently into stats.o");
    puts("           and vector.o, each DEFINING some of those symbols.");
    puts("  vector.o additionally NEEDED ml_sqrt_newton, defined in stats.o.");
    puts("  the linker matched every undefined symbol to a definition and");
    puts("           produced one executable.");
    puts("");
    puts("  Run `make symbols` to see this in the actual symbol tables.");
    puts("  Delete stats.c from the Makefile and you get exactly the error");
    puts("  everyone has met: \"undefined reference to `stats_compute'\".");
    puts("  It is a LINKER error. It always means: declared, never defined,");
    puts("  or defined in a file you forgot to compile or link.");

    return 0;
}
