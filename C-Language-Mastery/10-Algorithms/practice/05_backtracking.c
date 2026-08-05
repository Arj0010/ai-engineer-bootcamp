/* 05_backtracking.c — try, recurse, UNDO. A DFS over partial solutions.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 05_backtracking.c -o t && ./t
 *
 * THE SHAPE, always:
 *
 *     solve(state):
 *         if state is complete:  record it; return
 *         for each candidate:
 *             if candidate is valid:      <- PRUNING lives here
 *                 apply(candidate)        <- make the move
 *                 solve(state)            <- recurse
 *                 undo(candidate)         <- BACKTRACK
 *
 * The `undo` is what makes it backtracking rather than plain recursion, and
 * the validity test is what makes it finish in your lifetime. Every program
 * here counts the nodes it explored, so pruning's effect is measurable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

static size_t nodes_explored, solutions_found;

/* ================================================================= *
 * 1. N-QUEENS
 *
 * Place N queens on an NxN board so none attacks another.
 *
 * KEY INSIGHT: exactly one queen per column, so instead of choosing from
 * N^2 squares, choose a ROW for each COLUMN. That alone cuts the search
 * space from C(64,8) = 4.4 billion to 8^8 = 16.7 million for N=8.
 * ================================================================= */
typedef struct {
    int  n;
    int *row_of_col;      /* row_of_col[c] = which row the queen in column c is on */
    bool *row_used;       /* is this row taken?                    */
    bool *diag1_used;     /* "\" diagonals, indexed by (r + c)     */
    bool *diag2_used;     /* "/" diagonals, indexed by (r - c + n)  */
} Queens;

static bool queens_init(Queens *q, int n)
{
    q->n = n;
    q->row_of_col = malloc((size_t)n * sizeof *q->row_of_col);
    q->row_used   = calloc((size_t)n, sizeof *q->row_used);
    q->diag1_used = calloc((size_t)(2 * n), sizeof *q->diag1_used);
    q->diag2_used = calloc((size_t)(2 * n), sizeof *q->diag2_used);
    return q->row_of_col && q->row_used && q->diag1_used && q->diag2_used;
}
static void queens_free(Queens *q)
{ free(q->row_of_col); free(q->row_used); free(q->diag1_used); free(q->diag2_used); }

static void queens_print(const Queens *q)
{
    for (int r = 0; r < q->n; r++) {
        printf("      ");
        for (int c = 0; c < q->n; c++) printf("%s ", q->row_of_col[c] == r ? "Q" : ".");
        puts("");
    }
    puts("");
}

/* The O(1) validity test is the whole trick. Scanning previous columns
 * would be O(n) per check; three boolean arrays make it constant. */
static void queens_solve(Queens *q, int col, bool print_first)
{
    nodes_explored++;

    if (col == q->n) {                            /* COMPLETE */
        solutions_found++;
        if (print_first && solutions_found == 1) queens_print(q);
        return;
    }

    for (int row = 0; row < q->n; row++) {
        int d1 = row + col;                        /* "\" diagonal */
        int d2 = row - col + q->n;                 /* "/" diagonal */

        /* PRUNE: reject immediately if this row or either diagonal is used. */
        if (q->row_used[row] || q->diag1_used[d1] || q->diag2_used[d2]) continue;

        /* MAKE THE MOVE */
        q->row_of_col[col] = row;
        q->row_used[row] = q->diag1_used[d1] = q->diag2_used[d2] = true;

        queens_solve(q, col + 1, print_first);     /* RECURSE */

        /* UNDO — this is the "backtrack" */
        q->row_used[row] = q->diag1_used[d1] = q->diag2_used[d2] = false;
    }
}

/* The same search with NO pruning, to measure what pruning is worth. */
static bool queens_conflicts(const int *rows, int col, int row)
{
    for (int c = 0; c < col; c++)
        if (rows[c] == row || abs(rows[c] - row) == abs(c - col)) return true;
    return false;
}
static void queens_no_prune(int *rows, int n, int col, size_t *nodes, size_t *sols)
{
    (*nodes)++;
    if (col == n) { (*sols)++; return; }

    for (int row = 0; row < n; row++) {
        rows[col] = row;                           /* place it WITHOUT checking */
        /* Only validate at the very end — the naive approach. */
        if (col == n - 1) {
            bool ok = true;
            for (int c = 0; c < n && ok; c++)
                if (queens_conflicts(rows, c, rows[c])) ok = false;
            if (ok) { (*nodes)++; (*sols)++; }
        } else {
            queens_no_prune(rows, n, col + 1, nodes, sols);
        }
    }
}

/* ================================================================= *
 * 2. SUDOKU SOLVER
 * ================================================================= */
/* NOTE: `int g[9][9]`, not `const int g[9][9]`.
 *
 * C does NOT implicitly convert `int (*)[9]` to `const int (*)[9]`. Adding
 * const at the TOP level of a pointer is fine; adding it one level down is
 * not. This is a genuine C wart (C++ allows it, and C23 finally does too) —
 * under -Wpedantic GCC reports "invalid use of pointers to arrays with
 * different qualifiers". The portable fixes are to drop the const, or to
 * take `const int (*g)[9]` and have the CALLER pass a matching pointer. */
static bool sudoku_valid(int g[9][9], int r, int c, int v)
{
    for (int i = 0; i < 9; i++) {
        if (g[r][i] == v) return false;                     /* row */
        if (g[i][c] == v) return false;                     /* column */
    }
    int br = (r / 3) * 3, bc = (c / 3) * 3;                 /* 3x3 box */
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (g[br + i][bc + j] == v) return false;
    return true;
}

static bool sudoku_solve(int g[9][9], size_t *nodes)
{
    (*nodes)++;

    /* Find the next empty cell. (A big optimisation, not done here for
     * clarity: pick the cell with the FEWEST legal candidates — the
     * "most constrained variable" heuristic. It typically cuts the search
     * by orders of magnitude.) */
    int row = -1, col = -1;
    for (int r = 0; r < 9 && row < 0; r++)
        for (int c = 0; c < 9; c++)
            if (g[r][c] == 0) { row = r; col = c; break; }

    if (row < 0) return true;                    /* no empty cells: SOLVED */

    for (int v = 1; v <= 9; v++) {
        if (!sudoku_valid(g, row, col, v)) continue;

        g[row][col] = v;                          /* make the move */
        if (sudoku_solve(g, nodes)) return true;  /* recurse; keep it if it worked */
        g[row][col] = 0;                          /* UNDO */
    }
    return false;                                 /* no value works: backtrack */
}

static void sudoku_print(int g[9][9])
{
    for (int r = 0; r < 9; r++) {
        if (r % 3 == 0) puts("      +-------+-------+-------+");
        printf("      ");
        for (int c = 0; c < 9; c++) {
            if (c % 3 == 0) printf("| ");
            if (g[r][c]) printf("%d ", g[r][c]); else printf(". ");
        }
        puts("|");
    }
    puts("      +-------+-------+-------+");
}

/* ================================================================= *
 * 3. PERMUTATIONS — via swapping, which needs no "used" array
 * ================================================================= */
static void permute(int *a, int k, int n, int *count, bool print)
{
    if (k == n) {
        (*count)++;
        if (print && *count <= 6) {
            printf("      ");
            for (int i = 0; i < n; i++) printf("%d ", a[i]);
            puts("");
        }
        return;
    }
    for (int i = k; i < n; i++) {
        int t = a[k]; a[k] = a[i]; a[i] = t;      /* swap into position k */
        permute(a, k + 1, n, count, print);
        t = a[k]; a[k] = a[i]; a[i] = t;          /* UNDO the swap */
    }
}

/* ================================================================= *
 * 4. SUBSETS (the power set) — include or exclude, at every element
 * ================================================================= */
static void subsets(const int *a, int n, int k, int *current, int len,
                    int *count, bool print)
{
    if (k == n) {
        (*count)++;
        if (print) {
            printf("      { ");
            for (int i = 0; i < len; i++) printf("%d ", current[i]);
            puts("}");
        }
        return;
    }
    /* EXCLUDE a[k] */
    subsets(a, n, k + 1, current, len, count, print);
    /* INCLUDE a[k] */
    current[len] = a[k];
    subsets(a, n, k + 1, current, len + 1, count, print);
    /* nothing to undo: `len` is passed by value, which IS the undo */
}

/* ================================================================= *
 * 5. SUBSET SUM — the same shape, but with pruning that actually bites
 * ================================================================= */
static bool subset_sum(const int *a, int n, int k, int target,
                       int *chosen, int n_chosen, size_t *nodes,
                       int *out, int *out_len)
{
    (*nodes)++;

    if (target == 0) {                             /* found it */
        for (int i = 0; i < n_chosen; i++) out[i] = chosen[i];
        *out_len = n_chosen;
        return true;
    }
    if (target < 0 || k == n) return false;        /* PRUNE: overshot or ran out */

    /* PRUNE HARDER: if everything remaining still cannot reach the target,
     * stop now rather than exploring the whole subtree. */
    int remaining = 0;
    for (int i = k; i < n; i++) remaining += a[i];
    if (remaining < target) return false;

    chosen[n_chosen] = a[k];                       /* take a[k] */
    if (subset_sum(a, n, k + 1, target - a[k], chosen, n_chosen + 1, nodes, out, out_len))
        return true;
    /* UNDO is implicit: n_chosen is by value */

    return subset_sum(a, n, k + 1, target, chosen, n_chosen, nodes, out, out_len);
}

/* ================================================================= *
 * 6. RAT IN A MAZE — backtracking on a grid
 * ================================================================= */
#define MAZE_N 6
static bool maze_solve(int maze[MAZE_N][MAZE_N], int sol[MAZE_N][MAZE_N],
                       int r, int c, size_t *nodes)
{
    (*nodes)++;
    if (r == MAZE_N - 1 && c == MAZE_N - 1 && maze[r][c] == 1) {
        sol[r][c] = 1;
        return true;
    }
    /* PRUNE: off the grid, into a wall, or already on our own path */
    if (r < 0 || r >= MAZE_N || c < 0 || c >= MAZE_N) return false;
    if (maze[r][c] == 0 || sol[r][c] == 1) return false;

    sol[r][c] = 1;                                 /* step here */

    if (maze_solve(maze, sol, r + 1, c, nodes)) return true;   /* down  */
    if (maze_solve(maze, sol, r, c + 1, nodes)) return true;   /* right */
    if (maze_solve(maze, sol, r - 1, c, nodes)) return true;   /* up    */
    if (maze_solve(maze, sol, r, c - 1, nodes)) return true;   /* left  */

    sol[r][c] = 0;                                 /* UNDO: step back off */
    return false;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE BACKTRACKING TEMPLATE ===");
    puts("      solve(state):");
    puts("          if complete:  record; return");
    puts("          for each candidate:");
    puts("              if valid:            <- PRUNING");
    puts("                  apply(candidate)");
    puts("                  solve(state)");
    puts("                  undo(candidate)  <- THE BACKTRACK");
    puts("");
    puts("  It is a DFS over the tree of PARTIAL solutions. Pruning cuts whole");
    puts("  subtrees before you walk into them, and it is the difference");
    puts("  between seconds and centuries.\n");

    puts("=== 1. N-QUEENS ===");
    {
        Queens q;
        queens_init(&q, 8);
        nodes_explored = solutions_found = 0;

        clock_t t = clock();
        queens_solve(&q, 0, true);
        double elapsed = seconds_since(t);

        printf("    8-queens: %zu solutions, %zu nodes explored, %.4f s\n",
               solutions_found, nodes_explored, elapsed);
        queens_free(&q);

        puts("\n    solutions for each board size:");
        for (int n = 4; n <= 11; n++) {
            Queens qq;
            queens_init(&qq, n);
            nodes_explored = solutions_found = 0;
            t = clock();
            queens_solve(&qq, 0, false);
            elapsed = seconds_since(t);
            printf("      %2d-queens: %6zu solutions, %9zu nodes, %.4f s\n",
                   n, solutions_found, nodes_explored, elapsed);
            queens_free(&qq);
        }
    }

    puts("\n=== WHAT PRUNING IS WORTH ===");
    {
        for (int n = 6; n <= 9; n++) {
            Queens q; queens_init(&q, n);
            nodes_explored = solutions_found = 0;
            clock_t t = clock();
            queens_solve(&q, 0, false);
            double t_pruned = seconds_since(t);
            size_t n_pruned = nodes_explored;
            size_t s_pruned = solutions_found;
            queens_free(&q);

            int *rows = malloc((size_t)n * sizeof *rows);
            size_t n_naive = 0, s_naive = 0;
            t = clock();
            queens_no_prune(rows, n, 0, &n_naive, &s_naive);
            double t_naive = seconds_since(t);
            free(rows);

            printf("  n=%d  pruned: %8zu nodes %.4f s | naive: %10zu nodes %.4f s"
                   "  (%.0fx more work)\n",
                   n, n_pruned, t_pruned, n_naive, t_naive,
                   (double)n_naive / (double)n_pruned);
            if (s_pruned != s_naive)
                printf("       (solution counts differ: %zu vs %zu)\n", s_pruned, s_naive);
        }
        puts("");
        puts("  Both explore the same tree. The pruned version REJECTS a branch");
        puts("  as soon as it is provably hopeless; the naive one builds every");
        puts("  complete assignment and checks at the end. Same answer, orders");
        puts("  of magnitude more work.");
        puts("");
        puts("  Two things made the pruned version fast:");
        puts("    1. one queen per COLUMN by construction — the search space is");
        puts("       n^n, not C(n^2, n)");
        puts("    2. three boolean arrays make the validity test O(1) instead");
        puts("       of O(n)");
    }

    puts("\n=== 2. SUDOKU ===");
    {
        int puzzle[9][9] = {
            {5,3,0, 0,7,0, 0,0,0}, {6,0,0, 1,9,5, 0,0,0}, {0,9,8, 0,0,0, 0,6,0},
            {8,0,0, 0,6,0, 0,0,3}, {4,0,0, 8,0,3, 0,0,1}, {7,0,0, 0,2,0, 0,0,6},
            {0,6,0, 0,0,0, 2,8,0}, {0,0,0, 4,1,9, 0,0,5}, {0,0,0, 0,8,0, 0,7,9},
        };
        puts("    puzzle:");
        sudoku_print(puzzle);

        size_t nodes = 0;
        clock_t t = clock();
        bool solved = sudoku_solve(puzzle, &nodes);
        double elapsed = seconds_since(t);

        printf("    %s in %zu nodes, %.4f s:\n",
               solved ? "SOLVED" : "no solution", nodes, elapsed);
        sudoku_print(puzzle);

        puts("    A 9x9 sudoku has 6.7 x 10^21 possible fillings. Backtracking");
        puts("    with constraint checking finds the answer in a few thousand");
        puts("    nodes, because an invalid partial grid is rejected the moment");
        puts("    it becomes invalid — not after it is complete.");
        puts("");
        puts("    The big further optimisation (not done here, for clarity) is");
        puts("    the MOST CONSTRAINED VARIABLE heuristic: always fill the cell");
        puts("    with the FEWEST legal candidates. It typically cuts the node");
        puts("    count by another order of magnitude, and it is the standard");
        puts("    technique in real constraint solvers.");
    }

    puts("\n=== 3. PERMUTATIONS ===");
    {
        int a[] = {1, 2, 3, 4};
        int count = 0;
        puts("    permutations of {1,2,3,4} (first six):");
        permute(a, 0, 4, &count, true);
        printf("    total: %d = 4! \n", count);

        for (int n = 5; n <= 9; n++) {
            int *b = malloc((size_t)n * sizeof *b);
            for (int i = 0; i < n; i++) b[i] = i + 1;
            count = 0;
            clock_t t = clock();
            permute(b, 0, n, &count, false);
            printf("    %d elements: %8d permutations in %.4f s\n",
                   n, count, seconds_since(t));
            free(b);
        }
        puts("    O(n!) — 10 elements is 3.6 million, 13 is 6 billion, 20 is");
        puts("    more than the age of the universe in nanoseconds. Any");
        puts("    'try every ordering' approach is only usable for tiny n.");
        puts("");
        puts("    The SWAP formulation needs no `used` array: swapping element i");
        puts("    into position k and swapping it back IS the make/undo pair.");
    }

    puts("\n=== 4. SUBSETS (the power set) ===");
    {
        int a[] = {1, 2, 3};
        int current[8];
        int count = 0;
        puts("    all subsets of {1,2,3}:");
        subsets(a, 3, 0, current, 0, &count, true);
        printf("    total: %d = 2^3\n", count);

        for (int n = 10; n <= 22; n += 4) {
            int *b = malloc((size_t)n * sizeof *b);
            int *cur = malloc((size_t)n * sizeof *cur);
            for (int i = 0; i < n; i++) b[i] = i;
            count = 0;
            clock_t t = clock();
            subsets(b, n, 0, cur, 0, &count, false);
            printf("    %2d elements: %8d subsets in %.4f s\n",
                   n, count, seconds_since(t));
            free(b); free(cur);
        }
        puts("    O(2^n). Each element is a binary include/exclude decision, so");
        puts("    the recursion tree is a perfect binary tree of depth n.");
        puts("    (For subsets specifically, iterating a bitmask from 0 to 2^n-1");
        puts("     is simpler and faster — see module 11's bit manipulation.)");
    }

    puts("\n=== 5. SUBSET SUM, WITH PRUNING THAT BITES ===");
    {
        int a[] = {3, 34, 4, 12, 5, 2, 7, 8, 15, 21};
        int n = (int)(sizeof a / sizeof a[0]);
        int chosen[16], out[16], out_len = 0;

        printf("    set: ");
        for (int i = 0; i < n; i++) printf("%d ", a[i]);
        puts("");

        int targets[] = {9, 30, 100, 111, 1};
        for (size_t t = 0; t < 5; t++) {
            size_t nodes = 0;
            bool found = subset_sum(a, n, 0, targets[t], chosen, 0, &nodes, out, &out_len);
            printf("    target %3d: ", targets[t]);
            if (found) {
                printf("YES  ");
                for (int i = 0; i < out_len; i++) printf("%d%s", out[i], i + 1 < out_len ? "+" : "");
                printf(" = %d", targets[t]);
            } else {
                printf("no");
            }
            printf("   (%zu nodes)\n", nodes);
        }
        puts("");
        puts("    Three prunes are at work:");
        puts("      target < 0        we overshot — no point continuing");
        puts("      k == n            ran out of elements");
        puts("      remaining < target  even taking EVERYTHING left falls short");
        puts("    That third one is the powerful one: it rejects an entire");
        puts("    subtree using a cheap arithmetic bound. Finding good bounds");
        puts("    like this is the core skill in branch-and-bound search.");
    }

    puts("\n=== 6. RAT IN A MAZE ===");
    {
        int maze[MAZE_N][MAZE_N] = {
            {1,0,1,1,1,1}, {1,1,1,0,1,1}, {0,1,0,1,1,0},
            {1,1,1,1,0,1}, {1,0,0,1,1,1}, {1,1,0,0,1,1},
        };
        int sol[MAZE_N][MAZE_N] = {{0}};
        size_t nodes = 0;

        puts("    maze (1 = open, 0 = wall):");
        for (int r = 0; r < MAZE_N; r++) {
            printf("      ");
            for (int c = 0; c < MAZE_N; c++) printf("%c ", maze[r][c] ? '.' : '#');
            puts("");
        }

        if (maze_solve(maze, sol, 0, 0, &nodes)) {
            printf("    path found in %zu nodes:\n", nodes);
            for (int r = 0; r < MAZE_N; r++) {
                printf("      ");
                for (int c = 0; c < MAZE_N; c++)
                    printf("%c ", sol[r][c] ? '*' : (maze[r][c] ? '.' : '#'));
                puts("");
            }
        } else {
            printf("    no path (explored %zu nodes)\n", nodes);
        }
        puts("    `sol[r][c] = 0` on the way out is the backtrack: it removes");
        puts("    the square from the current path so a different route may use");
        puts("    it. Forget that line and the rat walks into a dead end and");
        puts("    permanently blocks itself out of the rest of the maze.");
    }

    puts("\n=== BACKTRACKING vs DYNAMIC PROGRAMMING ===");
    puts("  Both explore a space of choices. The difference:");
    puts("    DP            subproblems OVERLAP, so cache and reuse answers.");
    puts("                  Returns an OPTIMAL VALUE.");
    puts("    BACKTRACKING  subproblems are mostly DISTINCT, so there is");
    puts("                  nothing to reuse. Returns ACTUAL SOLUTIONS —");
    puts("                  all of them, or the first one found.");
    puts("");
    puts("  Subset-sum is the interesting boundary case: as a yes/no question");
    puts("  it is a DP; as 'show me the actual subset' it is backtracking.");
    puts("");
    puts("  MAKING BACKTRACKING FAST:");
    puts("    1. PRUNE EARLY. Reject a partial solution the instant it becomes");
    puts("       impossible, not when it is complete.");
    puts("    2. Make the validity test O(1) with incremental state (the three");
    puts("       boolean arrays in n-queens).");
    puts("    3. ORDER the candidates well. Most-constrained-first typically");
    puts("       cuts the tree by an order of magnitude.");
    puts("    4. Find a BOUND. If the best possible completion is still worse");
    puts("       than the answer you already have, abandon the branch. That is");
    puts("       branch and bound.");
    puts("    5. Break SYMMETRY. In n-queens, solutions come in mirrored pairs;");
    puts("       fixing the first queen to the left half halves the work.");

    return 0;
}
