/* 07_string_algorithms.c — substring search, from naive to linear.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 07_string_algorithms.c -o t && ./t
 *
 * The problem: find a pattern of length m inside a text of length n.
 *
 * The naive method is O(nm) because on a mismatch it throws away everything
 * it learned and restarts one character along. Every algorithm here is a
 * different answer to the same question: WHAT DID THE MISMATCH TELL US, and
 * how far can we safely skip?
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stddef.h>

static size_t char_comparisons;

/* ================================================================= *
 * 1. NAIVE — try every alignment.
 * ================================================================= */
static ptrdiff_t naive_search(const char *text, const char *pat)
{
    size_t n = strlen(text), m = strlen(pat);
    char_comparisons = 0;
    if (m == 0 || m > n) return -1;

    for (size_t i = 0; i + m <= n; i++) {
        size_t j = 0;
        while (j < m) { char_comparisons++; if (text[i + j] != pat[j]) break; j++; }
        if (j == m) return (ptrdiff_t)i;
        /* On a mismatch we discard everything learned and restart at i+1.
         * That waste is what the next three algorithms eliminate. */
    }
    return -1;
}

/* ================================================================= *
 * 2. KMP (Knuth-Morris-Pratt) — O(n + m), never backs up in the text.
 *
 * THE INSIGHT: after matching "abcab" and failing on the next character,
 * we already know the last two characters of the text were "ab" — which is
 * also the pattern's PREFIX. So we can slide the pattern forward by 3 and
 * resume from position 2, without re-reading a single text character.
 *
 * The FAILURE FUNCTION lps[i] = the length of the longest proper prefix of
 * pat[0..i] that is also a suffix of it. It is computed from the pattern
 * alone, before the search starts.
 * ================================================================= */
static void kmp_build_lps(const char *pat, size_t m, size_t *lps)
{
    lps[0] = 0;
    size_t len = 0, i = 1;

    while (i < m) {
        if (pat[i] == pat[len]) {
            lps[i++] = ++len;
        } else if (len != 0) {
            len = lps[len - 1];      /* fall back — do NOT reset to 0 */
        } else {
            lps[i++] = 0;
        }
    }
}

static ptrdiff_t kmp_search(const char *text, const char *pat)
{
    size_t n = strlen(text), m = strlen(pat);
    char_comparisons = 0;
    if (m == 0 || m > n) return -1;

    size_t *lps = malloc(m * sizeof *lps);
    kmp_build_lps(pat, m, lps);

    size_t i = 0, j = 0;              /* i indexes the text, j the pattern */
    while (i < n) {
        char_comparisons++;
        if (text[i] == pat[j]) {
            i++; j++;
            if (j == m) { free(lps); return (ptrdiff_t)(i - m); }
        } else if (j > 0) {
            j = lps[j - 1];           /* slide the PATTERN; i does NOT move back */
        } else {
            i++;
        }
    }
    free(lps);
    return -1;
}

/* ================================================================= *
 * 3. RABIN-KARP — compare HASHES, not characters.
 *
 * A ROLLING HASH updates in O(1) as the window slides: subtract the
 * departing character's contribution, shift, add the arriving one. Only when
 * the hashes match do we verify the characters (a "spurious hit" is possible
 * but rare with a good modulus).
 *
 * Its real strength is MULTI-PATTERN search: hash k patterns once, then one
 * pass over the text finds any of them.
 * ================================================================= */
#define RK_BASE 256u
#define RK_MOD  1000000007u

static ptrdiff_t rabin_karp_search(const char *text, const char *pat, size_t *spurious)
{
    size_t n = strlen(text), m = strlen(pat);
    char_comparisons = 0;
    if (spurious) *spurious = 0;
    if (m == 0 || m > n) return -1;

    /* high = BASE^(m-1) mod MOD — the weight of the leaving character. */
    uint64_t high = 1;
    for (size_t i = 1; i < m; i++) high = (high * RK_BASE) % RK_MOD;

    uint64_t pat_hash = 0, win_hash = 0;
    for (size_t i = 0; i < m; i++) {
        pat_hash = (pat_hash * RK_BASE + (unsigned char)pat[i])  % RK_MOD;
        win_hash = (win_hash * RK_BASE + (unsigned char)text[i]) % RK_MOD;
    }

    for (size_t i = 0; ; i++) {
        if (pat_hash == win_hash) {
            /* Hashes agree — VERIFY, because different strings can collide. */
            size_t j = 0;
            while (j < m) { char_comparisons++; if (text[i + j] != pat[j]) break; j++; }
            if (j == m) return (ptrdiff_t)i;
            if (spurious) (*spurious)++;
        }
        if (i + m >= n) break;

        /* ROLL the window in O(1): drop text[i], add text[i+m]. */
        win_hash = (win_hash + RK_MOD - ((uint64_t)(unsigned char)text[i] * high) % RK_MOD) % RK_MOD;
        win_hash = (win_hash * RK_BASE + (unsigned char)text[i + m]) % RK_MOD;
    }
    return -1;
}

/* ================================================================= *
 * 4. BOYER-MOORE-HORSPOOL — scan the pattern BACKWARDS and skip far.
 *
 * THE INSIGHT: compare from the pattern's END. If the text character that
 * aligned with the pattern's last position does not occur in the pattern at
 * all, NOTHING can match here — skip the whole pattern length.
 *
 * This is SUBLINEAR in the best case: O(n/m). It does not read most of the
 * text at all. Every real `grep` uses a variant of this.
 * ================================================================= */
static ptrdiff_t horspool_search(const char *text, const char *pat, size_t *skipped)
{
    size_t n = strlen(text), m = strlen(pat);
    char_comparisons = 0;
    if (skipped) *skipped = 0;
    if (m == 0 || m > n) return -1;

    /* BAD CHARACTER TABLE: for each byte, how far can we shift?
     * Default is the whole pattern length. */
    size_t shift[256];
    for (int c = 0; c < 256; c++) shift[c] = m;
    for (size_t i = 0; i + 1 < m; i++) shift[(unsigned char)pat[i]] = m - 1 - i;

    size_t i = 0;
    while (i + m <= n) {
        size_t j = m;
        while (j > 0) { char_comparisons++; if (text[i + j - 1] != pat[j - 1]) break; j--; }
        if (j == 0) return (ptrdiff_t)i;

        size_t step = shift[(unsigned char)text[i + m - 1]];
        if (skipped) *skipped += step;
        i += step;                                /* skip, possibly by m */
    }
    return -1;
}

/* ================================================================= *
 * 5. Z-ALGORITHM — z[i] = the length of the longest substring starting at
 * i that is also a PREFIX of the whole string.
 *
 * Build z over "pattern + separator + text"; any z value equal to the
 * pattern length marks a match. O(n + m), and the z-array itself is useful
 * for many other string problems.
 * ================================================================= */
static void z_build(const char *s, size_t n, size_t *z)
{
    z[0] = n;
    size_t l = 0, r = 0;                          /* the current "z-box" */

    for (size_t i = 1; i < n; i++) {
        if (i < r) {
            /* Reuse a previously computed value — this is what makes it linear. */
            size_t k = i - l;
            z[i] = (z[k] < r - i) ? z[k] : r - i;
        } else {
            z[i] = 0;
        }
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) z[i]++;   /* extend */
        if (i + z[i] > r) { l = i; r = i + z[i]; }               /* new box */
    }
}

static ptrdiff_t z_search(const char *text, const char *pat)
{
    size_t n = strlen(text), m = strlen(pat);
    char_comparisons = 0;
    if (m == 0 || m > n) return -1;

    size_t total = m + 1 + n;
    char *combined = malloc(total + 1);
    size_t *z = malloc(total * sizeof *z);

    /* '\x01' is a separator guaranteed not to occur in ordinary text. */
    memcpy(combined, pat, m);
    combined[m] = '\x01';
    memcpy(combined + m + 1, text, n);
    combined[total] = '\0';

    z_build(combined, total, z);

    ptrdiff_t result = -1;
    for (size_t i = m + 1; i < total; i++)
        if (z[i] == m) { result = (ptrdiff_t)(i - m - 1); break; }

    free(combined); free(z);
    return result;
}

/* Find ALL occurrences with KMP — the common real requirement. */
static size_t kmp_find_all(const char *text, const char *pat, size_t *out, size_t max_out)
{
    size_t n = strlen(text), m = strlen(pat);
    if (m == 0 || m > n) return 0;

    size_t *lps = malloc(m * sizeof *lps);
    kmp_build_lps(pat, m, lps);

    size_t count = 0, i = 0, j = 0;
    while (i < n) {
        if (text[i] == pat[j]) {
            i++; j++;
            if (j == m) {
                if (count < max_out) out[count] = i - m;
                count++;
                j = lps[j - 1];        /* continue — this is what finds OVERLAPS */
            }
        } else if (j > 0) {
            j = lps[j - 1];
        } else {
            i++;
        }
    }
    free(lps);
    return count;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE PROBLEM ===");
    puts("  Find a pattern of length m in a text of length n.");
    puts("  The naive method tries every alignment and, on a mismatch, throws");
    puts("  away everything it learned and restarts one character along.");
    puts("  Every algorithm below answers the same question differently:");
    puts("  WHAT DID THE MISMATCH TELL US, and how far can we safely skip?\n");

    puts("=== ALL FIVE AGREE ===");
    {
        const char *text = "the quick brown fox jumps over the lazy dog";
        const char *pats[] = {"brown", "the", "dog", "cat", "quick brown"};

        printf("  text: \"%s\"\n\n", text);
        printf("  %-14s %8s %8s %8s %8s %8s\n",
               "pattern", "naive", "kmp", "rabin-k", "horspool", "z-algo");
        for (size_t p = 0; p < 5; p++) {
            ptrdiff_t r1 = naive_search(text, pats[p]);
            ptrdiff_t r2 = kmp_search(text, pats[p]);
            ptrdiff_t r3 = rabin_karp_search(text, pats[p], NULL);
            ptrdiff_t r4 = horspool_search(text, pats[p], NULL);
            ptrdiff_t r5 = z_search(text, pats[p]);
            printf("  %-14s %8td %8td %8td %8td %8td%s\n",
                   pats[p], r1, r2, r3, r4, r5,
                   (r1 == r2 && r2 == r3 && r3 == r4 && r4 == r5) ? "" : "  *** DISAGREE ***");
        }
        puts("\n  (-1 means not found. All five must always agree — they differ");
        puts("   only in HOW MUCH WORK they do to get there.)");
    }

    puts("\n=== THE KMP FAILURE FUNCTION ===");
    {
        const char *pats[] = {"ababaca", "aaaa", "abcdef", "aabaaac"};
        for (size_t p = 0; p < 4; p++) {
            size_t m = strlen(pats[p]);
            size_t *lps = malloc(m * sizeof *lps);
            kmp_build_lps(pats[p], m, lps);

            printf("  pattern: %-10s lps: [", pats[p]);
            for (size_t i = 0; i < m; i++) printf("%zu%s", lps[i], i + 1 < m ? " " : "");
            puts("]");
            free(lps);
        }
        puts("");
        puts("  lps[i] = the length of the longest PROPER PREFIX of pat[0..i]");
        puts("  that is also a SUFFIX of pat[0..i].");
        puts("");
        puts("  For \"ababaca\": at index 4 (\"ababa\") the answer is 3, because");
        puts("  \"aba\" is both a prefix and a suffix. So if we have matched five");
        puts("  characters and the sixth fails, we already know the last three");
        puts("  text characters were \"aba\" — the pattern's own prefix. Slide the");
        puts("  pattern forward by 2 and resume at position 3.");
        puts("");
        puts("  THE TEXT INDEX NEVER MOVES BACKWARD. That is the whole guarantee,");
        puts("  and it is why KMP is O(n + m) and can run on a STREAM you cannot");
        puts("  rewind — a socket, a pipe, a tape.");
    }

    puts("\n=== ADVERSARIAL INPUT: WHERE NAIVE COLLAPSES ===");
    {
        /* The worst case for naive search: long runs of one character, with
         * the mismatch only at the very last position of each alignment. */
        const size_t N = 100000, M = 500;
        char *text = malloc(N + 1);
        char *pat  = malloc(M + 1);
        memset(text, 'a', N); text[N] = '\0';
        memset(pat,  'a', M); pat[M - 1] = 'b'; pat[M] = '\0';   /* "aaa...ab" */

        printf("  text = \"aaaa...a\" (%zu chars), pattern = \"aaa...ab\" (%zu chars)\n",
               N, M);
        puts("  Every alignment matches m-1 characters and then fails. This is");
        puts("  the exact shape that makes naive search quadratic.\n");

        clock_t t;
        size_t spurious = 0, skipped = 0;

        t = clock(); ptrdiff_t r1 = naive_search(text, pat);    double t1 = seconds_since(t);
        size_t c1 = char_comparisons;
        t = clock(); ptrdiff_t r2 = kmp_search(text, pat);      double t2 = seconds_since(t);
        size_t c2 = char_comparisons;
        t = clock(); ptrdiff_t r3 = rabin_karp_search(text, pat, &spurious); double t3 = seconds_since(t);
        size_t c3 = char_comparisons;
        t = clock(); ptrdiff_t r4 = horspool_search(text, pat, &skipped);    double t4 = seconds_since(t);
        size_t c4 = char_comparisons;
        t = clock(); ptrdiff_t r5 = z_search(text, pat);        double t5 = seconds_since(t);

        printf("  %-14s %10s %16s  %s\n", "algorithm", "time", "char comparisons", "result");
        printf("  %-14s %9.4fs %16zu  %td\n", "naive",     t1, c1, r1);
        printf("  %-14s %9.4fs %16zu  %td\n", "kmp",       t2, c2, r2);
        printf("  %-14s %9.4fs %16zu  %td\n", "rabin-karp",t3, c3, r3);
        printf("  %-14s %9.4fs %16zu  %td\n", "horspool",  t4, c4, r4);
        printf("  %-14s %9.4fs %16s  %td\n", "z-algorithm",t5, "(builds z array)", r5);
        printf("\n  naive did %.0fx more character comparisons than KMP.\n",
               c2 ? (double)c1 / (double)c2 : 0);
        printf("  rabin-karp had %zu spurious hash hits (verified and rejected).\n", spurious);

        free(text); free(pat);
    }

    puts("\n=== WHERE HORSPOOL WINS: SUBLINEAR SEARCH ===");
    {
        /* Natural-ish text with a long, distinctive pattern — the case
         * Boyer-Moore was designed for, and what grep actually faces. */
        const size_t N = 2000000;
        char *text = malloc(N + 1);
        unsigned seed = 42;
        for (size_t i = 0; i < N; i++) {
            seed = seed * 1103515245u + 12345u;
            text[i] = (char)('a' + (seed >> 16) % 26);
        }
        text[N] = '\0';
        const char *pat = "zyxwvutsrqponm";           /* almost certainly absent */
        memcpy(text + N - strlen(pat), pat, strlen(pat));   /* plant it at the end */

        clock_t t;
        size_t skipped = 0;

        t = clock(); ptrdiff_t r1 = naive_search(text, pat);  double t1 = seconds_since(t);
        size_t c1 = char_comparisons;
        t = clock(); ptrdiff_t r2 = kmp_search(text, pat);    double t2 = seconds_since(t);
        size_t c2 = char_comparisons;
        t = clock(); ptrdiff_t r3 = horspool_search(text, pat, &skipped); double t3 = seconds_since(t);
        size_t c3 = char_comparisons;

        printf("  %zu chars of random text, a %zu-char pattern at the very end:\n",
               N, strlen(pat));
        printf("    naive    : %.4f s, %9zu comparisons -> %td\n", t1, c1, r1);
        printf("    kmp      : %.4f s, %9zu comparisons -> %td\n", t2, c2, r2);
        printf("    horspool : %.4f s, %9zu comparisons -> %td\n", t3, c3, r3);
        printf("    horspool examined only %.1f%% as many characters as KMP\n",
               100.0 * (double)c3 / (double)c2);
        puts("");
        puts("  Horspool did FEWER comparisons than there are characters in the");
        puts("  text — it is SUBLINEAR. It never even looks at most of the text.");
        puts("  Scanning the pattern backwards means one mismatched character");
        puts("  can rule out an entire pattern-length window at once, and the");
        puts("  longer the pattern, the bigger the skips.");
        puts("");
        puts("  This is why GNU grep is fast: a Boyer-Moore variant that skips");
        puts("  most of the input, plus reading the file with mmap.");
        puts("");
        puts("  NOTE what else this run shows: on RANDOM text, naive search beat");
        puts("  KMP. A mismatch almost always happens on the FIRST character, so");
        puts("  naive never backtracks and its inner loop is tighter than KMP's.");
        puts("  KMP's guarantee is about the WORST case, and you pay a constant");
        puts("  factor for it on ordinary input. Guaranteed bounds are worth");
        puts("  having when the input may be ADVERSARIAL — not otherwise.");

        free(text);
    }

    puts("\n=== FINDING ALL OCCURRENCES, INCLUDING OVERLAPS ===");
    {
        const char *text = "aaaaaa";
        const char *pat  = "aa";
        size_t positions[16];
        size_t count = kmp_find_all(text, pat, positions, 16);

        printf("  all \"%s\" in \"%s\": %zu occurrences at ", pat, text, count);
        for (size_t i = 0; i < count && i < 16; i++) printf("%zu ", positions[i]);
        puts("");
        puts("  Note the OVERLAPS: positions 0,1,2,3,4 — not just 0,2,4.");
        puts("  After a match, KMP resumes at lps[m-1] rather than restarting,");
        puts("  which is exactly what finds overlapping matches for free.");
        puts("  (If you do NOT want overlaps, set j = 0 after a match instead.)");

        const char *dna = "ATCGATCGATCGATCG";
        count = kmp_find_all(dna, "ATCGATCG", positions, 16);
        printf("  \"ATCGATCG\" in \"%s\": %zu at ", dna, count);
        for (size_t i = 0; i < count && i < 16; i++) printf("%zu ", positions[i]);
        puts("");
    }

    puts("\n=== CHOOSING ===");
    puts("                 preprocess  search      best for");
    puts("  naive          none        O(nm)       tiny inputs; it is what");
    puts("                                         strstr() does for short patterns");
    puts("  KMP            O(m)        O(n)        guaranteed linear; STREAMING");
    puts("                                         input you cannot rewind");
    puts("  Rabin-Karp     O(m)        O(n) avg    MULTI-PATTERN search: hash k");
    puts("                                         patterns, one pass finds any");
    puts("  Boyer-Moore    O(m+256)    O(n/m) best FASTEST IN PRACTICE for long");
    puts("                                         patterns; what grep uses");
    puts("  Z-algorithm    O(n+m)      O(n+m)      the z-array solves many other");
    puts("                                         string problems too");
    puts("");
    puts("  IN PRACTICE: call strstr(). glibc's implementation is a tuned");
    puts("  Two-Way algorithm (linear worst case, sublinear typical) and will");
    puts("  beat anything here. Implement these ONCE to understand the ideas —");
    puts("  the failure function and the skip table are genuinely transferable.");
    puts("");
    puts("  WORTH KNOWING ABOUT:");
    puts("    Aho-Corasick   KMP generalised to a TRIE of many patterns at once;");
    puts("                   used by intrusion detection and virus scanners");
    puts("    Suffix array   sort all suffixes; then ANY pattern is a binary");
    puts("                   search away. O(n log n) to build, O(m log n) to query");
    puts("    Suffix tree    a trie of all suffixes; O(m) queries, memory-hungry");
    puts("    Two-Way        what glibc's strstr actually uses");

    return 0;
}
