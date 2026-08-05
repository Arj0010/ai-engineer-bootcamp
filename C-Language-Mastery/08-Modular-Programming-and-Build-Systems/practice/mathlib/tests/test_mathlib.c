/* test_mathlib.c — a self-contained test program.
 *
 * It links against the library's object files but has its own main(), which
 * is why the Makefile builds it separately: a program may have exactly ONE
 * main, so main.o and test_mathlib.o can never be in the same executable.
 */
#include "mathlib.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static int tests_run = 0, tests_failed = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        tests_run++;                                                    \
        if (!(cond)) {                                                  \
            tests_failed++;                                             \
            printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);    \
        }                                                               \
    } while (0)

/* Floating point never compares exactly. Always use a tolerance. */
#define CHECK_NEAR(a, b, tol)                                                  \
    do {                                                                       \
        tests_run++;                                                           \
        double _d = (a) - (b);                                                 \
        if (_d < 0) _d = -_d;                                                  \
        if (_d > (tol)) {                                                      \
            tests_failed++;                                                    \
            printf("  FAIL %s:%d  %s == %s  (%.10f vs %.10f)\n",               \
                   __FILE__, __LINE__, #a, #b, (double)(a), (double)(b));      \
        }                                                                      \
    } while (0)

static void test_stats(void)
{
    puts("  stats_compute");
    double data[] = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    Stats s;

    CHECK(stats_compute(data, 8, &s) == true);
    CHECK_NEAR(s.mean,     5.0, 1e-9);      /* 40 / 8 */
    CHECK_NEAR(s.variance, 4.0, 1e-9);      /* the textbook example */
    CHECK_NEAR(s.stddev,   2.0, 1e-9);
    CHECK_NEAR(s.min,      2.0, 1e-9);
    CHECK_NEAR(s.max,      9.0, 1e-9);
    CHECK(s.count == 8);

    /* boundary and error cases — the part people skip */
    CHECK(stats_compute(NULL, 8, &s)  == false);
    CHECK(stats_compute(data, 0, &s)  == false);
    CHECK(stats_compute(data, 8, NULL)== false);

    double single = 42.0;
    CHECK(stats_compute(&single, 1, &s) == true);
    CHECK_NEAR(s.mean, 42.0, 1e-9);
    CHECK_NEAR(s.variance, 0.0, 1e-9);
}

static void test_median(void)
{
    puts("  stats_median");
    double odd[]  = {5.0, 1.0, 3.0};
    double even[] = {4.0, 1.0, 3.0, 2.0};
    CHECK_NEAR(stats_median(odd, 3),  3.0, 1e-9);
    CHECK_NEAR(stats_median(even, 4), 2.5, 1e-9);   /* (2+3)/2 */
    CHECK_NEAR(stats_median(NULL, 0), 0.0, 1e-9);

    /* the documented side effect: the input is now sorted */
    CHECK(odd[0] == 1.0 && odd[1] == 3.0 && odd[2] == 5.0);
}

static void test_vectors(void)
{
    puts("  vec3");
    Vec3 a = {3.0, 4.0, 0.0};
    Vec3 b = {1.0, 0.0, 2.0};

    Vec3 sum = vec3_add(a, b);
    CHECK_NEAR(sum.x, 4.0, 1e-9);
    CHECK_NEAR(sum.y, 4.0, 1e-9);
    CHECK_NEAR(sum.z, 2.0, 1e-9);

    CHECK_NEAR(vec3_dot(a, b), 3.0, 1e-9);
    CHECK_NEAR(vec3_length(a), 5.0, 1e-9);          /* 3-4-5 */

    Vec3 cross = vec3_cross(a, b);
    /* a x b must be perpendicular to both — a property test, not a fixed value */
    CHECK_NEAR(vec3_dot(cross, a), 0.0, 1e-9);
    CHECK_NEAR(vec3_dot(cross, b), 0.0, 1e-9);

    Vec3 unit = vec3_normalise(a);
    CHECK_NEAR(vec3_length(unit), 1.0, 1e-9);

    /* the edge case: normalising a zero vector must not divide by zero */
    Vec3 zero = {0.0, 0.0, 0.0};
    Vec3 nz = vec3_normalise(zero);
    CHECK(vec3_is_zero(nz));
    CHECK(vec3_is_zero(zero));
    CHECK(!vec3_is_zero(a));
}

static void test_version(void)
{
    puts("  mathlib_version");
    CHECK(mathlib_version() != NULL);
    CHECK(strstr(mathlib_version(), "mathlib") != NULL);
    CHECK(MATHLIB_VERSION_MAJOR == 1);
}

int main(void)
{
    puts("running mathlib tests\n");

    test_version();
    test_stats();
    test_median();
    test_vectors();

    printf("\n%d checks, %d failed\n", tests_run, tests_failed);
    if (tests_failed == 0) puts("ALL TESTS PASSED");

    /* Non-zero exit on failure so `make test` and CI notice. */
    return tests_failed == 0 ? 0 : 1;
}
