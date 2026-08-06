/* 01_control_flow.c — selection, iteration, and the escape hatches.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_control_flow.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ROWS 4
#define COLS 5

/* ---------------------------------------------------------------- *
 * switch: fallthrough, both accidental and deliberate.
 * ---------------------------------------------------------------- */
static const char *classify(int c)
{
    switch (c) {
    /* Deliberate fallthrough: several labels, one body. This form is fine
     * and never warns, because there is no code between the labels. */
    case 'a': case 'e': case 'i': case 'o': case 'u':
        return "vowel";

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return "digit";

    case ' ': case '\t': case '\n':
        return "whitespace";

    default:
        return (c >= 'a' && c <= 'z') ? "consonant" : "other";
    }
}

/* Deliberate fallthrough WITH code in between — this is the form that needs
 * an explicit marker, or -Wextra will (correctly) complain. */
static int count_down_actions(int level)
{
    int actions = 0;
    switch (level) {
    case 3:
        actions++;              /* level 3 does this AND everything below */
        __attribute__((fallthrough));
    case 2:
        actions++;
        __attribute__((fallthrough));
    case 1:
        actions++;
        break;
    default:
        break;
    }
    return actions;
}

/* ---------------------------------------------------------------- *
 * goto for cleanup — the one idiomatic use of goto in C.
 * C has no destructors and no `finally`, so this is how you guarantee
 * every acquired resource is released on every exit path.
 * ---------------------------------------------------------------- */
static int process_with_cleanup(size_t n, bool simulate_failure)
{
    int   rc   = -1;        /* pessimistic: assume failure */
    int  *bufa = NULL;
    int  *bufb = NULL;
    FILE *f    = NULL;

    bufa = malloc(n * sizeof *bufa);
    if (bufa == NULL) goto out;                 /* nothing to undo yet */

    bufb = malloc(n * sizeof *bufb);
    if (bufb == NULL) goto out;                 /* out frees bufa */

    f = fopen("/dev/null", "w");
    if (f == NULL) goto out;

    if (simulate_failure) {
        fprintf(stderr, "  (simulated failure partway through)\n");
        goto out;                               /* out frees BOTH and closes f */
    }

    for (size_t i = 0; i < n; i++) { bufa[i] = (int)i; bufb[i] = bufa[i] * 2; }
    fprintf(f, "%d\n", bufb[n - 1]);
    rc = 0;                                     /* success */

out:
    /* Release in reverse order of acquisition. free(NULL) is defined as a
     * no-op, which is what makes this single exit path work for every case. */
    if (f != NULL) fclose(f);
    free(bufb);
    free(bufa);
    return rc;
}

int main(void)
{
    /* ---------------- truthiness ---------------- */
    puts("=== everything scalar is a condition ===");
    {
        int n = 0;
        int *p = NULL;
        const char *s = "abc";
        printf("  if (0)      -> %s\n", n ? "true" : "false");
        printf("  if (NULL)   -> %s\n", p ? "true" : "false");
        printf("  if (-1)     -> %s   <- NON-ZERO is true, not just 1\n", -1 ? "true" : "false");
        printf("  if (0.0)    -> %s\n", 0.0 ? "true" : "false");
        printf("  strcmp(\"abc\",\"abc\") == 0 -> %s   (strcmp returns 0 for EQUAL)\n",
               strcmp(s, "abc") == 0 ? "true" : "false");
        puts("  Write `strcmp(a,b) == 0`, not `!strcmp(a,b)` — it reads backwards.");
    }

    /* ---------------- dangling else / unbraced bodies ---------------- */
    puts("\n=== why you brace EVERY body ===");
    puts("      if (x)");
    puts("          a();");
    puts("          b();     <- NOT part of the if. Runs unconditionally.");
    puts("  This exact shape produced Apple's 2014 'goto fail' TLS vulnerability.");

    /* ---------------- switch ---------------- */
    puts("\n=== switch ===");
    {
        const char *samples = "ae9 z!";
        for (const char *p = samples; *p != '\0'; p++)
            printf("  '%c' -> %s\n", *p, classify((unsigned char)*p));
        printf("  count_down_actions(3) = %d (deliberate fallthrough)\n",
               count_down_actions(3));
        printf("  count_down_actions(1) = %d\n", count_down_actions(1));
    }

    /* ---------------- loops ---------------- */
    puts("\n=== the three loops ===");
    {
        printf("  while:    ");
        int i = 0;
        while (i < 5) { printf("%d ", i); i++; }

        printf("\n  do-while: ");
        i = 10;
        do { printf("%d ", i); i++; } while (i < 5);   /* body runs ONCE */
        puts("  <- do-while always executes at least once");

        printf("  for:      ");
        for (int j = 0; j < 5; j++) printf("%d ", j);

        printf("\n  reverse:  ");
        for (int j = 4; j >= 0; j--) printf("%d ", j);

        /* Counting down with an UNSIGNED index needs this idiom, because
         * `i >= 0` is always true for unsigned and loops forever. */
        printf("\n  unsigned reverse: ");
        for (size_t k = 5; k-- > 0; ) printf("%zu ", k);
        puts("  <- the `k-- > 0` idiom");

        printf("  two counters: ");
        for (int a = 0, b = 5; a < b; a++, b--) printf("(%d,%d) ", a, b);
        puts("");
    }

    /* ---------------- break / continue ---------------- */
    puts("\n=== break and continue ===");
    {
        printf("  odd numbers under 10 via continue: ");
        for (int i = 0; i < 10; i++) {
            if (i % 2 == 0) continue;      /* skip to the i++ step */
            printf("%d ", i);
        }
        printf("\n  first multiple of 7 over 20 via break: ");
        for (int i = 21; ; i++) {
            if (i % 7 == 0) { printf("%d\n", i); break; }
        }
    }

    /* ---------------- escaping nested loops ---------------- */
    puts("\n=== escaping nested loops (C has no labelled break) ===");
    {
        int grid[ROWS][COLS];
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                grid[r][c] = r * COLS + c;
        const int target = 13;

        /* option 1: a flag in the outer condition */
        int fr = -1, fc = -1;
        bool found = false;
        for (int r = 0; r < ROWS && !found; r++)
            for (int c = 0; c < COLS; c++)
                if (grid[r][c] == target) { fr = r; fc = c; found = true; break; }
        printf("  flag  : found %d at (%d,%d)\n", target, fr, fc);

        /* option 2: goto — arguably the clearest, and jumps only forward+out */
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++)
                if (grid[r][c] == target) { fr = r; fc = c; goto done; }
        fr = fc = -1;
    done:
        printf("  goto  : found %d at (%d,%d)\n", target, fr, fc);
        puts("  option 3 (best): extract the search into a function and return.");
    }

    /* ---------------- goto for cleanup ---------------- */
    puts("\n=== goto for resource cleanup ===");
    printf("  success path : rc = %d\n", process_with_cleanup(16, false));
    printf("  failure path : rc = %d (no leaks — verify with valgrind)\n",
           process_with_cleanup(16, true));
    puts("  Rules: jump FORWARD only, jump OUT of scopes only, one label per");
    puts("  resource level, release in reverse order of acquisition.");

    return 0;
}
