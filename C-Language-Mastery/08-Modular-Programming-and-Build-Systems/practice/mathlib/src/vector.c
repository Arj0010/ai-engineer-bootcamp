/* vector.c — the second translation unit.
 *
 * It uses ml_sqrt_newton and ml_log from stats.c. The compiler only sees
 * their DECLARATIONS (from internal.h) and emits a call to an undefined
 * symbol; the LINKER matches it to the definition in stats.o.
 *
 * Run `nm vector.o` after building and you will see:
 *     U ml_sqrt_newton      <- Undefined: needed from elsewhere
 *     T vec3_length         <- Text: defined here
 */
#include "mathlib.h"
#include "internal.h"

Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return (Vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    return (Vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 vec3_scale(Vec3 v, double k)
{
    return (Vec3){v.x * k, v.y * k, v.z * k};
}

double vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

double vec3_length(Vec3 v)
{
    return ml_sqrt_newton(vec3_dot(v, v));    /* resolved by the linker */
}

Vec3 vec3_normalise(Vec3 v)
{
    double len = vec3_length(v);
    if (len < ML_EPSILON) {
        ml_log("vec3_normalise on a zero-length vector");
        return (Vec3){0.0, 0.0, 0.0};
    }
    return vec3_scale(v, 1.0 / len);
}
