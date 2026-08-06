/* 01_arrays.c — arrays: initialisation, decay, 2D layout, and the sizeof trap.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_arrays.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROWS 3
#define COLS 4

/* THE CLASSIC BUG. `arr` here is NOT an array — it is a pointer. The [] in
 * the parameter list is decorative; the compiler rewrites it to `int *arr`.
 * So sizeof arr is 8 (pointer size) and this returns 2, always.
 *
 * GCC and Clang both catch this with -Wsizeof-array-argument (part of -Wall).
 * The warning is silenced HERE ONLY so the demo still builds clean — in your
 * own code, that warning means you have this exact bug. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-array-argument"
static size_t broken_length(int arr[])
{
    return sizeof arr / sizeof arr[0];
}
#pragma GCC diagnostic pop

/* The only correct signature: pointer AND length, always together. */
static long long sum(const int *arr, size_t n)
{
    long long total = 0;
    for (size_t i = 0; i < n; i++) total += arr[i];
    return total;
}

/* Passing a 2D array: every dimension except the first must be known,
 * because the compiler needs the row stride to compute g[r][c]. */
static int sum_2d_fixed(int g[][COLS], int rows)
{
    int total = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < COLS; c++) total += g[r][c];
    return total;
}

/* C99 variably-modified parameter: dimensions come first, then the array.
 * The cleanest form, and it works with any shape. */
static int sum_2d_vla(int rows, int cols, int g[rows][cols])
{
    int total = 0;
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++) total += g[r][c];
    return total;
}

/* The portable-everywhere form: treat it as flat and index manually.
 * This is what you use when you allocate the matrix yourself (module 05). */
static int sum_2d_flat(const int *g, int rows, int cols)
{
    int total = 0;
    for (int i = 0; i < rows * cols; i++) total += g[i];
    return total;
}

int main(void)
{
    puts("=== INITIALISATION ===");
    {
        int a[5];                          /* garbage — do not read before writing */
        int b[5] = {1, 2, 3, 4, 5};
        int c[5] = {1, 2};                 /* rest are ZERO, guaranteed */
        int d[5] = {0};                    /* the "all zeros" idiom */
        int e[]  = {1, 2, 3};              /* size deduced from the initialiser */
        int f[5] = {[4] = 9, [0] = 1};     /* designated initialisers (C99) */

        memset(a, 0, sizeof a);            /* the runtime way to zero an array */

        #define SHOW(arr) do {                                  \
            printf("  %-24s = {", #arr);                        \
            for (size_t i = 0; i < sizeof arr / sizeof arr[0]; i++) \
                printf("%d%s", arr[i],                          \
                       i + 1 < sizeof arr / sizeof arr[0] ? ", " : ""); \
            printf("}  (%zu elements)\n", sizeof arr / sizeof arr[0]); \
        } while (0)

        SHOW(a); SHOW(b); SHOW(c); SHOW(d); SHOW(e); SHOW(f);
        #undef SHOW
    }

    puts("\n=== THE sizeof IDIOM — and where it breaks ===");
    {
        int arr[10] = {0};
        printf("  in main, where the array is in scope:\n");
        printf("    sizeof arr           = %zu bytes\n", sizeof arr);
        printf("    sizeof arr[0]        = %zu bytes\n", sizeof arr[0]);
        printf("    length               = %zu   <- correct\n",
               sizeof arr / sizeof arr[0]);
        printf("  after passing to a function:\n");
        printf("    broken_length(arr)   = %zu   <- WRONG, it is 8/4\n",
               broken_length(arr));
        puts("    The array DECAYED to a pointer at the call. The length is gone.");
        printf("    sum(arr, 10)         = %lld  <- pass the length explicitly\n",
               sum(arr, 10));
    }

    puts("\n=== ARRAY DECAY ===");
    {
        int a[5] = {10, 20, 30, 40, 50};
        int *p = a;                        /* implicit: same as &a[0] */

        printf("  a          = %p\n", (void *)a);
        printf("  &a[0]      = %p   <- identical\n", (void *)&a[0]);
        printf("  &a         = %p   <- same ADDRESS, different TYPE: int (*)[5]\n", (void *)&a);
        printf("  a + 1      = %p   (+%zu bytes: one int)\n",
               (void *)(a + 1), sizeof(int));
        printf("  &a + 1     = %p   (+%zu bytes: the WHOLE array)\n",
               (void *)(&a + 1), sizeof a);

        puts("\n  These four are all the same element:");
        printf("    a[2]      = %d\n", a[2]);
        printf("    *(a + 2)  = %d\n", *(a + 2));
        printf("    p[2]      = %d\n", p[2]);
        printf("    2[a]      = %d   <- legal C, because a[i] IS *(a+i)\n", 2[a]);

        puts("\n  The three places an array does NOT decay:");
        printf("    sizeof a  -> %zu (the array), not %zu (a pointer)\n",
               sizeof a, sizeof(int *));
        puts("    &a        -> int (*)[5], pointer to the whole array");
        puts("    char s[] = \"hi\"  -> copies the literal, does not point at it");
    }

    puts("\n=== 2D ARRAYS ARE ONE CONTIGUOUS BLOCK (row-major) ===");
    {
        int grid[ROWS][COLS];
        for (int r = 0; r < ROWS; r++)
            for (int c = 0; c < COLS; c++) grid[r][c] = r * COLS + c;

        puts("  logical view:");
        for (int r = 0; r < ROWS; r++) {
            printf("    ");
            for (int c = 0; c < COLS; c++) printf("%3d ", grid[r][c]);
            putchar('\n');
        }

        puts("  physical memory (one flat run of ints — row 0, then row 1, ...):");
        printf("    ");
        const int *flat = &grid[0][0];
        for (int i = 0; i < ROWS * COLS; i++) printf("%d ", flat[i]);
        puts("");

        printf("  sizeof grid       = %zu (whole thing)\n", sizeof grid);
        printf("  sizeof grid[0]    = %zu (one row)\n", sizeof grid[0]);
        printf("  sizeof grid[0][0] = %zu (one element)\n", sizeof grid[0][0]);
        printf("  rows = %zu, cols = %zu\n",
               sizeof grid / sizeof grid[0], sizeof grid[0] / sizeof grid[0][0]);

        puts("\n  grid[1][2] is computed as *(*(grid + 1) + 2):");
        printf("    grid[1][2]              = %d\n", grid[1][2]);
        printf("    *(*(grid + 1) + 2)      = %d\n", *(*(grid + 1) + 2));
        printf("    flat[1 * COLS + 2]      = %d   <- the manual form\n",
               flat[1 * COLS + 2]);

        printf("\n  three ways to pass it: fixed=%d  vla=%d  flat=%d\n",
               sum_2d_fixed(grid, ROWS),
               sum_2d_vla(ROWS, COLS, grid),
               sum_2d_flat(flat, ROWS, COLS));
    }

    puts("\n=== ROW-MAJOR ORDER IS A PERFORMANCE FACT, NOT A DETAIL ===");
    puts("  for (r...) for (c...) m[r][c]   walks memory sequentially — fast");
    puts("  for (c...) for (r...) m[r][c]   jumps `cols` ints each step — slow");
    puts("  On a big matrix the difference is often 5-10x. Module 13 measures it.");

    puts("\n=== NO BOUNDS CHECKING. NONE. ===");
    {
        int a[5] = {0};
        printf("  int a[5] has valid indices 0..4, and a[4] = %d\n", a[4]);
        printf("  a[5] would read past the end: undefined behaviour.\n");
        printf("  &a[5] is legal to COMPUTE (one-past-the-end is a valid address)\n");
        printf("  but dereferencing it is not. &a[6] is UB even to compute.\n");
        puts("  Catch these with:  -fsanitize=address");
        puts("  or at compile time with -Wall -O2 for constant indices.");
    }

    puts("\n=== VARIABLE-LENGTH ARRAYS: know them, avoid them ===");
    puts("  void f(int n) { int a[n]; }    /* C99; OPTIONAL since C11 */");
    puts("  - allocated on the STACK, so a large or attacker-controlled n is a");
    puts("    stack overflow, i.e. a remotely triggerable crash");
    puts("  - sizeof a is computed at RUN TIME (the one case where sizeof");
    puts("    evaluates its operand)");
    puts("  - MSVC does not support them; the Linux kernel removed all of them");
    puts("  Use malloc for anything whose size is not a compile-time constant.");

    puts("\n=== ARRAYS CANNOT BE ASSIGNED OR COMPARED ===");
    {
        int a[3] = {1, 2, 3}, b[3];
        /* b = a;            <- does not compile */
        /* if (a == b)       <- compiles, but compares ADDRESSES, always false */
        memcpy(b, a, sizeof a);                       /* copy */
        printf("  memcpy(b, a, sizeof a) -> b = {%d,%d,%d}\n", b[0], b[1], b[2]);
        printf("  memcmp(a, b, sizeof a) == 0 -> %s   (byte-wise comparison)\n",
               memcmp(a, b, sizeof a) == 0 ? "equal" : "different");
        puts("  A struct CAN be assigned with =. An array cannot. Wrapping an");
        puts("  array in a struct is the standard trick to make it copyable.");
    }

    return 0;
}
