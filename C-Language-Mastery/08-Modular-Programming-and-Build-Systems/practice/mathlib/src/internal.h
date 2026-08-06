/* internal.h — a PRIVATE header. It lives in src/, not include/, so it is
 * not part of the published API and callers cannot reach it.
 *
 * Splitting headers this way is the whole point of the include/ + src/ layout:
 * `-Iinclude` puts mathlib.h on the search path for everyone, while this file
 * is only findable from within src/.
 */
#ifndef MATHLIB_INTERNAL_H
#define MATHLIB_INTERNAL_H

#include <stddef.h>

/* Shared between stats.c and vector.c, but invisible outside the library.
 * These are DECLARATIONS; the definitions are in src/stats.c. */
void   ml_log(const char *fmt, ...);
double ml_sqrt_newton(double x);      /* so the demo needs no -lm */

/* A compile-time constant used by more than one implementation file. */
#define ML_EPSILON 1e-12

#endif /* MATHLIB_INTERNAL_H */
