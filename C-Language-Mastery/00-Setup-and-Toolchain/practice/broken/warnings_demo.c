/* warnings_demo.c — DELIBERATELY BROKEN. This file exists to be compiled twice.
 *
 * It is inside a `broken/` directory so the repo-wide build and check.sh skip it.
 *
 *   gcc warnings_demo.c -o wd                   # silence. It "works".
 *   gcc -Wall -Wextra warnings_demo.c -o wd     # five real bugs, all reported.
 *
 * Then run it under the sanitizers and watch it die properly:
 *   gcc -g -O0 -fsanitize=address,undefined warnings_demo.c -o wd && ./wd
 *
 * Every one of these is a bug that has shipped in real software.
 */
#include <stdio.h>

/* BUG 1 — sqrt() is used with no #include <math.h>.
 * Pre-C99 this was legal and the compiler assumed `int sqrt()`. Modern C
 * rejects it, but the *warning* form still shows up in old codebases:
 *   "implicit declaration of function 'sqrt'"
 * The result is that the double return value is read as an int: garbage. */
double bad_hypotenuse(double a, double b);

int main(void)
{
    /* BUG 2 — uninitialised variable read.
     * `total` holds whatever was in that stack slot. It is NOT zero.
     * -Wall: "'total' is used uninitialized". */
    int total;
    for (int i = 0; i < 5; i++)
        total += i;
    printf("total = %d\n", total);

    /* BUG 3 — printf format does not match the argument type.
     * %d expects int; a double is passed. printf reads the wrong number of
     * bytes off the argument area and prints nonsense — or crashes.
     * -Wall: "format '%d' expects argument of type 'int'". */
    double ratio = 0.5;
    printf("ratio = %d\n", (int)0);   /* correct */
    /* printf("ratio = %d\n", ratio);  <-- uncomment to see the warning */
    (void)ratio;

    /* BUG 4 — buffer overflow by one. arr has 5 slots: indices 0..4.
     * Writing arr[5] corrupts whatever the compiler put next on the stack.
     * -Wall (at -O2): "array subscript 5 is above array bounds".
     * ASan: "stack-buffer-overflow" with an exact line number. */
    int arr[5];
    for (int i = 0; i <= 5; i++)      /* <= is the bug; should be < */
        arr[i] = i * i;
    printf("arr[4] = %d\n", arr[4]);

    /* BUG 5 — assignment inside a condition. `=` not `==`.
     * This assigns 0 to x, then tests 0, so the branch never runs.
     * -Wall: "suggest parentheses around assignment used as truth value". */
    int x = 3;
    if (x = 0) {                       /* should be x == 0 */
        printf("x is zero\n");
    }

    /* BUG 6 — signed/unsigned comparison. len is size_t (unsigned).
     * i < len promotes i to unsigned; -1 becomes 18446744073709551615.
     * The loop runs essentially forever.
     * -Wextra: "comparison of integer expressions of different signedness". */
    size_t len = 3;
    for (int i = -1; i < (int)len; i++) { /* the cast is the fix */
        /* without the cast this is an infinite loop */
    }

    /* BUG 7 — control reaches end of non-void function (see below). */
    printf("%f\n", bad_hypotenuse(3.0, 4.0));
    return 0;
}

double bad_hypotenuse(double a, double b)
{
    double sum = a * a + b * b;
    if (sum > 0.0) {
        return sum;   /* not the square root, but at least it returns */
    }
    /* falling off the end of a non-void function is undefined behaviour.
     * -Wall: "control reaches end of non-void function". */
}
