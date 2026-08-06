/* 04_operators.c — precedence, short-circuiting, and the operators that lie.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 04_operators.c -o t && ./t
 */
#include <stdio.h>
#include <stdbool.h>

/* A function with a visible side effect, so we can SEE evaluation order. */
static int noisy(const char *name, int value)
{
    printf("    [evaluated %s -> %d]\n", name, value);
    return value;
}

/* True modulo: C's % is a REMAINDER and takes the sign of the dividend. */
static int mod_floor(int a, int n)
{
    int r = a % n;
    return (r != 0 && ((r < 0) != (n < 0))) ? r + n : r;
}

int main(void)
{
    /* The next two demos deliberately write the ambiguous form so you can see
     * what it parses to. GCC and Clang warn about exactly this (-Wparentheses,
     * part of -Wall), which is the point — so we silence the warning ONLY here
     * and nowhere else in the curriculum. In real code, add the parentheses. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
    puts("=== PRECEDENCE TRAP 1: == binds tighter than & ===");
    {
        int a = 6, b = 4, c = 4;
        printf("  a & b == c   parses as a & (b == c) = %d   <- almost never what you want\n",
               a & b == c);
        printf("  (a & b) == c is                     = %d\n", (a & b) == c);
        puts("  RULE: always parenthesise bitwise operators.");
    }

    puts("\n=== PRECEDENCE TRAP 2: + binds tighter than << ===");
    {
        int x = 1;
        printf("  x << 1 + 2   parses as x << (1 + 2) = %d\n", x << 1 + 2);
        printf("  (x << 1) + 2 is                     = %d\n", (x << 1) + 2);
    }
#pragma GCC diagnostic pop

    puts("\n=== PRECEDENCE TRAP 3: * binds tighter than ++ on the RESULT, not the pointer ===");
    {
        int arr[] = {10, 20, 30};
        int *p = arr;
        printf("  *p++   : reads %d then advances p (post-increment binds tighter than *)\n", *p++);
        printf("  now *p : %d\n", *p);
        p = arr;
        printf("  (*p)++ : reads %d and increments the VALUE\n", (*p)++);
        printf("  arr[0] : %d\n", arr[0]);
    }

    puts("\n=== % IS REMAINDER, NOT MODULO ===");
    printf("   7 %% 3 = %d      -7 %% 3 = %d   <- negative!\n", 7 % 3, -7 % 3);
    printf("   7 / 3 = %d      -7 / 3 = %d   <- truncates TOWARD ZERO\n", 7 / 3, -7 / 3);
    printf("  mod_floor(-7, 3) = %d   <- what you usually want (always 0..n-1)\n",
           mod_floor(-7, 3));
    puts("  Invariant the standard guarantees: (a/b)*b + a%b == a");

    puts("\n=== INTEGER DIVISION TRUNCATES ===");
    printf("  5 / 2       = %d      (both operands int -> integer division)\n", 5 / 2);
    printf("  5 / 2.0     = %g      (one operand double -> both become double)\n", 5 / 2.0);
    printf("  (double)5/2 = %g\n", (double)5 / 2);
    printf("  5.0 / 0     = %g      (float division by zero: inf, well-defined)\n", 5.0 / 0.0);
    puts("  5 / 0 with INTEGERS is UNDEFINED BEHAVIOUR — usually SIGFPE, a crash.");

    puts("\n=== SHORT-CIRCUIT EVALUATION (guaranteed by the standard) ===");
    puts("  false && noisy(...)  -> right side NEVER evaluated:");
    if (false && noisy("right side of &&", 1)) { }
    puts("  true  || noisy(...)  -> right side NEVER evaluated:");
    if (true || noisy("right side of ||", 1)) { }
    puts("  This is why the NULL guard idiom is correct:");
    puts("      if (p != NULL && p->field > 0)     /* p->field is safe */");
    puts("      if (i < n && arr[i] == target)     /* arr[i] is in bounds */");

    puts("\n  But & and | do NOT short-circuit — both sides always evaluate:");
    if (noisy("left of &", 0) & noisy("right of &", 1)) { }

    puts("\n=== ARGUMENT EVALUATION ORDER IS UNSPECIFIED ===");
    puts("  printf(\"%d %d\", f(), g());  -- f may run before or after g.");
    printf("  ");
    printf("%d %d\n", noisy("\n      first arg", 1), noisy("      second arg", 2));
    puts("  If f() and g() touch shared state, the program is not deterministic.");

    puts("\n=== EXPRESSIONS THAT ARE UNDEFINED BEHAVIOUR ===");
    puts("    i = i++ + ++i;          modified twice, no sequence point");
    puts("    arr[i] = i++;           read and write of i unsequenced");
    puts("    printf(\"%d %d\", i++, i++);");
    puts("  These are not 'compiler-dependent'. They are UB: the whole program");
    puts("  becomes meaningless, and optimisers really do exploit that.");
    puts("  RULE: at most one modification of an object per expression.");

    puts("\n=== THE TERNARY ===");
    {
        int a = 5, b = 9;
        int max = (a > b) ? a : b;
        printf("  max(%d,%d) = %d\n", a, b, max);
        puts("  ?: is an EXPRESSION, so it works where if/else cannot:");
        printf("  \"%s\"\n", (max > 5) ? "big" : "small");
        puts("  Both branches are converted to a common type — mixing an int and");
        puts("  a double branch yields a double.");
    }

    puts("\n=== COMPOUND ASSIGNMENT EVALUATES THE LEFT SIDE ONCE ===");
    {
        int arr[3] = {0, 0, 0};
        int i = 0;
        arr[i++] += 10;     /* i++ happens once; with arr[i++] = arr[i++] + 10 it would not */
        printf("  arr = {%d,%d,%d}, i = %d\n", arr[0], arr[1], arr[2], i);
    }

    puts("\n=== THE COMMA OPERATOR ===");
    {
        /* Each sub-expression is evaluated in order; the value of the whole
         * is the LAST one. Useful only when the earlier parts have side
         * effects — which is why the compiler warns when they do not. */
        int i = 0, j = 0;
        int x = (i++, j++, i + j);
        printf("  x = (i++, j++, i + j) -> %d   (i=%d, j=%d)\n", x, i, j);
        puts("  Mostly useful in for-loops: for (i = 0, j = n; i < j; i++, j--)");
    }

    puts("\n=== sizeof DOES NOT EVALUATE ITS OPERAND ===");
    {
        int i = 0;
        size_t s = sizeof(i++);      /* i++ never happens */
        printf("  after sizeof(i++): i = %d (unchanged), size = %zu\n", i, s);
        puts("  (Exception: variable-length arrays, where the size IS computed.)");
    }

    return 0;
}
