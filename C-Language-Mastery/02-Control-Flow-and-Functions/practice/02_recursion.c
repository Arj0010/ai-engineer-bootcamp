/* 02_recursion.c — recursion, its cost, and when to stop using it.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 02_recursion.c -o t && ./t
 *
 * Compile at -O0 too and compare the Fibonacci timings. The gap between
 * naive and memoised does not close; the gap between recursive-tail and
 * iterative does, because -O2 turns the tail call into a loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* --------------------------------------------------------------- *
 * 1. Factorial — the textbook shape: base case + strictly smaller step.
 * --------------------------------------------------------------- */
static unsigned long long factorial(unsigned n)
{
    if (n <= 1) return 1;                 /* BASE CASE — without it, infinite */
    return n * factorial(n - 1);          /* argument strictly decreases */
}

/* The same thing iteratively. Constant stack, and honestly clearer. */
static unsigned long long factorial_iter(unsigned n)
{
    unsigned long long acc = 1;
    for (unsigned i = 2; i <= n; i++) acc *= i;
    return acc;
}

/* --------------------------------------------------------------- *
 * 2. Fibonacci — the standard demonstration that recursion can be a
 *    catastrophically bad algorithm, not just a slow one.
 * --------------------------------------------------------------- */

/* O(phi^n) ~ O(1.618^n). fib(40) makes ~331 million calls. */
static long long fib_naive(int n)
{
    if (n < 2) return n;
    return fib_naive(n - 1) + fib_naive(n - 2);   /* recomputes everything */
}

/* O(n): remember what you already worked out. Same recursion, one array. */
static long long fib_memo_helper(int n, long long *memo)
{
    if (n < 2) return n;
    if (memo[n] != -1) return memo[n];             /* already known */
    memo[n] = fib_memo_helper(n - 1, memo) + fib_memo_helper(n - 2, memo);
    return memo[n];
}
static long long fib_memo(int n)
{
    long long *memo = malloc((size_t)(n + 1) * sizeof *memo);
    if (memo == NULL) return -1;
    for (int i = 0; i <= n; i++) memo[i] = -1;
    long long r = fib_memo_helper(n, memo);
    free(memo);
    return r;
}

/* O(n) time, O(1) space. No recursion, no allocation. Usually the right answer. */
static long long fib_iter(int n)
{
    long long a = 0, b = 1;
    for (int i = 0; i < n; i++) { long long t = a + b; a = b; b = t; }
    return a;
}

/* TAIL recursion: the recursive call is the LAST operation, so nothing needs
 * to be remembered after it returns. GCC/Clang at -O2 turn this into a jump,
 * using constant stack. C does NOT guarantee this — never depend on it. */
static long long fib_tail(int n, long long a, long long b)
{
    if (n == 0) return a;
    return fib_tail(n - 1, b, a + b);
}

/* --------------------------------------------------------------- *
 * 3. Towers of Hanoi — a problem where recursion IS the clean solution.
 *    Moves required: 2^n - 1, which is provably optimal.
 * --------------------------------------------------------------- */
static int hanoi_moves = 0;
static void hanoi(int n, char from, char to, char via, int verbose)
{
    if (n == 0) return;
    hanoi(n - 1, from, via, to, verbose);      /* move n-1 out of the way */
    hanoi_moves++;
    if (verbose) printf("    disk %d: %c -> %c\n", n, from, to);
    hanoi(n - 1, via, to, from, verbose);      /* bring them back on top */
}

/* --------------------------------------------------------------- *
 * 4. Binary search, recursive and iterative — same algorithm, and the
 *    iterative version is what you should actually write.
 * --------------------------------------------------------------- */
static int bsearch_rec(const int *a, int lo, int hi, int target)
{
    if (lo > hi) return -1;
    int mid = lo + (hi - lo) / 2;      /* NOT (lo+hi)/2 — that can overflow */
    if (a[mid] == target) return mid;
    if (a[mid] <  target) return bsearch_rec(a, mid + 1, hi, target);
    return                       bsearch_rec(a, lo, mid - 1, target);
}
static int bsearch_iter(const int *a, int n, int target)
{
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (a[mid] == target) return mid;
        if (a[mid] <  target) lo = mid + 1;
        else                  hi = mid - 1;
    }
    return -1;
}

/* --------------------------------------------------------------- *
 * 5. Mutual recursion — two functions calling each other. Needs a
 *    forward declaration, since C reads the file top to bottom.
 * --------------------------------------------------------------- */
static int is_odd(unsigned n);
static int is_even(unsigned n) { return (n == 0) ? 1 : is_odd(n - 1); }
static int is_odd (unsigned n) { return (n == 0) ? 0 : is_even(n - 1); }

/* --------------------------------------------------------------- *
 * 6. Recursion depth: how deep can you actually go?
 * --------------------------------------------------------------- */
static size_t depth_reached = 0;
static void descend(size_t depth, size_t limit)
{
    char frame_padding[512];        /* make each frame chunky on purpose */
    memset(frame_padding, 0, sizeof frame_padding);
    depth_reached = depth;
    if (depth < limit) descend(depth + 1, limit);
    /* keep the padding alive so the compiler cannot optimise the frame away */
    if (frame_padding[0] != 0) puts("unreachable");
}

static double seconds_since(clock_t start)
{
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

int main(void)
{
    puts("=== factorial: recursive vs iterative ===");
    for (unsigned n = 0; n <= 20; n += 5)
        printf("  %2u! = %20llu   (iter: %llu)\n", n, factorial(n), factorial_iter(n));
    puts("  21! overflows unsigned long long. Recursion is not the limit here — the type is.");

    puts("\n=== fibonacci: the SAME answer at wildly different costs ===");
    {
        const int n = 35;
        clock_t t;

        t = clock();
        long long r1 = fib_naive(n);
        double d1 = seconds_since(t);

        t = clock();
        long long r2 = fib_memo(n);
        double d2 = seconds_since(t);

        t = clock();
        long long r3 = fib_iter(n);
        double d3 = seconds_since(t);

        t = clock();
        long long r4 = fib_tail(n, 0, 1);
        double d4 = seconds_since(t);

        printf("  fib(%d) = %lld everywhere\n", n, r1);
        printf("  naive     O(1.618^n)  %10.6f s   %lld\n", d1, r1);
        printf("  memoised  O(n)        %10.6f s   %lld\n", d2, r2);
        printf("  iterative O(n), O(1)  %10.6f s   %lld\n", d3, r3);
        printf("  tail-rec  O(n)        %10.6f s   %lld\n", d4, r4);
        puts("  The naive version recomputes fib(2) about 9 million times.");
        puts("  Recursion is not slow. Recomputing the same subproblem is.");
    }

    puts("\n=== towers of hanoi: recursion as the natural formulation ===");
    hanoi_moves = 0;
    hanoi(3, 'A', 'C', 'B', 1);
    printf("  3 disks: %d moves (2^3 - 1 = 7, and that is optimal)\n", hanoi_moves);
    for (int n = 10; n <= 20; n += 5) {
        hanoi_moves = 0;
        hanoi(n, 'A', 'C', 'B', 0);
        printf("  %2d disks: %8d moves\n", n, hanoi_moves);
    }

    puts("\n=== binary search ===");
    {
        int a[] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19};
        int n = (int)(sizeof a / sizeof a[0]);
        for (int target = 1; target <= 20; target += 6)
            printf("  find %2d -> recursive: %2d   iterative: %2d\n",
                   target, bsearch_rec(a, 0, n - 1, target), bsearch_iter(a, n, target));
        puts("  Note `mid = lo + (hi - lo) / 2`. Writing (lo + hi) / 2 overflows");
        puts("  for large arrays — a bug that sat in the JDK for nine years.");
    }

    puts("\n=== mutual recursion ===");
    for (unsigned n = 0; n < 5; n++)
        printf("  %u is %s\n", n, is_even(n) ? "even" : "odd");
    puts("  is_even needs a forward declaration of is_odd. C compiles top-down.");

    puts("\n=== how deep can recursion go? ===");
    {
        /* Stay well under the 8 MB default stack: 512-byte frames * 4000 = 2 MB. */
        depth_reached = 0;
        descend(0, 4000);
        printf("  reached depth %zu with ~512-byte frames (~%zu KB of stack)\n",
               depth_reached, depth_reached * 512 / 1024);
        puts("  Default stack on Linux is 8 MB (`ulimit -s`). Exceed it and you get");
        puts("  SIGSEGV with no useful message — the classic stack overflow.");
        puts("  If depth depends on INPUT SIZE, convert to a loop with an explicit");
        puts("  stack (see module 09) rather than trusting the call stack.");
    }

    puts("\n=== when to use recursion ===");
    puts("  YES: tree/graph traversal, divide & conquer, parsers, backtracking —");
    puts("       anywhere the data structure is itself recursive.");
    puts("  NO : simple iteration over a sequence; anything where depth scales");
    puts("       with input size and the input is unbounded.");
    puts("  ALWAYS: prove the base case is reachable before you run it.");

    return 0;
}
