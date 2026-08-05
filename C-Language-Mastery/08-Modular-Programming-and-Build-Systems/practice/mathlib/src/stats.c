/* stats.c — one translation unit of the library.
 *
 * Note the pattern: `#include "mathlib.h"` FIRST. If the public header is
 * missing an include it needs, this is where it breaks — which is exactly
 * when you want to find out.
 */
#include "mathlib.h"
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* THE DEFINITION of the global declared `extern` in mathlib.h.
 * Exactly one translation unit may do this. */
int g_mathlib_verbose = 0;

/* ---------------------------------------------------------------- *
 * Internal helpers. `static` = internal linkage = invisible to the linker
 * outside this file. Another .c file may define its own `clamp01` with no
 * conflict, and the compiler is free to inline or delete this one.
 * ---------------------------------------------------------------- */
static double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

static int cmp_double(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

/* NOT static: declared in internal.h so vector.c can use it. It has external
 * linkage, so it WILL appear in the library's symbol table — visible to the
 * linker, but not to anyone who only has the public header. */
void ml_log(const char *fmt, ...)
{
    if (!g_mathlib_verbose) return;
    va_list ap;
    va_start(ap, fmt);
    fputs("[mathlib] ", stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}

/* Newton's method, so the library needs no -lm and the link line stays simple.
 * (In real code you would just use sqrt() and link -lm.) */
double ml_sqrt_newton(double x)
{
    if (x < 0.0) return -1.0;
    if (x == 0.0) return 0.0;
    double guess = x > 1.0 ? x / 2.0 : 1.0;
    for (int i = 0; i < 60; i++) {
        double next = 0.5 * (guess + x / guess);
        double delta = next - guess;
        if (delta < 0) delta = -delta;
        guess = next;
        if (delta < ML_EPSILON * guess) break;
    }
    return guess;
}

/* ---------------------------------------------------------------- *
 * Public API
 * ---------------------------------------------------------------- */
const char *mathlib_version(void) { return "mathlib 1.2"; }

bool stats_compute(const double *data, size_t n, Stats *out)
{
    if (data == NULL || out == NULL || n == 0) return false;

    ml_log("stats_compute over %zu values", n);

    double sum = 0.0, lo = data[0], hi = data[0];
    for (size_t i = 0; i < n; i++) {
        sum += data[i];
        if (data[i] < lo) lo = data[i];
        if (data[i] > hi) hi = data[i];
    }
    double mean = sum / (double)n;

    double sq = 0.0;
    for (size_t i = 0; i < n; i++) {
        double d = data[i] - mean;
        sq += d * d;
    }

    out->mean     = mean;
    out->variance = sq / (double)n;
    out->stddev   = ml_sqrt_newton(out->variance);
    out->min      = lo;
    out->max      = hi;
    out->count    = n;

    (void)clamp01;          /* kept to show a private helper; unused here */
    return true;
}

double stats_median(double *data, size_t n)
{
    if (data == NULL || n == 0) return 0.0;
    qsort(data, n, sizeof *data, cmp_double);     /* documented: SORTS IN PLACE */
    return (n % 2 == 1) ? data[n / 2]
                        : (data[n / 2 - 1] + data[n / 2]) / 2.0;
}
