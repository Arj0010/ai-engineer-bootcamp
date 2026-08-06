/* 03_dynamic_programming.c — DP: when it applies, and both ways to write it.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 03_dynamic_programming.c -o t && ./t
 *
 * DP APPLIES WHEN TWO THINGS ARE TRUE:
 *   1. OPTIMAL SUBSTRUCTURE   — the best solution is built from best
 *                               solutions to subproblems
 *   2. OVERLAPPING SUBPROBLEMS — the same subproblem recurs many times
 *
 * If only (1) holds and each subproblem is distinct, you have divide and
 * conquer (merge sort). If (2) holds too, caching turns exponential into
 * polynomial. That is the entire idea.
 *
 * TWO FORMS:
 *   TOP-DOWN  (memoisation) — write the recursion, cache the answers.
 *                             Easier to derive; only computes what it needs.
 *   BOTTOM-UP (tabulation)  — fill a table in dependency order.
 *                             No recursion, better constants, and it often
 *                             lets you shrink the table to a single row.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

static size_t calls;

static int max_i(int a, int b) { return a > b ? a : b; }
static int min_i(int a, int b) { return a < b ? a : b; }

/* ================================================================= *
 * 1. FIBONACCI — the canonical demonstration
 * ================================================================= */
static long long fib_naive(int n)
{
    calls++;
    if (n < 2) return n;
    return fib_naive(n - 1) + fib_naive(n - 2);       /* recomputes everything */
}

static long long fib_memo_rec(int n, long long *memo)
{
    calls++;
    if (n < 2) return n;
    if (memo[n] != -1) return memo[n];                /* already solved */
    return memo[n] = fib_memo_rec(n - 1, memo) + fib_memo_rec(n - 2, memo);
}
static long long fib_memo(int n)
{
    long long *memo = malloc((size_t)(n + 1) * sizeof *memo);
    for (int i = 0; i <= n; i++) memo[i] = -1;
    long long r = fib_memo_rec(n, memo);
    free(memo);
    return r;
}

static long long fib_table(int n)
{
    if (n < 2) return n;
    long long *dp = malloc((size_t)(n + 1) * sizeof *dp);
    dp[0] = 0; dp[1] = 1;
    for (int i = 2; i <= n; i++) { calls++; dp[i] = dp[i-1] + dp[i-2]; }
    long long r = dp[n];
    free(dp);
    return r;
}

/* SPACE OPTIMISATION: dp[i] only ever reads dp[i-1] and dp[i-2], so the
 * whole table collapses to two variables. This "keep only the rows you
 * still need" step applies to a great many DP problems. */
static long long fib_rolling(int n)
{
    long long a = 0, b = 1;
    for (int i = 0; i < n; i++) { calls++; long long t = a + b; a = b; b = t; }
    return a;
}

/* ================================================================= *
 * 2. 0/1 KNAPSACK — the archetypal DP
 *
 * n items with weights and values, a bag of capacity W. Each item is taken
 * WHOLE or not at all. Maximise the value.
 *
 * RECURRENCE: for each item, either skip it or take it.
 *   dp[i][w] = max( dp[i-1][w],                          skip item i
 *                   dp[i-1][w - wt[i]] + val[i] )        take item i
 * ================================================================= */
static int knapsack_table(const int *wt, const int *val, int n, int W, bool verbose)
{
    /* (n+1) x (W+1). Row i means "considering the first i items". */
    int **dp = malloc((size_t)(n + 1) * sizeof *dp);
    for (int i = 0; i <= n; i++) dp[i] = calloc((size_t)(W + 1), sizeof *dp[i]);

    for (int i = 1; i <= n; i++) {
        for (int w = 0; w <= W; w++) {
            dp[i][w] = dp[i - 1][w];                      /* skip item i-1 */
            if (wt[i - 1] <= w)                           /* can we afford it? */
                dp[i][w] = max_i(dp[i][w], dp[i - 1][w - wt[i - 1]] + val[i - 1]);
        }
    }
    int best = dp[n][W];

    if (verbose) {
        /* RECONSTRUCT which items were chosen by walking the table BACKWARDS.
         * dp[i][w] != dp[i-1][w] means item i-1 must have been taken. */
        printf("    chosen items: ");
        int w = W;
        for (int i = n; i > 0; i--) {
            if (dp[i][w] != dp[i - 1][w]) {
                printf("(wt %d, val %d) ", wt[i - 1], val[i - 1]);
                w -= wt[i - 1];
            }
        }
        puts("");
    }

    for (int i = 0; i <= n; i++) free(dp[i]);
    free(dp);
    return best;
}

/* SPACE-OPTIMISED: row i only reads row i-1, so ONE row suffices —
 * provided you iterate w BACKWARDS. Going forwards would let the same item
 * be used twice (that is exactly the UNBOUNDED knapsack). */
static int knapsack_1d(const int *wt, const int *val, int n, int W)
{
    int *dp = calloc((size_t)(W + 1), sizeof *dp);
    for (int i = 0; i < n; i++)
        for (int w = W; w >= wt[i]; w--)                  /* BACKWARDS */
            dp[w] = max_i(dp[w], dp[w - wt[i]] + val[i]);
    int best = dp[W];
    free(dp);
    return best;
}

/* ================================================================= *
 * 3. LONGEST COMMON SUBSEQUENCE — the basis of diff
 * ================================================================= */
static int lcs(const char *a, const char *b, char *out, size_t outsize)
{
    size_t m = strlen(a), n = strlen(b);
    int **dp = malloc((m + 1) * sizeof *dp);
    for (size_t i = 0; i <= m; i++) dp[i] = calloc(n + 1, sizeof *dp[i]);

    for (size_t i = 1; i <= m; i++)
        for (size_t j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1])
                     ? dp[i-1][j-1] + 1                   /* characters match */
                     : max_i(dp[i-1][j], dp[i][j-1]);     /* drop one or the other */

    int len = dp[m][n];

    /* Reconstruct, walking backwards from the corner. */
    if (out != NULL && outsize > 0) {
        size_t pos = (size_t)len;
        if (pos < outsize) out[pos] = '\0';
        size_t i = m, j = n;
        while (i > 0 && j > 0 && pos > 0) {
            if (a[i-1] == b[j-1])            { out[--pos] = a[i-1]; i--; j--; }
            else if (dp[i-1][j] >= dp[i][j-1]) i--;
            else                               j--;
        }
    }

    for (size_t i = 0; i <= m; i++) free(dp[i]);
    free(dp);
    return len;
}

/* ================================================================= *
 * 4. EDIT DISTANCE (Levenshtein) — spell checkers, DNA alignment, diff
 *
 * The minimum number of insert/delete/substitute operations to turn a
 * into b.
 *   dp[i][j] = if a[i-1]==b[j-1]: dp[i-1][j-1]
 *              else 1 + min(dp[i-1][j],    delete
 *                           dp[i][j-1],    insert
 *                           dp[i-1][j-1])  substitute
 * ================================================================= */
static int edit_distance(const char *a, const char *b, bool show_table)
{
    size_t m = strlen(a), n = strlen(b);
    int **dp = malloc((m + 1) * sizeof *dp);
    for (size_t i = 0; i <= m; i++) dp[i] = malloc((n + 1) * sizeof *dp[i]);

    /* BASE CASES: turning a prefix into the empty string costs one delete
     * per character, and vice versa. */
    for (size_t i = 0; i <= m; i++) dp[i][0] = (int)i;
    for (size_t j = 0; j <= n; j++) dp[0][j] = (int)j;

    for (size_t i = 1; i <= m; i++)
        for (size_t j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1])
                     ? dp[i-1][j-1]
                     : 1 + min_i(dp[i-1][j-1], min_i(dp[i-1][j], dp[i][j-1]));

    int d = dp[m][n];

    if (show_table) {
        printf("        ");
        for (size_t j = 0; j < n; j++) printf(" %c", b[j]);
        puts("");
        for (size_t i = 0; i <= m; i++) {
            printf("      %c ", i == 0 ? ' ' : a[i-1]);
            for (size_t j = 0; j <= n; j++) printf("%2d", dp[i][j]);
            puts("");
        }
    }

    for (size_t i = 0; i <= m; i++) free(dp[i]);
    free(dp);
    return d;
}

/* ================================================================= *
 * 5. COIN CHANGE — minimum coins, and the number of ways
 * ================================================================= */
static int coin_change_min(const int *coins, int n, int amount)
{
    int *dp = malloc((size_t)(amount + 1) * sizeof *dp);
    dp[0] = 0;
    for (int a = 1; a <= amount; a++) dp[a] = INT_MAX;

    for (int a = 1; a <= amount; a++)
        for (int c = 0; c < n; c++)
            if (coins[c] <= a && dp[a - coins[c]] != INT_MAX)
                dp[a] = min_i(dp[a], dp[a - coins[c]] + 1);

    int r = (dp[amount] == INT_MAX) ? -1 : dp[amount];
    free(dp);
    return r;
}

/* COUNTING ways rather than minimising. The LOOP ORDER is what distinguishes
 * combinations from permutations here, and getting it backwards is the
 * classic bug: coins OUTSIDE counts each combination once; coins INSIDE
 * would count 1+2 and 2+1 separately. */
static long long coin_change_ways(const int *coins, int n, int amount)
{
    long long *dp = calloc((size_t)(amount + 1), sizeof *dp);
    dp[0] = 1;                                   /* one way to make 0: take nothing */

    for (int c = 0; c < n; c++)                  /* coin loop OUTSIDE */
        for (int a = coins[c]; a <= amount; a++)
            dp[a] += dp[a - coins[c]];

    long long r = dp[amount];
    free(dp);
    return r;
}

/* ================================================================= *
 * 6. LONGEST INCREASING SUBSEQUENCE — O(n^2) DP and the O(n log n) trick
 * ================================================================= */
static int lis_dp(const int *a, int n, int *out, int *out_len)
{
    int *dp   = malloc((size_t)n * sizeof *dp);      /* LIS ending AT i */
    int *prev = malloc((size_t)n * sizeof *prev);    /* for reconstruction */

    int best = 0, best_i = 0;
    for (int i = 0; i < n; i++) {
        dp[i] = 1; prev[i] = -1;
        for (int j = 0; j < i; j++)
            if (a[j] < a[i] && dp[j] + 1 > dp[i]) { dp[i] = dp[j] + 1; prev[i] = j; }
        if (dp[i] > best) { best = dp[i]; best_i = i; }
    }

    if (out && out_len) {                        /* walk the prev chain back */
        int len = 0;
        for (int i = best_i; i != -1; i = prev[i]) out[len++] = a[i];
        for (int i = 0; i < len / 2; i++) { int t = out[i]; out[i] = out[len-1-i]; out[len-1-i] = t; }
        *out_len = len;
    }

    free(dp); free(prev);
    return best;
}

/* O(n log n): keep `tails[k]` = the SMALLEST possible tail of an increasing
 * subsequence of length k+1. That array is sorted, so each element can be
 * placed with a binary search. `tails` is NOT the LIS itself — only its
 * LENGTH is meaningful — which surprises people. */
static int lis_binary(const int *a, int n)
{
    int *tails = malloc((size_t)n * sizeof *tails);
    int len = 0;

    for (int i = 0; i < n; i++) {
        int lo = 0, hi = len;                    /* lower_bound over tails */
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            if (tails[mid] < a[i]) lo = mid + 1;
            else                   hi = mid;
        }
        tails[lo] = a[i];                        /* extend, or improve a tail */
        if (lo == len) len++;
    }
    free(tails);
    return len;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== WHEN DOES DP APPLY? ===");
    puts("  1. OPTIMAL SUBSTRUCTURE: the best answer is built from the best");
    puts("     answers to subproblems.");
    puts("  2. OVERLAPPING SUBPROBLEMS: the same subproblem comes up again");
    puts("     and again.");
    puts("");
    puts("  Only (1)?  -> divide and conquer (merge sort: each half is distinct)");
    puts("  Both?      -> DP. Caching turns exponential into polynomial.");
    puts("  Neither?   -> you need a different technique entirely.\n");

    puts("=== 1. FIBONACCI: THE SAME ANSWER AT FOUR COSTS ===");
    {
        const int n = 35;
        clock_t t;

        calls = 0; t = clock();
        long long r1 = fib_naive(n);
        double d1 = seconds_since(t); size_t c1 = calls;

        calls = 0; t = clock();
        long long r2 = fib_memo(n);
        double d2 = seconds_since(t); size_t c2 = calls;

        calls = 0; t = clock();
        long long r3 = fib_table(n);
        double d3 = seconds_since(t); size_t c3 = calls;

        calls = 0; t = clock();
        long long r4 = fib_rolling(n);
        double d4 = seconds_since(t); size_t c4 = calls;

        printf("  fib(%d) = %lld everywhere\n\n", n, r1);
        printf("    %-24s %10.6f s  %12zu steps  O(2^n) time, O(n) space\n",
               "naive recursion", d1, c1);
        printf("    %-24s %10.6f s  %12zu steps  O(n) time, O(n) space\n",
               "top-down memoised", d2, c2);
        printf("    %-24s %10.6f s  %12zu steps  O(n) time, O(n) space\n",
               "bottom-up table", d3, c3);
        printf("    %-24s %10.6f s  %12zu steps  O(n) time, O(1) SPACE\n",
               "rolling variables", d4, c4);
        (void)r2; (void)r3; (void)r4;
        puts("");
        puts("  The naive version calls itself 30 million times to compute 36");
        puts("  distinct values. fib(2) alone is recomputed about 9 million times.");
        puts("  Memoising is a THREE-LINE change and removes all of it.");
        puts("");
        puts("  The rolling version is the last step: dp[i] only ever reads");
        puts("  dp[i-1] and dp[i-2], so the whole table collapses to two");
        puts("  variables. Look for this in every bottom-up DP you write.");
    }

    puts("\n=== 2. 0/1 KNAPSACK ===");
    {
        int wt[]  = {1, 3, 4, 5, 7};
        int val[] = {1, 4, 5, 7, 9};
        int n = 5, W = 10;

        printf("  items (weight, value): ");
        for (int i = 0; i < n; i++) printf("(%d,%d) ", wt[i], val[i]);
        printf("\n  capacity %d\n", W);

        int best = knapsack_table(wt, val, n, W, true);
        printf("    best value: %d\n", best);
        printf("    1-D space-optimised gives: %d (same)\n", knapsack_1d(wt, val, n, W));

        puts("\n  THE RECURRENCE — for each item, two choices:");
        puts("      dp[i][w] = max( dp[i-1][w],                     SKIP item i");
        puts("                      dp[i-1][w-wt[i]] + val[i] )     TAKE item i");
        puts("");
        puts("  The 1-D version keeps ONE row and iterates w BACKWARDS. Going");
        puts("  forwards would read a value already updated for THIS item, which");
        puts("  lets the item be used twice — that is the UNBOUNDED knapsack.");
        puts("  One loop direction is the entire difference between two problems.");
        puts("");
        puts("  Note: knapsack is O(n*W), which is PSEUDO-polynomial. W is a");
        puts("  VALUE, not an input size, so a capacity of 10^9 is intractable");
        puts("  even with only 5 items. The problem is NP-hard; DP only wins");
        puts("  when W is small.");
    }

    puts("\n=== 3. LONGEST COMMON SUBSEQUENCE ===");
    {
        struct { const char *a, *b; } pairs[] = {
            {"ABCBDAB", "BDCABA"}, {"AGGTAB", "GXTXAYB"}, {"kitten", "sitting"},
        };
        for (size_t i = 0; i < 3; i++) {
            char out[64];
            int len = lcs(pairs[i].a, pairs[i].b, out, sizeof out);
            printf("  LCS(\"%s\", \"%s\") = %d  -> \"%s\"\n",
                   pairs[i].a, pairs[i].b, len, out);
        }
        puts("");
        puts("  A SUBSEQUENCE is not a SUBSTRING: it need not be contiguous.");
        puts("  LCS is the core of `diff`: the lines NOT in the LCS are exactly");
        puts("  the additions and deletions. It is also the basis of git's diff,");
        puts("  and of similarity scoring in bioinformatics.");
    }

    puts("\n=== 4. EDIT DISTANCE ===");
    {
        printf("  edit_distance(\"kitten\", \"sitting\"):\n");
        int d = edit_distance("kitten", "sitting", true);
        printf("      = %d  (k->s, e->i, and insert g)\n", d);

        struct { const char *a, *b; } pairs[] = {
            {"sunday","saturday"}, {"", "abc"}, {"abc","abc"}, {"flaw","lawn"},
        };
        for (size_t i = 0; i < 4; i++)
            printf("  \"%s\" -> \"%s\" : %d\n",
                   pairs[i].a, pairs[i].b, edit_distance(pairs[i].a, pairs[i].b, false));
        puts("");
        puts("  Each cell asks: what is the cheapest way to turn a[0..i) into");
        puts("  b[0..j)? Three moves are available — delete, insert, substitute —");
        puts("  and each corresponds to one neighbouring cell. The answer is in");
        puts("  the bottom-right corner.");
        puts("  Used by: spell checkers, fuzzy search, DNA sequence alignment,");
        puts("  plagiarism detection, and 'did you mean...?' suggestions.");
    }

    puts("\n=== 5. COIN CHANGE ===");
    {
        int us[]  = {1, 5, 10, 25};
        int odd[] = {1, 3, 4};

        printf("  US coins {1,5,10,25}:\n");
        for (int amt = 30; amt <= 63; amt += 11)
            printf("    %2d cents: %d coins minimum, %lld distinct ways\n",
                   amt, coin_change_min(us, 4, amt), coin_change_ways(us, 4, amt));

        printf("  coins {1,3,4}, amount 6:\n");
        printf("    DP says %d coins (3 + 3)\n", coin_change_min(odd, 3, 6));
        printf("    GREEDY would take 4, then 1, then 1 = 3 coins — WRONG.\n");
        puts("    This is the standard demonstration that greedy is not DP.");
        puts("    Greedy is optimal for {1,5,10,25} and wrong for {1,3,4};");
        puts("    DP is correct for both because it considers every choice.");

        int impossible[] = {5, 10};
        printf("  coins {5,10}, amount 3: %d (correctly impossible)\n",
               coin_change_min(impossible, 2, 3));

        puts("\n  THE LOOP ORDER MATTERS in the counting version:");
        puts("    coins OUTSIDE, amounts inside -> COMBINATIONS (1+2 == 2+1)");
        puts("    amounts outside, coins inside -> PERMUTATIONS (counted twice)");
        puts("  Two nearly identical loops solving genuinely different problems.");
    }

    puts("\n=== 6. LONGEST INCREASING SUBSEQUENCE ===");
    {
        int a[] = {10, 9, 2, 5, 3, 7, 101, 18, 4, 8, 12};
        int n = (int)(sizeof a / sizeof a[0]);

        printf("  input: ");
        for (int i = 0; i < n; i++) printf("%d ", a[i]);
        puts("");

        int out[32], out_len = 0;
        int len_dp  = lis_dp(a, n, out, &out_len);
        int len_bin = lis_binary(a, n);

        printf("  O(n^2) DP        : length %d -> ", len_dp);
        for (int i = 0; i < out_len; i++) printf("%d ", out[i]);
        puts("");
        printf("  O(n log n) patience: length %d (lengths agree: %s)\n",
               len_bin, len_dp == len_bin ? "yes" : "NO");

        /* Show the asymptotic difference on real input. */
        const int N = 20000;
        int *big = malloc((size_t)N * sizeof *big);
        unsigned seed = 7;
        for (int i = 0; i < N; i++) { seed = seed * 1103515245u + 12345u; big[i] = (int)(seed >> 16); }

        clock_t t = clock();
        int r1 = lis_dp(big, N, NULL, NULL);
        double t1 = seconds_since(t);

        t = clock();
        int r2 = lis_binary(big, N);
        double t2 = seconds_since(t);

        printf("\n  n = %d random values (both give %d / %d):\n", N, r1, r2);
        printf("    O(n^2)     : %.4f s\n", t1);
        printf("    O(n log n) : %.4f s   (%.0fx faster)\n", t2, t1 / t2);
        free(big);

        puts("");
        puts("  The O(n log n) version keeps tails[k] = the SMALLEST possible");
        puts("  tail of an increasing subsequence of length k+1. That array is");
        puts("  always sorted, so each element is placed with a binary search.");
        puts("  CAVEAT: `tails` is not the LIS itself — only its LENGTH is");
        puts("  meaningful. Reconstructing the actual sequence needs a separate");
        puts("  parent array. This surprises nearly everyone the first time.");
    }

    puts("\n=== HOW TO DERIVE A DP SOLUTION ===");
    puts("  1. Define the STATE. What does dp[i] or dp[i][j] mean, EXACTLY?");
    puts("     Write it as an English sentence. This is 80% of the work, and");
    puts("     a vague state definition is why most DP attempts fail.");
    puts("  2. Write the RECURRENCE. How does this state depend on smaller ones?");
    puts("  3. Identify the BASE CASES. The smallest states, answered directly.");
    puts("  4. Determine the ORDER. Every state must be computed after the");
    puts("     states it depends on.");
    puts("  5. Find the ANSWER. Which cell holds it? Often not the last one.");
    puts("  6. OPTIMISE SPACE. If dp[i] only reads dp[i-1], keep one row.");
    puts("");
    puts("  TOP-DOWN vs BOTTOM-UP:");
    puts("    top-down  : easier to write (mirror the recursion), only computes");
    puts("                reachable states, but pays call overhead and can");
    puts("                overflow the stack");
    puts("    bottom-up : better constants, no recursion, and it is the form");
    puts("                that permits the space optimisation");
    puts("    Derive it top-down, then convert if the constants matter.");

    return 0;
}
