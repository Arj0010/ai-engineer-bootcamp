/* 01_sorting.c — eight sorting algorithms, instrumented and benchmarked.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 01_sorting.c -o t && ./t
 *
 * Every sort counts its comparisons and swaps, so the asymptotics are visible
 * rather than asserted. The benchmarks at the end show where theory and the
 * stopwatch disagree — which is the real lesson.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>
#include <stddef.h>   /* ptrdiff_t */

static size_t comparisons, swaps;

static void swap_i(int *a, int *b) { int t = *a; *a = *b; *b = t; swaps++; }
static bool less(int a, int b) { comparisons++; return a < b; }

/* ================================================================= *
 * O(n^2) SORTS
 * ================================================================= */

/* BUBBLE: repeatedly swap adjacent out-of-order pairs. The largest element
 * "bubbles" to the end each pass. Included for completeness — it is strictly
 * worse than insertion sort in every respect. The early-exit flag makes it
 * O(n) on already-sorted input, which is its one redeeming feature. */
static void bubble_sort(int *a, size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        bool swapped = false;
        for (size_t j = 0; j + 1 < n - i; j++)
            if (less(a[j + 1], a[j])) { swap_i(&a[j], &a[j + 1]); swapped = true; }
        if (!swapped) return;                    /* already sorted */
    }
}

/* SELECTION: find the minimum of the unsorted part, swap it into place.
 * Always O(n^2) comparisons, but only n-1 SWAPS — the minimum possible.
 * That makes it the right choice when a write is far more expensive than a
 * read (EEPROM, flash memory with limited write cycles). */
static void selection_sort(int *a, size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++) if (less(a[j], a[min])) min = j;
        if (min != i) swap_i(&a[i], &a[min]);
    }
}

/* INSERTION: take the next element and slide it left into position, like
 * sorting a hand of cards.
 *
 * THE ONE TO REMEMBER. It is O(n^2) in theory but:
 *   - O(n) on already-sorted or nearly-sorted input
 *   - tiny constants: a tight loop over adjacent memory, no recursion
 *   - stable, in place, and ONLINE (it can sort a stream as it arrives)
 * Every real library sort falls back to it below ~16-32 elements. */
static void insertion_sort(int *a, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        int key = a[i];
        size_t j = i;
        while (j > 0 && less(key, a[j - 1])) { a[j] = a[j - 1]; j--; swaps++; }
        a[j] = key;
    }
}

/* SHELL SORT: insertion sort on interleaved subsequences with a shrinking
 * gap. Large gaps move elements a long way cheaply; the final gap-1 pass is
 * a plain insertion sort on nearly-sorted data. ~O(n^1.3) with these gaps,
 * in place, and remarkably good for how simple it is. */
static void shell_sort(int *a, size_t n)
{
    /* Ciura's gap sequence — empirically the best known for small n. */
    static const size_t gaps[] = {701, 301, 132, 57, 23, 10, 4, 1};
    for (size_t g = 0; g < sizeof gaps / sizeof gaps[0]; g++) {
        size_t gap = gaps[g];
        if (gap >= n) continue;
        for (size_t i = gap; i < n; i++) {
            int key = a[i];
            size_t j = i;
            while (j >= gap && less(key, a[j - gap])) { a[j] = a[j - gap]; j -= gap; swaps++; }
            a[j] = key;
        }
    }
}

/* ================================================================= *
 * O(n log n) SORTS
 * ================================================================= */

/* MERGE SORT: split in half, sort each half, merge.
 * O(n log n) ALWAYS — best, average and worst are the same, which makes it
 * predictable. Stable. Needs O(n) scratch space, which is its one real cost.
 * It is the algorithm for linked lists and for external (on-disk) sorting,
 * because it only ever needs SEQUENTIAL access. */
static void merge_halves(int *a, int *scratch, size_t lo, size_t mid, size_t hi)
{
    size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        /* `<=` not `<` — taking from the LEFT half on a tie is what makes
         * this STABLE. Flipping it silently breaks stability. */
        if (!less(a[j], a[i])) scratch[k++] = a[i++];
        else                   scratch[k++] = a[j++];
    }
    while (i < mid) scratch[k++] = a[i++];
    while (j < hi)  scratch[k++] = a[j++];
    memcpy(a + lo, scratch + lo, (hi - lo) * sizeof *a);
}
static void merge_rec(int *a, int *scratch, size_t lo, size_t hi)
{
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    merge_rec(a, scratch, lo, mid);
    merge_rec(a, scratch, mid, hi);
    if (less(a[mid], a[mid - 1]))                 /* already in order? skip */
        merge_halves(a, scratch, lo, mid, hi);
}
static void merge_sort(int *a, size_t n)
{
    if (n < 2) return;
    int *scratch = malloc(n * sizeof *scratch);
    if (scratch == NULL) return;
    merge_rec(a, scratch, 0, n);
    free(scratch);
}

/* QUICKSORT: pick a pivot, partition so that everything smaller is left and
 * everything larger is right, recurse on both sides.
 *
 * Fastest in practice — the partition step is a single sequential pass with
 * excellent cache behaviour and no extra memory. Its weakness is the pivot:
 * a bad choice gives O(n^2). MEDIAN-OF-THREE plus an insertion-sort cutoff
 * is what makes it robust. */
/* NOTE THE SIGNED INDICES. Writing this with size_t is a classic trap: when
 * the pivot lands at index 0, `hi = p - 1` wraps to SIZE_MAX and the next
 * partition indexes wildly out of bounds. It is a real heap-buffer-overflow
 * that ASan catches instantly and that -O2 alone will not. Inclusive bounds
 * that can go "one before the start" need a SIGNED type. */
static ptrdiff_t partition_lomuto(int *a, ptrdiff_t lo, ptrdiff_t hi)
{
    /* MEDIAN-OF-THREE: sort a[lo], a[mid], a[hi] and use the median as the
     * pivot. This makes already-sorted input the BEST case instead of the
     * worst, which is the single most important quicksort hardening. */
    ptrdiff_t mid = lo + (hi - lo) / 2;
    if (less(a[mid], a[lo]))  swap_i(&a[mid], &a[lo]);
    if (less(a[hi],  a[lo]))  swap_i(&a[hi],  &a[lo]);
    if (less(a[hi],  a[mid])) swap_i(&a[hi],  &a[mid]);
    swap_i(&a[mid], &a[hi]);                     /* park the pivot at the end */

    int pivot = a[hi];
    ptrdiff_t i = lo;
    for (ptrdiff_t j = lo; j < hi; j++)
        if (less(a[j], pivot)) swap_i(&a[i++], &a[j]);
    swap_i(&a[i], &a[hi]);                       /* put the pivot in its place */
    return i;
}
static void quick_rec(int *a, ptrdiff_t lo, ptrdiff_t hi)
{
    while (lo < hi) {
        /* CUTOFF: below ~16 elements, insertion sort is genuinely faster. */
        if (hi - lo < 16) { insertion_sort(a + lo, (size_t)(hi - lo + 1)); return; }

        ptrdiff_t p = partition_lomuto(a, lo, hi);

        /* Recurse into the SMALLER side and loop on the larger one. This
         * bounds the STACK DEPTH at O(log n) even in the worst case — without
         * it, a pathological pivot sequence overflows the stack. */
        if (p - lo < hi - p) { quick_rec(a, lo, p - 1); lo = p + 1; }
        else                 { quick_rec(a, p + 1, hi); hi = p - 1; }
    }
}
static void quick_sort(int *a, size_t n) { if (n > 1) quick_rec(a, 0, (ptrdiff_t)n - 1); }

/* HEAPSORT: build a max-heap, then repeatedly move the root to the back.
 * O(n log n) GUARANTEED and O(1) space — no adversarial input exists. It is
 * 2-3x slower than quicksort in practice because sift-down jumps around the
 * array, which the cache hates. Real libraries use it as quicksort's
 * fallback (INTROSORT) when the recursion gets too deep. */
static void sift_down_max(int *a, size_t n, size_t i)
{
    for (;;) {
        size_t l = 2*i + 1, r = 2*i + 2, largest = i;
        if (l < n && less(a[largest], a[l])) largest = l;
        if (r < n && less(a[largest], a[r])) largest = r;
        if (largest == i) return;
        swap_i(&a[i], &a[largest]);
        i = largest;
    }
}
static void heap_sort(int *a, size_t n)
{
    for (size_t i = n / 2; i-- > 0; ) sift_down_max(a, n, i);
    for (size_t end = n; end-- > 1; ) { swap_i(&a[0], &a[end]); sift_down_max(a, end, 0); }
}

/* ================================================================= *
 * NON-COMPARISON SORTS — these BEAT the Omega(n log n) lower bound by
 * never comparing two elements. They use the KEY ITSELF as an index.
 * ================================================================= */

/* COUNTING SORT: tally how many of each value, then write them back out.
 * O(n + k) where k is the value range. Unbeatable when k is small; useless
 * when k is huge (sorting 10 int32 values would need a 4-billion tally). */
static void counting_sort(int *a, size_t n, int min, int max)
{
    size_t range = (size_t)(max - min + 1);
    size_t *count = calloc(range, sizeof *count);
    if (count == NULL) return;

    for (size_t i = 0; i < n; i++) count[a[i] - min]++;

    size_t out = 0;
    for (size_t v = 0; v < range; v++)
        while (count[v]-- > 0) a[out++] = (int)v + min;

    free(count);
}

/* RADIX SORT (LSD): counting-sort by each digit, least significant first.
 * Stability is ESSENTIAL — it is what preserves the ordering established by
 * the previous, less significant digit. O(d * (n + k)) for d digits. */
static void radix_sort(int *a, size_t n)
{
    if (n < 2) return;
    int max = a[0];
    for (size_t i = 1; i < n; i++) if (a[i] > max) max = a[i];

    int *buf = malloc(n * sizeof *buf);
    if (buf == NULL) return;

    for (long exp = 1; max / exp > 0; exp *= 10) {
        size_t count[10] = {0};
        for (size_t i = 0; i < n; i++) count[(a[i] / exp) % 10]++;
        for (int d = 1; d < 10; d++) count[d] += count[d - 1];   /* prefix sums */

        /* Iterate BACKWARDS to keep it stable. Forwards would reverse equal
         * keys and destroy the previous digit's ordering. */
        for (size_t i = n; i-- > 0; ) buf[--count[(a[i] / exp) % 10]] = a[i];
        memcpy(a, buf, n * sizeof *a);
    }
    free(buf);
}

/* ================================================================= *
 * HARNESS
 * ================================================================= */
static bool is_sorted(const int *a, size_t n)
{
    for (size_t i = 1; i < n; i++) if (a[i] < a[i - 1]) return false;
    return true;
}
static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

static unsigned rng_state = 12345;
static int next_random(void) { rng_state = rng_state * 1103515245u + 12345u; return (int)((rng_state >> 16) & 0x7FFF); }

typedef enum { DATA_RANDOM, DATA_SORTED, DATA_REVERSED, DATA_FEW_UNIQUE } DataKind;
static void fill(int *a, size_t n, DataKind kind)
{
    rng_state = 12345;
    for (size_t i = 0; i < n; i++) {
        switch (kind) {
        case DATA_RANDOM:     a[i] = next_random();      break;
        case DATA_SORTED:     a[i] = (int)i;             break;
        case DATA_REVERSED:   a[i] = (int)(n - i);       break;
        case DATA_FEW_UNIQUE: a[i] = next_random() % 10; break;
        }
    }
}

typedef void (*SortFn)(int *, size_t);
static void run(const char *name, SortFn fn, const int *src, size_t n, int *work)
{
    memcpy(work, src, n * sizeof *work);
    comparisons = swaps = 0;
    clock_t t = clock();
    fn(work, n);
    double elapsed = seconds_since(t);
    printf("    %-16s %8.4f s  %12zu comparisons  %12zu moves  %s\n",
           name, elapsed, comparisons, swaps, is_sorted(work, n) ? "OK" : "*** WRONG ***");
}

/* Stability demonstration: sort (key, tag) pairs by key only. */
typedef struct { int key; char tag; } Pair;
static void stable_insertion(Pair *a, size_t n)
{
    for (size_t i = 1; i < n; i++) {
        Pair key = a[i];
        size_t j = i;
        while (j > 0 && a[j - 1].key > key.key) { a[j] = a[j - 1]; j--; }
        a[j] = key;
    }
}
static void unstable_selection(Pair *a, size_t n)
{
    for (size_t i = 0; i + 1 < n; i++) {
        size_t min = i;
        for (size_t j = i + 1; j < n; j++) if (a[j].key < a[min].key) min = j;
        Pair t = a[i]; a[i] = a[min]; a[min] = t;
    }
}

int main(void)
{
    puts("=== CORRECTNESS AND OPERATION COUNTS (n = 2000, random) ===");
    {
        const size_t n = 2000;
        int *src  = malloc(n * sizeof *src);
        int *work = malloc(n * sizeof *work);
        fill(src, n, DATA_RANDOM);

        run("bubble",    bubble_sort,    src, n, work);
        run("selection", selection_sort, src, n, work);
        run("insertion", insertion_sort, src, n, work);
        run("shell",     shell_sort,     src, n, work);
        run("merge",     merge_sort,     src, n, work);
        run("quick",     quick_sort,     src, n, work);
        run("heap",      heap_sort,      src, n, work);
        run("radix",     radix_sort,     src, n, work);

        printf("\n  For n=%zu:  n^2 = %zu,  n*log2(n) = %.0f\n",
               n, n * n, (double)n * 11.0);
        puts("  Compare those to the comparison counts above. The O(n^2) sorts");
        puts("  land near n^2/2 or n^2/4; the O(n log n) sorts land near n*log2(n).");
        puts("  radix reports ZERO comparisons — it never compares two elements.");

        free(src); free(work);
    }

    puts("\n=== INPUT SHAPE CHANGES EVERYTHING ===");
    {
        const size_t n = 20000;
        int *src  = malloc(n * sizeof *src);
        int *work = malloc(n * sizeof *work);

        const char *names[] = {"RANDOM", "ALREADY SORTED", "REVERSED", "FEW UNIQUE VALUES"};
        DataKind kinds[] = {DATA_RANDOM, DATA_SORTED, DATA_REVERSED, DATA_FEW_UNIQUE};

        for (int k = 0; k < 4; k++) {
            printf("\n  %s (n = %zu):\n", names[k], n);
            fill(src, n, kinds[k]);
            run("insertion", insertion_sort, src, n, work);
            run("shell",     shell_sort,     src, n, work);
            run("merge",     merge_sort,     src, n, work);
            run("quick",     quick_sort,     src, n, work);
            run("heap",      heap_sort,      src, n, work);
        }

        puts("\n  WHAT TO NOTICE:");
        puts("    - insertion sort is O(n) on SORTED input and the fastest thing");
        puts("      here; on REVERSED input it is the worst. Its cost is the");
        puts("      number of INVERSIONS, not n^2 in itself.");
        puts("    - merge sort barely changes: it is O(n log n) on everything.");
        puts("      That predictability is its selling point.");
        puts("    - quicksort handles sorted input fine ONLY because of");
        puts("      median-of-three. With a naive last-element pivot, sorted");
        puts("      input is its O(n^2) worst case — a real denial-of-service");
        puts("      vector in software that sorts user-supplied data.");

        free(src); free(work);
    }

    puts("\n=== WHERE INSERTION SORT BEATS THE 'BETTER' ALGORITHMS ===");
    {
        int src[512], work[512];
        puts("    n   insertion    merge      quick     winner");
        for (size_t n = 4; n <= 256; n *= 2) {
            fill(src, n, DATA_RANDOM);

            /* Repeat many times: a single run at this size is unmeasurable. */
            const int REPS = 20000;
            clock_t t;
            double t_ins, t_mrg, t_qck;

            t = clock();
            for (int r = 0; r < REPS; r++) { memcpy(work, src, n * sizeof *work); insertion_sort(work, n); }
            t_ins = seconds_since(t);

            t = clock();
            for (int r = 0; r < REPS; r++) { memcpy(work, src, n * sizeof *work); merge_sort(work, n); }
            t_mrg = seconds_since(t);

            t = clock();
            for (int r = 0; r < REPS; r++) { memcpy(work, src, n * sizeof *work); quick_sort(work, n); }
            t_qck = seconds_since(t);

            const char *winner = (t_ins <= t_mrg && t_ins <= t_qck) ? "insertion"
                               : (t_mrg <= t_qck) ? "merge" : "quick";
            printf("  %4zu   %8.4f  %8.4f  %8.4f   %s\n", n, t_ins, t_mrg, t_qck, winner);
        }
        puts("");
        puts("  (Note our quicksort already DELEGATES to insertion sort below 16");
        puts("   elements, so at n=4 and n=8 the two columns are measuring the");
        puts("   same code plus a little call overhead. That is the point being");
        puts("   made, arriving a row early.)");
        puts("");
        puts("  Big-O ignores CONSTANTS, and for small n the constants dominate.");
        puts("  Insertion sort is a single tight loop over adjacent memory with");
        puts("  no recursion, no allocation and perfect branch prediction.");
        puts("  Merge sort allocates and copies; quicksort recurses.");
        puts("");
        puts("  This is why EVERY production sort is a hybrid:");
        puts("    introsort (C++ std::sort): quicksort, switching to heapsort");
        puts("      when the recursion is too deep, and to insertion sort under 16");
        puts("    timsort (Python, Java):    merge sort that finds already-sorted");
        puts("      RUNS in the input and merges them, with insertion sort for");
        puts("      short runs. O(n) on nearly-sorted data.");
        puts("    pdqsort (Rust):            pattern-defeating quicksort");
    }

    puts("\n=== STABILITY ===");
    {
        Pair a[] = {{3,'a'},{1,'b'},{3,'c'},{2,'d'},{1,'e'},{3,'f'}};
        Pair b[6];
        memcpy(b, a, sizeof a);

        printf("  input           : ");
        for (int i = 0; i < 6; i++) printf("%d%c ", a[i].key, a[i].tag);

        stable_insertion(a, 6);
        printf("\n  insertion (STABLE): ");
        for (int i = 0; i < 6; i++) printf("%d%c ", a[i].key, a[i].tag);

        unstable_selection(b, 6);
        printf("\n  selection (unstable): ");
        for (int i = 0; i < 6; i++) printf("%d%c ", b[i].key, b[i].tag);
        puts("");
        puts("  A STABLE sort keeps equal keys in their original relative order:");
        puts("  1b before 1e, and 3a before 3c before 3f.");
        puts("  Why it matters: sort by surname, then by department, and a stable");
        puts("  sort leaves each department alphabetised. An unstable one does not.");
        puts("  Stable: insertion, merge, counting, radix, timsort.");
        puts("  Unstable: selection, quicksort, heapsort.");
        puts("  Any sort can be MADE stable by adding the original index as a");
        puts("  tiebreaker — at the cost of O(n) memory.");
    }

    puts("\n=== NON-COMPARISON SORTS BEAT THE LOWER BOUND ===");
    {
        const size_t n = 1000000;
        int *src  = malloc(n * sizeof *src);
        int *work = malloc(n * sizeof *work);

        /* Small value range, so counting sort is applicable. */
        rng_state = 999;
        for (size_t i = 0; i < n; i++) src[i] = next_random() % 1000;

        memcpy(work, src, n * sizeof *work);
        clock_t t = clock();
        quick_sort(work, n);
        double t_quick = seconds_since(t);

        memcpy(work, src, n * sizeof *work);
        t = clock();
        counting_sort(work, n, 0, 999);
        double t_count = seconds_since(t);
        bool ok_count = is_sorted(work, n);

        memcpy(work, src, n * sizeof *work);
        t = clock();
        radix_sort(work, n);
        double t_radix = seconds_since(t);
        bool ok_radix = is_sorted(work, n);

        printf("  %zu integers in the range 0..999:\n", n);
        printf("    quicksort : %.4f s\n", t_quick);
        printf("    counting  : %.4f s  (%.1fx faster) %s\n",
               t_count, t_quick / t_count, ok_count ? "" : "WRONG");
        printf("    radix     : %.4f s  (%.1fx faster) %s\n",
               t_radix, t_quick / t_radix, ok_radix ? "" : "WRONG");
        puts("");
        puts("  Omega(n log n) is a PROVEN lower bound — for COMPARISON sorts.");
        puts("  Counting and radix sort sidestep it entirely by using the key's");
        puts("  VALUE as an array index instead of comparing keys to each other.");
        puts("");
        puts("  The catch: counting sort needs O(k) memory for a value range k.");
        puts("  Here k=1000, which is trivial. For arbitrary 32-bit ints, k is");
        puts("  4 billion and it is unusable. Radix sort fixes that by working");
        puts("  one digit at a time, so k is 10 (or 256 for byte-wise radix).");
        puts("  Both need FIXED-WIDTH keys, so they do not work for strings of");
        puts("  arbitrary length or for user-supplied comparators.");

        free(src); free(work);
    }

    puts("\n=== CHOOSING A SORT ===");
    puts("  n < 20 or nearly sorted    insertion sort");
    puts("  general purpose            quicksort with median-of-three + cutoff");
    puts("  worst case matters         heapsort or merge sort (real-time, adversarial input)");
    puts("  stability required         merge sort (or timsort)");
    puts("  linked list                merge sort — the only good option");
    puts("  small integer range        counting sort");
    puts("  fixed-width integer keys   radix sort");
    puts("  data larger than RAM       external merge sort");
    puts("  nearly-sorted, large       timsort");
    puts("");
    puts("  IN PRACTICE: call qsort(). It is a tuned hybrid, and hand-rolling");
    puts("  a sort is almost never the right use of your time. Implement them");
    puts("  ONCE, to understand them; then use the library.");

    return 0;
}
