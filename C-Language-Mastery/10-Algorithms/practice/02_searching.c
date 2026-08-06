/* 02_searching.c — binary search and every variant you actually need.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 02_searching.c -o t && ./t
 *
 * Binary search is four lines and is famously easy to get wrong. Jon Bentley
 * reported that 90% of professional programmers failed to write a correct one
 * given two hours. The overflow bug below sat in the JDK for nine years.
 *
 * Three things go wrong:
 *   1. mid = (lo + hi) / 2  OVERFLOWS for large arrays
 *   2. the loop condition and the update must agree, or it never terminates
 *   3. "find any match" and "find the FIRST match" are different algorithms
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>
#include <stddef.h>

static size_t probes;

/* ================================================================= *
 * LINEAR SEARCH — O(n), but requires nothing of the data.
 * ================================================================= */
static ptrdiff_t linear_search(const int *a, size_t n, int target)
{
    probes = 0;
    for (size_t i = 0; i < n; i++) { probes++; if (a[i] == target) return (ptrdiff_t)i; }
    return -1;
}

/* SENTINEL LINEAR SEARCH: put the target at the end so the loop needs only
 * ONE test per iteration instead of two. A real ~2x on a hot scan, and the
 * classic demonstration that constant factors are worth attention. */
static ptrdiff_t sentinel_search(int *a, size_t n, int target)
{
    int last = a[n - 1];
    a[n - 1] = target;                    /* the sentinel guarantees a hit */

    size_t i = 0;
    while (a[i] != target) i++;           /* no bounds check needed */

    a[n - 1] = last;                      /* restore */
    if (i < n - 1) return (ptrdiff_t)i;
    return (last == target) ? (ptrdiff_t)(n - 1) : -1;
}

/* ================================================================= *
 * BINARY SEARCH — O(log n), requires SORTED data.
 * ================================================================= */

/* THE CANONICAL FORM. Half-open interval [lo, hi): lo is inclusive, hi is
 * exclusive. This is the version to memorise, because the half-open
 * convention makes every off-by-one question answer itself. */
static ptrdiff_t binary_search(const int *a, size_t n, int target)
{
    probes = 0;
    size_t lo = 0, hi = n;                /* [lo, hi) — hi is ONE PAST the end */

    while (lo < hi) {                     /* empty when lo == hi */
        /* NOT (lo + hi) / 2 — see the overflow demonstration below. */
        size_t mid = lo + (hi - lo) / 2;
        probes++;

        if      (a[mid] == target) return (ptrdiff_t)mid;
        else if (a[mid] <  target) lo = mid + 1;   /* discard [lo, mid] */
        else                       hi = mid;       /* discard [mid, hi) */
    }
    return -1;
}

/* LOWER BOUND: the index of the FIRST element >= target.
 * Equivalently: where would you INSERT target to keep the array sorted?
 * This is the most useful variant, and it never returns "not found" — the
 * answer is always a valid insertion point in [0, n]. */
static size_t lower_bound(const int *a, size_t n, int target)
{
    probes = 0;
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        probes++;
        if (a[mid] < target) lo = mid + 1;    /* mid is too small: exclude it */
        else                 hi = mid;        /* mid might be the answer: KEEP it */
    }
    return lo;
}

/* UPPER BOUND: the index of the first element STRICTLY GREATER than target.
 * The only change from lower_bound is `<` becoming `<=`. */
static size_t upper_bound(const int *a, size_t n, int target)
{
    probes = 0;
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        probes++;
        if (a[mid] <= target) lo = mid + 1;
        else                  hi = mid;
    }
    return lo;
}

/* With both bounds you get COUNT and RANGE for free:
 *     count(x)  = upper_bound(x) - lower_bound(x)
 *     range(x)  = [lower_bound(x), upper_bound(x))
 * This is why lower/upper bound are what the C++ standard library exposes
 * rather than a plain "find". */

/* THE BUGGY VERSION, for the demonstration below. */
static ptrdiff_t binary_search_overflowing(const int *a, int lo, int hi, int target)
{
    while (lo <= hi) {
        int mid = (lo + hi) / 2;          /* THE BUG: lo + hi can overflow int */
        if (a[mid] == target) return mid;
        if (a[mid] <  target) lo = mid + 1;
        else                  hi = mid - 1;
    }
    return -1;
}

/* ================================================================= *
 * INTERPOLATION SEARCH — guess WHERE the value should be, rather than
 * always splitting in the middle. For UNIFORMLY distributed data this is
 * O(log log n), which is close to constant. For skewed data it degrades
 * to O(n), which is worse than binary search.
 * ================================================================= */
static ptrdiff_t interpolation_search(const int *a, size_t n, int target)
{
    probes = 0;
    size_t lo = 0, hi = n - 1;

    while (lo <= hi && target >= a[lo] && target <= a[hi]) {
        probes++;
        if (a[hi] == a[lo]) {                       /* avoid dividing by zero */
            return (a[lo] == target) ? (ptrdiff_t)lo : -1;
        }
        /* Linear interpolation: assume the values rise evenly between
         * a[lo] and a[hi], and jump straight to the predicted position. */
        size_t pos = lo + (size_t)(((double)(target - a[lo]) /
                                    (double)(a[hi] - a[lo])) * (double)(hi - lo));
        if (pos > hi) pos = hi;

        if      (a[pos] == target) return (ptrdiff_t)pos;
        else if (a[pos] <  target) lo = pos + 1;
        else                       { if (pos == 0) break; hi = pos - 1; }
    }
    return -1;
}

/* ================================================================= *
 * EXPONENTIAL SEARCH — for an UNBOUNDED or very large sorted range where
 * the target is likely near the start. Double the bound until you overshoot,
 * then binary search the last doubling. O(log i) where i is the target's
 * index — independent of n.
 * ================================================================= */
static ptrdiff_t exponential_search(const int *a, size_t n, int target)
{
    probes = 0;
    if (n == 0) return -1;
    probes++;
    if (a[0] == target) return 0;

    size_t bound = 1;
    while (bound < n && a[bound] <= target) { probes++; bound *= 2; }

    /* Binary search within (bound/2, min(bound, n)) */
    size_t lo = bound / 2, hi = (bound < n) ? bound + 1 : n;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        probes++;
        if      (a[mid] == target) return (ptrdiff_t)mid;
        else if (a[mid] <  target) lo = mid + 1;
        else                       hi = mid;
    }
    return -1;
}

/* ================================================================= *
 * BINARY SEARCH ON AN ANSWER — the technique most people never learn.
 *
 * You do not need an ARRAY. You need a MONOTONIC PREDICATE: something
 * that is false, false, false, true, true, true. Binary search finds the
 * boundary. The "array" can be entirely imaginary.
 * ================================================================= */

/* Integer square root: find the largest x with x*x <= n. */
static long isqrt_bsearch(long n)
{
    probes = 0;
    long lo = 0, hi = n;
    while (lo < hi) {
        long mid = lo + (hi - lo + 1) / 2;      /* round UP to avoid an infinite loop */
        probes++;
        if (mid <= n / (mid == 0 ? 1 : mid)) lo = mid;   /* mid*mid <= n, no overflow */
        else                                 hi = mid - 1;
    }
    return lo;
}

/* A classic: the minimum capacity needed to ship all packages within D days.
 * The predicate "can we do it with capacity C?" is monotonic in C. */
static bool can_ship(const int *weights, size_t n, int capacity, int days)
{
    int used_days = 1, load = 0;
    for (size_t i = 0; i < n; i++) {
        if (weights[i] > capacity) return false;     /* one package will not fit */
        if (load + weights[i] > capacity) { used_days++; load = 0; }
        load += weights[i];
    }
    return used_days <= days;
}
static int min_capacity(const int *weights, size_t n, int days)
{
    int lo = 0, hi = 0;
    for (size_t i = 0; i < n; i++) { hi += weights[i]; if (weights[i] > lo) lo = weights[i]; }

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (can_ship(weights, n, mid, days)) hi = mid;   /* mid works: try smaller */
        else                                 lo = mid + 1;
    }
    return lo;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== LINEAR vs BINARY ===");
    {
        int a[] = {2, 5, 8, 12, 16, 23, 38, 56, 72, 91};
        size_t n = sizeof a / sizeof a[0];

        printf("  array: ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        puts("\n");

        int targets[] = {23, 2, 91, 40};
        for (size_t t = 0; t < 4; t++) {
            ptrdiff_t li = linear_search(a, n, targets[t]);
            size_t lp = probes;
            ptrdiff_t bi = binary_search(a, n, targets[t]);
            printf("  find %2d -> linear: index %2td in %zu probes | "
                   "binary: index %2td in %zu probes\n",
                   targets[t], li, lp, bi, probes);
        }
        puts("\n  Binary search halves the search space each probe, so it needs");
        puts("  at most ceil(log2(n)) probes. For n = 1,000,000 that is 20.");
        puts("  Linear search needs 1,000,000. But binary search requires the");
        puts("  data to be SORTED, and sorting costs O(n log n) — so for a");
        puts("  ONE-OFF search on unsorted data, scan it linearly.");
    }

    puts("\n=== THE OVERFLOW BUG ===");
    puts("      int mid = (lo + hi) / 2;         // WRONG");
    puts("      int mid = lo + (hi - lo) / 2;    // correct");
    {
        int lo = 1073741823, hi = 1073741824;    /* both under INT_MAX */
        printf("  lo = %d, hi = %d  (both valid indices)\n", lo, hi);
        printf("  lo + hi          = %d   <- OVERFLOWED to negative\n", lo + hi);
        printf("  (lo + hi) / 2    = %d   <- a negative index\n", (lo + hi) / 2);
        printf("  lo + (hi-lo) / 2 = %d   <- correct\n", lo + (hi - lo) / 2);
        puts("");
        puts("  a[negative_index] reads outside the array. Signed overflow is");
        puts("  also UNDEFINED BEHAVIOUR, so the optimiser may do anything at all.");
        puts("");
        puts("  This exact bug was in java.util.Arrays.binarySearch for NINE");
        puts("  YEARS, and in the JDK's own merge sort. Joshua Bloch wrote it");
        puts("  up in 2006 under the title 'Nearly All Binary Searches and");
        puts("  Mergesorts Are Broken'. It only triggers above 2^30 elements,");
        puts("  which is exactly why it survived so long.");
        (void)binary_search_overflowing;
    }

    puts("\n=== THE TERMINATION BUG ===");
    puts("  These two must agree:");
    puts("      while (lo < hi)   with  hi = mid       [lo, hi)  half-open");
    puts("      while (lo <= hi)  with  hi = mid - 1   [lo, hi]  closed");
    puts("");
    puts("  Mixing them — `while (lo < hi)` with `hi = mid - 1` — can skip the");
    puts("  answer; `while (lo <= hi)` with `hi = mid` LOOPS FOREVER when");
    puts("  lo == hi == mid and nothing changes.");
    puts("");
    puts("  PICK THE HALF-OPEN FORM AND ALWAYS USE IT. It matches how C thinks");
    puts("  about ranges (begin/end, one-past-the-end), the size is simply");
    puts("  hi - lo, and an empty range is lo == hi. Every off-by-one question");
    puts("  then answers itself.");

    puts("\n=== DUPLICATES: lower_bound AND upper_bound ===");
    {
        int a[] = {1, 3, 3, 3, 5, 7, 7, 9};
        size_t n = sizeof a / sizeof a[0];

        printf("  array: ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        printf("\n  index: ");
        for (size_t i = 0; i < n; i++) printf("%zu ", i);
        puts("\n");

        int targets[] = {3, 7, 5, 4, 0, 10};
        for (size_t t = 0; t < 6; t++) {
            size_t lb = lower_bound(a, n, targets[t]);
            size_t ub = upper_bound(a, n, targets[t]);
            printf("  %2d -> lower_bound %zu, upper_bound %zu, count %zu",
                   targets[t], lb, ub, ub - lb);
            if (ub - lb == 0) printf("   (absent; insert at %zu)", lb);
            puts("");
        }
        puts("");
        puts("  binary_search returns ANY matching index — with duplicates you");
        puts("  cannot predict which. lower_bound returns the FIRST, which is");
        puts("  deterministic and far more useful:");
        puts("      count(x)      = upper_bound(x) - lower_bound(x)");
        puts("      range of x    = [lower_bound(x), upper_bound(x))");
        puts("      insert point  = lower_bound(x)          (always valid)");
        puts("      first >= x    = lower_bound(x)");
        puts("      first > x     = upper_bound(x)");
        puts("");
        puts("  The ONLY difference between them is `<` versus `<=`. That is why");
        puts("  the C++ standard library exposes these two rather than a `find`.");
    }

    puts("\n=== INTERPOLATION AND EXPONENTIAL SEARCH ===");
    {
        const size_t N = 1000000;
        int *uniform = malloc(N * sizeof *uniform);
        int *skewed  = malloc(N * sizeof *skewed);

        for (size_t i = 0; i < N; i++) {
            uniform[i] = (int)(i * 3);                        /* evenly spaced */
            skewed[i]  = (int)(i < N - 10 ? i : i * 1000);    /* a long tail */
        }

        int target = uniform[N / 3];
        binary_search(uniform, N, target);        size_t p_bin = probes;
        interpolation_search(uniform, N, target); size_t p_int = probes;
        printf("  UNIFORM data, %zu elements:\n", N);
        printf("    binary        : %2zu probes\n", p_bin);
        printf("    interpolation : %2zu probes   <- it GUESSES the position\n", p_int);

        target = skewed[N - 5];
        binary_search(skewed, N, target);         p_bin = probes;
        interpolation_search(skewed, N, target);  p_int = probes;
        printf("  SKEWED data (long tail):\n");
        printf("    binary        : %2zu probes   <- unchanged, always log2(n)\n", p_bin);
        printf("    interpolation : %2zu probes   <- its guess is now useless\n", p_int);

        target = uniform[20];
        binary_search(uniform, N, target);        p_bin = probes;
        exponential_search(uniform, N, target);   size_t p_exp = probes;
        printf("  target near the START of the array:\n");
        printf("    binary        : %2zu probes\n", p_bin);
        printf("    exponential   : %2zu probes   <- O(log i), not O(log n)\n", p_exp);

        puts("");
        puts("  INTERPOLATION: assumes values rise EVENLY and jumps to the");
        puts("  predicted spot. O(log log n) on uniform data — about 5 probes for");
        puts("  a million elements. Degrades to O(n) when the assumption fails,");
        puts("  so it is only safe when you KNOW the distribution.");
        puts("");
        puts("  EXPONENTIAL: doubles the bound until it overshoots, then binary");
        puts("  searches the last doubling. O(log i) where i is the target's");
        puts("  index — so it does not depend on n at all. Use it for unbounded");
        puts("  or streaming sorted data, or when hits cluster near the start.");

        free(uniform); free(skewed);
    }

    puts("\n=== THE CONSTANT FACTOR: SENTINEL LINEAR SEARCH ===");
    {
        const size_t N = 100000;
        int *a = malloc(N * sizeof *a);
        for (size_t i = 0; i < N; i++) a[i] = (int)i;

        const int REPS = 2000;
        clock_t t = clock();
        volatile ptrdiff_t sink = 0;
        for (int r = 0; r < REPS; r++) sink += linear_search(a, N, (int)(N - 1));
        double t_plain = seconds_since(t);

        t = clock();
        for (int r = 0; r < REPS; r++) sink += sentinel_search(a, N, (int)(N - 1));
        double t_sent = seconds_since(t);

        printf("  %d scans of %zu elements (worst case, target at the end):\n", REPS, N);
        printf("    plain    : %.4f s   (two tests per iteration: i < n, and a[i] == target)\n", t_plain);
        printf("    sentinel : %.4f s   (ONE test: a[i] == target)\n", t_sent);
        printf("    speedup  : %.2fx — same O(n), half the work per element\n",
               t_plain / t_sent);
        puts("  Writing the target into the last slot GUARANTEES a hit, so the");
        puts("  bounds check becomes unnecessary. Same complexity, real speedup.");
        puts("  (It does write to the array, so it cannot take a const pointer");
        puts("   and is not safe to call concurrently.)");
        free(a);
    }

    puts("\n=== BINARY SEARCH WITHOUT AN ARRAY ===");
    {
        puts("  You do not need an array. You need a MONOTONIC PREDICATE —");
        puts("  something that is false, false, false, TRUE, TRUE. Binary search");
        puts("  finds the boundary. The 'array' can be entirely imaginary.");
        puts("");

        long vals[] = {0, 1, 15, 16, 17, 1000000, 999999999999L};
        printf("  integer square root by binary search:\n");
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++) {
            long r = isqrt_bsearch(vals[i]);
            printf("    isqrt(%13ld) = %-9ld  (%ld probes; check: %ld^2 = %ld <= %ld)\n",
                   vals[i], r, probes, r, r * r, vals[i]);
        }

        puts("\n  minimum ship capacity to deliver all packages in D days:");
        int weights[] = {1,2,3,4,5,6,7,8,9,10};
        size_t n = sizeof weights / sizeof weights[0];
        for (int days = 1; days <= 5; days++)
            printf("    %d day(s) -> capacity %d\n", days, min_capacity(weights, n, days));
        puts("      The predicate 'can we ship with capacity C?' is monotonic:");
        puts("      if C works, so does C+1. So binary search the ANSWER SPACE,");
        puts("      testing feasibility instead of comparing array elements.");
        puts("");
        puts("  THIS IS THE MOST UNDER-USED ALGORITHM TECHNIQUE. Whenever you");
        puts("  can ask a yes/no question whose answer is monotonic in some");
        puts("  parameter, you can binary search that parameter — even if there");
        puts("  is no array anywhere in the problem.");
    }

    puts("\n=== SUMMARY ===");
    puts("  linear         O(n)        needs nothing; best for small or unsorted n");
    puts("  sentinel       O(n)        ~2x faster linear; writes to the array");
    puts("  binary         O(log n)    needs sorted data; THE default");
    puts("  lower/upper    O(log n)    handles duplicates; gives insert points");
    puts("  interpolation  O(log log n) uniform data only; O(n) otherwise");
    puts("  exponential    O(log i)    unbounded ranges; targets near the start");
    puts("  on the answer  O(log R)    any monotonic predicate over a range R");
    puts("");
    puts("  C provides bsearch() in <stdlib.h>. Use it — but it returns ANY");
    puts("  match, so implement lower_bound yourself when duplicates matter.");

    return 0;
}
