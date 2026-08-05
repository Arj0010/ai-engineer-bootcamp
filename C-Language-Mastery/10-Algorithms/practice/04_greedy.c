/* 04_greedy.c — take the best local option and never look back.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 04_greedy.c -o t && ./t
 *
 * Greedy algorithms are much simpler and faster than DP — and WRONG unless
 * the problem has the GREEDY-CHOICE PROPERTY: a locally optimal choice is
 * always part of some globally optimal solution.
 *
 * That property has to be PROVED. "It looks right on my examples" is how
 * people ship broken greedy code. This file includes worked cases where
 * greedy is provably optimal, and cases where it is provably not.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

static int max_i(int a, int b) { return a > b ? a : b; }

/* ================================================================= *
 * 1. ACTIVITY SELECTION — greedy IS optimal, and provably so.
 *
 * Given activities with start/finish times, pick the most that do not
 * overlap. Sort by FINISH time and take greedily.
 *
 * THE PROOF (exchange argument): let the greedy choice be the activity
 * finishing earliest, g. Take any optimal solution O. If O does not contain
 * g, swap O's first activity for g — g finishes no later, so nothing else
 * in O conflicts, and |O| is unchanged. So there is an optimal solution
 * containing g. Recurse on what remains.
 * ================================================================= */
typedef struct { int start, finish; char name[12]; } Activity;

static int cmp_by_finish(const void *a, const void *b)
{
    const Activity *x = a, *y = b;
    return (x->finish > y->finish) - (x->finish < y->finish);
}
static int cmp_by_start(const void *a, const void *b)
{
    const Activity *x = a, *y = b;
    return (x->start > y->start) - (x->start < y->start);
}
static int cmp_by_duration(const void *a, const void *b)
{
    const Activity *x = a, *y = b;
    int dx = x->finish - x->start, dy = y->finish - y->start;
    return (dx > dy) - (dx < dy);
}

static int select_activities(Activity *acts, int n,
                             int (*strategy)(const void *, const void *),
                             int *chosen, int *n_chosen)
{
    qsort(acts, (size_t)n, sizeof *acts, strategy);

    *n_chosen = 0;
    int last_finish = INT_MIN;
    for (int i = 0; i < n; i++)
        if (acts[i].start >= last_finish) {
            chosen[(*n_chosen)++] = i;
            last_finish = acts[i].finish;
        }
    return *n_chosen;
}

/* ================================================================= *
 * 2. FRACTIONAL KNAPSACK — greedy IS optimal, because items divide.
 *
 * Contrast with 0/1 knapsack (module 10, file 03), where items are
 * indivisible and greedy FAILS. That single difference — can you take half
 * an item? — flips the problem from NP-hard to O(n log n).
 * ================================================================= */
typedef struct { int weight, value; double ratio; char name[12]; } Item;

static int cmp_by_ratio_desc(const void *a, const void *b)
{
    const Item *x = a, *y = b;
    return (y->ratio > x->ratio) - (y->ratio < x->ratio);
}

static double fractional_knapsack(Item *items, int n, int capacity, bool verbose)
{
    /* Sort by VALUE PER UNIT WEIGHT — the greedy criterion. */
    for (int i = 0; i < n; i++)
        items[i].ratio = (double)items[i].value / (double)items[i].weight;
    qsort(items, (size_t)n, sizeof *items, cmp_by_ratio_desc);

    double total = 0.0;
    int remaining = capacity;

    for (int i = 0; i < n && remaining > 0; i++) {
        if (items[i].weight <= remaining) {
            total += items[i].value;                        /* take it whole */
            remaining -= items[i].weight;
            if (verbose) printf("      take ALL of %-8s (w %2d, v %2d, ratio %.2f)\n",
                                items[i].name, items[i].weight, items[i].value, items[i].ratio);
        } else {
            double fraction = (double)remaining / items[i].weight;
            total += items[i].value * fraction;             /* take a fraction */
            if (verbose) printf("      take %.0f%% of %-8s (w %2d, v %2d, ratio %.2f)\n",
                                fraction * 100, items[i].name, items[i].weight,
                                items[i].value, items[i].ratio);
            remaining = 0;
        }
    }
    return total;
}

/* The 0/1 version by DP, for comparison. */
static int knapsack_01(const Item *items, int n, int capacity)
{
    int *dp = calloc((size_t)capacity + 1, sizeof *dp);
    for (int i = 0; i < n; i++)
        for (int w = capacity; w >= items[i].weight; w--)
            dp[w] = max_i(dp[w], dp[w - items[i].weight] + items[i].value);
    int best = dp[capacity];
    free(dp);
    return best;
}

/* The 0/1 knapsack solved GREEDILY (by ratio) — to show it is WRONG. */
static int knapsack_01_greedy(Item *items, int n, int capacity)
{
    for (int i = 0; i < n; i++)
        items[i].ratio = (double)items[i].value / (double)items[i].weight;
    qsort(items, (size_t)n, sizeof *items, cmp_by_ratio_desc);

    int total = 0, remaining = capacity;
    for (int i = 0; i < n; i++)
        if (items[i].weight <= remaining) { total += items[i].value; remaining -= items[i].weight; }
    return total;
}

/* ================================================================= *
 * 3. HUFFMAN CODING — greedy IS optimal (Huffman proved it in 1952).
 *
 * Repeatedly merge the two LEAST frequent nodes. The result is a prefix-free
 * code with the minimum possible expected length.
 * ================================================================= */
typedef struct HNode {
    int freq;
    char symbol;                     /* only meaningful for leaves */
    struct HNode *left, *right;
} HNode;

/* A tiny min-heap over HNode pointers. */
typedef struct { HNode **a; int len, cap; } HHeap;

static void hheap_push(HHeap *h, HNode *node)
{
    int i = h->len++;
    h->a[i] = node;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->a[p]->freq <= h->a[i]->freq) break;
        HNode *t = h->a[p]; h->a[p] = h->a[i]; h->a[i] = t;
        i = p;
    }
}
static HNode *hheap_pop(HHeap *h)
{
    if (h->len == 0) return NULL;
    HNode *top = h->a[0];
    h->a[0] = h->a[--h->len];
    int i = 0;
    for (;;) {
        int l = 2*i+1, r = 2*i+2, small = i;
        if (l < h->len && h->a[l]->freq < h->a[small]->freq) small = l;
        if (r < h->len && h->a[r]->freq < h->a[small]->freq) small = r;
        if (small == i) break;
        HNode *t = h->a[i]; h->a[i] = h->a[small]; h->a[small] = t;
        i = small;
    }
    return top;
}

static HNode *huffman_build(const int *freq, const char *symbols, int n)
{
    HHeap h;
    h.a = malloc((size_t)(2 * n) * sizeof *h.a);
    h.len = 0; h.cap = 2 * n;

    for (int i = 0; i < n; i++) {
        if (freq[i] == 0) continue;
        HNode *leaf = calloc(1, sizeof *leaf);
        leaf->freq = freq[i];
        leaf->symbol = symbols[i];
        hheap_push(&h, leaf);
    }

    /* THE GREEDY STEP: merge the two least frequent, repeatedly. */
    while (h.len > 1) {
        HNode *a = hheap_pop(&h);
        HNode *b = hheap_pop(&h);
        HNode *parent = calloc(1, sizeof *parent);
        parent->freq = a->freq + b->freq;
        parent->left = a; parent->right = b;
        hheap_push(&h, parent);
    }

    HNode *root = hheap_pop(&h);
    free(h.a);
    return root;
}

static void huffman_codes(const HNode *n, char *code, int depth,
                          char table[128][32], int *total_bits, const int *freq_of)
{
    if (n == NULL) return;
    if (n->left == NULL && n->right == NULL) {           /* a leaf */
        code[depth] = '\0';
        if (depth == 0) { code[0] = '0'; code[1] = '\0'; depth = 1; }  /* single-symbol edge case */
        snprintf(table[(unsigned char)n->symbol], 32, "%s", code);
        *total_bits += depth * freq_of[(unsigned char)n->symbol];
        return;
    }
    code[depth] = '0'; huffman_codes(n->left,  code, depth + 1, table, total_bits, freq_of);
    code[depth] = '1'; huffman_codes(n->right, code, depth + 1, table, total_bits, freq_of);
}

static void huffman_free(HNode *n)
{ if (n) { huffman_free(n->left); huffman_free(n->right); free(n); } }

/* ================================================================= *
 * 4. COIN CHANGE — where greedy FAILS
 * ================================================================= */
static int coin_greedy(const int *coins, int n, int amount, int *used)
{
    /* Assumes coins are sorted descending. */
    int count = 0;
    for (int i = 0; i < n && amount > 0; i++) {
        used[i] = amount / coins[i];
        count += used[i];
        amount -= used[i] * coins[i];
    }
    return (amount == 0) ? count : -1;
}
static int coin_dp(const int *coins, int n, int amount)
{
    int *dp = malloc((size_t)(amount + 1) * sizeof *dp);
    dp[0] = 0;
    for (int a = 1; a <= amount; a++) {
        dp[a] = INT_MAX;
        for (int c = 0; c < n; c++)
            if (coins[c] <= a && dp[a - coins[c]] != INT_MAX && dp[a - coins[c]] + 1 < dp[a])
                dp[a] = dp[a - coins[c]] + 1;
    }
    int r = (dp[amount] == INT_MAX) ? -1 : dp[amount];
    free(dp);
    return r;
}

int main(void)
{
    puts("=== THE GREEDY-CHOICE PROPERTY ===");
    puts("  A greedy algorithm takes the best LOCAL option and never revisits");
    puts("  it. That works only when a locally optimal choice is always part of");
    puts("  SOME globally optimal solution.");
    puts("");
    puts("  That is a THEOREM about your problem, not a hope. It needs proof —");
    puts("  usually an EXCHANGE ARGUMENT: show that any optimal solution can be");
    puts("  transformed into one containing the greedy choice, without getting");
    puts("  worse.\n");

    puts("=== 1. ACTIVITY SELECTION — greedy is optimal ===");
    {
        Activity acts[] = {
            {1, 4, "lecture"},  {3, 5, "lab"},     {0, 6, "seminar"},
            {5, 7, "meeting"},  {3, 9, "workshop"},{5, 9, "review"},
            {6,10, "study"},    {8,11, "project"}, {8,12, "writing"},
            {2,14, "reading"},  {12,16,"exam"},
        };
        int n = (int)(sizeof acts / sizeof acts[0]);
        Activity work[16];
        int chosen[16], n_chosen;

        printf("  %d activities (start, finish):\n    ", n);
        for (int i = 0; i < n; i++) printf("%s(%d,%d) ", acts[i].name, acts[i].start, acts[i].finish);
        puts("\n");

        struct { const char *name; int (*cmp)(const void *, const void *); } strategies[] = {
            {"earliest FINISH  (correct)", cmp_by_finish},
            {"earliest START",             cmp_by_start},
            {"shortest DURATION",          cmp_by_duration},
        };

        for (size_t s = 0; s < 3; s++) {
            memcpy(work, acts, sizeof acts);
            select_activities(work, n, strategies[s].cmp, chosen, &n_chosen);
            printf("  %-28s -> %d activities: ", strategies[s].name, n_chosen);
            for (int i = 0; i < n_chosen; i++) printf("%s ", work[chosen[i]].name);
            puts("");
        }

        puts("\n  Only 'earliest finish' is provably optimal. THE PROOF (exchange");
        puts("  argument): let g be the activity that finishes earliest. Take any");
        puts("  optimal solution O. If g is not in O, swap O's first activity for");
        puts("  g — g finishes no later, so nothing else in O now conflicts, and");
        puts("  the count is unchanged. So SOME optimal solution contains g.");
        puts("  Recurse on the activities that start after g finishes.");
        puts("");
        puts("  'Shortest duration' is the intuitive choice and it is WRONG: one");
        puts("  short activity in the middle can block two longer ones on either");
        puts("  side. Intuition is not a proof.");
    }

    puts("\n=== 2. FRACTIONAL vs 0/1 KNAPSACK ===");
    {
        Item items[] = {
            {10, 60, 0, "gold"}, {20, 100, 0, "silver"}, {30, 120, 0, "bronze"},
        };
        int n = 3, capacity = 50;
        Item work[8];

        printf("  items: ");
        for (int i = 0; i < n; i++)
            printf("%s(w %d, v %d) ", items[i].name, items[i].weight, items[i].value);
        printf("  capacity %d\n\n", capacity);

        memcpy(work, items, sizeof items);
        puts("    FRACTIONAL (items divide) — greedy by value/weight:");
        double frac = fractional_knapsack(work, n, capacity, true);
        printf("      total value: %.1f  <- PROVABLY OPTIMAL\n", frac);

        memcpy(work, items, sizeof items);
        int greedy01 = knapsack_01_greedy(work, n, capacity);
        int dp01     = knapsack_01(items, n, capacity);
        printf("\n    0/1 (items are indivisible):\n");
        printf("      greedy by ratio : %d\n", greedy01);
        printf("      DP (correct)    : %d\n", dp01);
        printf("      %s\n", greedy01 == dp01
               ? "      (they agree HERE — but see the counterexample below)"
               : "      GREEDY IS WRONG");

        /* A case constructed so greedy definitely fails. */
        Item bad[] = { {3, 5, 0, "A"}, {4, 6, 0, "B"}, {7, 10, 0, "C"} };
        int cap2 = 10;
        memcpy(work, bad, sizeof bad);
        int g2 = knapsack_01_greedy(work, 3, cap2);
        int d2 = knapsack_01(bad, 3, cap2);
        printf("\n    counterexample: A(w3,v5) B(w4,v6) C(w7,v10), capacity %d\n", cap2);
        printf("      ratios: A = %.2f, B = %.2f, C = %.2f\n", 5/3.0, 6/4.0, 10/7.0);
        printf("      greedy by ratio : %d   (takes A, then B; C no longer fits)\n", g2);
        printf("      DP (correct)    : %d   (takes A + C, using the capacity exactly)\n", d2);
        printf("      %s\n", g2 < d2
               ? "      ^ GREEDY MISSED THE OPTIMUM by stranding 3 units of capacity"
               : "      (they agree — the counterexample failed to fire)");

        puts("\n  ONE difference — can you take HALF an item? — decides whether");
        puts("  greedy is optimal or the problem is NP-hard:");
        puts("    FRACTIONAL: greedy by ratio is optimal. O(n log n), just a sort.");
        puts("                Any leftover capacity is filled by a fraction of the");
        puts("                next-best item, so no capacity is ever wasted.");
        puts("    0/1:        greedy can strand capacity it cannot use. NP-hard;");
        puts("                needs DP (pseudo-polynomial) or branch and bound.");
    }

    puts("\n=== 3. HUFFMAN CODING — greedy is optimal ===");
    {
        const char *text = "this is an example of a huffman tree";
        int freq[128] = {0};
        for (const char *p = text; *p; p++) freq[(unsigned char)*p]++;

        char symbols[128];
        int  counts[128], n = 0;
        for (int c = 0; c < 128; c++)
            if (freq[c] > 0) { symbols[n] = (char)c; counts[n] = freq[c]; n++; }

        HNode *root = huffman_build(counts, symbols, n);

        char table[128][32] = {{0}};
        char code[64];
        int total_bits = 0;
        huffman_codes(root, code, 0, table, &total_bits, freq);

        printf("  text: \"%s\" (%zu characters, %d distinct)\n", text, strlen(text), n);
        puts("\n  symbol  freq  code");
        for (int i = 0; i < n && i < 12; i++) {
            unsigned char c = (unsigned char)symbols[i];
            printf("    '%c'   %4d  %s\n", c == ' ' ? '_' : c, freq[c], table[c]);
        }
        if (n > 12) printf("    ... and %d more\n", n - 12);

        size_t fixed_bits = strlen(text) * 8;
        printf("\n  fixed 8-bit encoding : %zu bits\n", fixed_bits);
        printf("  Huffman encoding     : %d bits  (%.0f%% of the original)\n",
               total_bits, 100.0 * total_bits / (double)fixed_bits);

        puts("\n  THE GREEDY STEP: repeatedly merge the two LEAST frequent nodes.");
        puts("  Frequent symbols end up near the root and get SHORT codes.");
        puts("");
        puts("  It produces a PREFIX-FREE code: no code is a prefix of another,");
        puts("  so the stream decodes unambiguously with no separators. That");
        puts("  falls out of every symbol being a LEAF.");
        puts("");
        puts("  Huffman PROVED this greedy choice optimal in 1952: no other");
        puts("  prefix-free code has a shorter expected length for these");
        puts("  frequencies. It is still inside gzip, JPEG, MP3 and PNG today.");

        huffman_free(root);
    }

    puts("\n=== 4. COIN CHANGE — where greedy FAILS ===");
    {
        int us[]  = {25, 10, 5, 1};        /* descending */
        int odd[] = {4, 3, 1};
        int used[8];

        puts("  US coins {25,10,5,1} — greedy IS optimal for this set:");
        for (int amt = 30; amt <= 99; amt += 23) {
            int g = coin_greedy(us, 4, amt, used);
            int d = coin_dp(us, 4, amt);
            printf("    %2d cents: greedy %d, DP %d  %s\n", amt, g, d,
                   g == d ? "agree" : "*** GREEDY WRONG ***");
        }

        puts("\n  coins {4,3,1} — greedy is NOT optimal:");
        for (int amt = 6; amt <= 9; amt++) {
            int g = coin_greedy(odd, 3, amt, used);
            int d = coin_dp(odd, 3, amt);
            printf("    %d units: greedy %d, DP %d  %s\n", amt, g, d,
                   g == d ? "agree" : "*** GREEDY WRONG ***");
        }
        puts("\n    For 6: greedy takes 4, then 1, then 1 = 3 coins.");
        puts("           The optimum is 3 + 3 = 2 coins.");
        puts("    Taking the largest coin STRANDED the remainder in a shape");
        puts("    that needed small change.");
        puts("");
        puts("  A coin system where greedy always works is called CANONICAL.");
        puts("  Most real currencies are canonical — deliberately, so that");
        puts("  cashiers can make change greedily without thinking. Checking");
        puts("  whether an arbitrary set is canonical is itself a hard problem.");
    }

    puts("\n=== GREEDY vs DYNAMIC PROGRAMMING ===");
    puts("                    GREEDY              DP");
    puts("    choices         one, irrevocable    considers all, keeps the best");
    puts("    time            usually O(n log n)  usually O(n * something)");
    puts("    space           O(1)                O(n) or O(n*m)");
    puts("    correctness     NEEDS A PROOF       correct by construction");
    puts("    when it works   greedy-choice       optimal substructure +");
    puts("                    property holds      overlapping subproblems");
    puts("");
    puts("  PROVABLY-OPTIMAL GREEDY ALGORITHMS WORTH KNOWING:");
    puts("    activity selection      earliest finish time");
    puts("    fractional knapsack     highest value/weight ratio");
    puts("    Huffman coding          merge the two least frequent");
    puts("    Dijkstra                nearest unvisited vertex (non-negative)");
    puts("    Kruskal / Prim          cheapest safe edge");
    puts("    job scheduling          earliest deadline first");
    puts("");
    puts("  HOW TO CHECK A GREEDY IDEA:");
    puts("    1. State the greedy choice precisely.");
    puts("    2. Try to prove it by EXCHANGE: take an optimal solution that");
    puts("       does NOT make your choice, and show you can swap your choice");
    puts("       in without making it worse.");
    puts("    3. If you cannot prove it, HUNT FOR A COUNTEREXAMPLE — small,");
    puts("       adversarial, and hand-constructed.");
    puts("    4. If neither works, use DP. It is slower and it is correct.");
    puts("");
    puts("  Testing a greedy algorithm on examples proves nothing. The coin");
    puts("  system {25,10,5,1} passes every test you would naturally write,");
    puts("  and {4,3,1} fails on the very first small case.");

    return 0;
}
