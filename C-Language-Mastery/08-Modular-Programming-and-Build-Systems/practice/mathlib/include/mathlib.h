/* mathlib.h — THE PUBLIC API.
 *
 * This is the entire contract. Everything a caller may rely on is here;
 * everything else is an implementation detail in src/ and can change freely.
 *
 * A header contains DECLARATIONS. Put a function DEFINITION or a variable
 * DEFINITION here and you get "multiple definition" the moment two .c files
 * include it.
 */
#ifndef MATHLIB_H            /* INCLUDE GUARD — without it, a header included */
#define MATHLIB_H            /* twice redefines its types and the build fails */

#include <stddef.h>          /* size_t — needed by the declarations BELOW, so it
                              * belongs here. Anything only the .c files need
                              * goes in the .c files, not here. */
#include <stdbool.h>

/* ---------------------------------------------------------------- *
 * Version — exposed so callers can adapt to different builds.
 * ---------------------------------------------------------------- */
#define MATHLIB_VERSION_MAJOR 1
#define MATHLIB_VERSION_MINOR 2

const char *mathlib_version(void);

/* ---------------------------------------------------------------- *
 * A shared global.
 *
 *   extern here  = "this exists somewhere" (a DECLARATION, no storage)
 *   int  in .c   = "here it is"            (the DEFINITION, exactly one)
 *
 * Getting this backwards — defining it in the header — gives you
 * "multiple definition of g_mathlib_verbose" as soon as two files include
 * this. That is the single most common multi-file mistake.
 * ---------------------------------------------------------------- */
extern int g_mathlib_verbose;

/* ---------------------------------------------------------------- *
 * Statistics over an array of doubles.
 * ---------------------------------------------------------------- */
typedef struct {
    double mean;
    double variance;      /* population variance */
    double stddev;
    double min, max;
    size_t count;
} Stats;

/* Returns false if data is NULL or n is 0. `out` is written only on success. */
bool   stats_compute(const double *data, size_t n, Stats *out);
double stats_median(double *data, size_t n);   /* NOTE: SORTS the input in place */

/* ---------------------------------------------------------------- *
 * A 3D vector. Defined here (not opaque) because callers create these by
 * value and the layout is part of the API.
 * ---------------------------------------------------------------- */
typedef struct { double x, y, z; } Vec3;

Vec3   vec3_add(Vec3 a, Vec3 b);
Vec3   vec3_sub(Vec3 a, Vec3 b);
Vec3   vec3_scale(Vec3 v, double k);
double vec3_dot(Vec3 a, Vec3 b);
Vec3   vec3_cross(Vec3 a, Vec3 b);
double vec3_length(Vec3 v);
Vec3   vec3_normalise(Vec3 v);      /* returns {0,0,0} for a zero vector */

/* A `static inline` function IS allowed in a header: each translation unit
 * gets its own private copy, so there is no multiple-definition problem, and
 * the compiler can inline it away entirely. This is how you put a tiny helper
 * in a header without paying for a call. */
static inline bool vec3_is_zero(Vec3 v)
{
    return v.x == 0.0 && v.y == 0.0 && v.z == 0.0;
}

#endif /* MATHLIB_H */
