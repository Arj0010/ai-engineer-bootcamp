/* 05_multilevel_pointers.c — T** in its three distinct roles.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 05_multilevel_pointers.c -o t && ./t
 *   valgrind --leak-check=full ./t          # should report zero leaks
 *
 * `int **` is not one concept. Recognising WHICH of the three you are looking
 * at is most of the battle.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================= *
 * ROLE 1: an out-parameter that is itself a pointer.
 *
 * The callee must change the CALLER'S pointer, so it needs the address
 * of that pointer. This is why allocating functions take T**.
 * ================================================================= */
static int alloc_and_fill(int **out, size_t n)
{
    if (out == NULL) return -1;
    int *p = malloc(n * sizeof *p);
    if (p == NULL) { *out = NULL; return -1; }
    for (size_t i = 0; i < n; i++) p[i] = (int)(i * i);
    *out = p;                       /* writes THROUGH to the caller's variable */
    return 0;
}

/* Free and NULL in one step — the callee needs T** to null the caller's pointer.
 * This idiom eliminates a whole class of use-after-free bugs. */
static void free_and_null(void **pp)
{
    if (pp != NULL && *pp != NULL) { free(*pp); *pp = NULL; }
}

/* WRONG version, for contrast: this can only null its own local copy. */
static void free_but_cannot_null(void *p) { free(p); }

/* ================================================================= *
 * ROLE 2: an array of pointers.
 *
 * char *argv[] IS char **argv. Each element points somewhere independent.
 * ================================================================= */
static void print_string_array(const char *const *strings, size_t n, const char *label)
{
    printf("  %s: ", label);
    for (size_t i = 0; i < n; i++) printf("[%s] ", strings[i]);
    puts("");
}

static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* ================================================================= *
 * ROLE 3: a jagged 2D structure.
 *
 * int **matrix is an array of ROW POINTERS, each pointing at a separately
 * allocated row. This is a DIFFERENT MEMORY LAYOUT from int m[3][4], and
 * they are NOT interchangeable — passing one where the other is expected
 * reinterprets pointers as ints and crashes.
 * ================================================================= */
static int **alloc_jagged(size_t rows, size_t cols)
{
    int **m = malloc(rows * sizeof *m);            /* the array of row pointers */
    if (m == NULL) return NULL;
    for (size_t r = 0; r < rows; r++) {
        m[r] = malloc(cols * sizeof *m[r]);        /* each row, separately */
        if (m[r] == NULL) {                        /* clean up what we already got */
            while (r-- > 0) free(m[r]);
            free(m);
            return NULL;
        }
        for (size_t c = 0; c < cols; c++) m[r][c] = (int)(r * cols + c);
    }
    return m;
}
static void free_jagged(int **m, size_t rows)
{
    if (m == NULL) return;
    for (size_t r = 0; r < rows; r++) free(m[r]);   /* rows first */
    free(m);                                        /* then the row array */
}

/* The better layout in almost every case: ONE allocation, indexed by hand.
 * Contiguous memory means one malloc, one free, and cache-friendly traversal. */
static int *alloc_contiguous(size_t rows, size_t cols)
{
    int *m = malloc(rows * cols * sizeof *m);
    if (m == NULL) return NULL;
    for (size_t i = 0; i < rows * cols; i++) m[i] = (int)i;
    return m;
}
#define AT(m, cols, r, c) ((m)[(r) * (cols) + (c)])

/* A hybrid: one data block, plus a row-pointer array INTO it. You get
 * m[r][c] syntax and contiguous memory. Two allocations, two frees. */
static int **alloc_hybrid(size_t rows, size_t cols, int **out_block)
{
    int  *block = malloc(rows * cols * sizeof *block);
    int **index = malloc(rows * sizeof *index);
    if (block == NULL || index == NULL) { free(block); free(index); return NULL; }
    for (size_t r = 0; r < rows; r++) index[r] = block + r * cols;
    for (size_t i = 0; i < rows * cols; i++) block[i] = (int)i * 10;
    *out_block = block;
    return index;
}

int main(int argc, char **argv)
{
    puts("=== ROLE 1: T** AS AN OUT-PARAMETER ===");
    {
        int *data = NULL;
        if (alloc_and_fill(&data, 6) == 0) {
            printf("  alloc_and_fill(&data, 6) -> ");
            for (int i = 0; i < 6; i++) printf("%d ", data[i]);
            puts("");
        }
        printf("  data before free_and_null: %p\n", (void *)data);
        free_and_null((void **)&data);
        printf("  data after  free_and_null: %p   <- nulled, so a later use crashes\n",
               (void *)data);
        puts("                                     loudly instead of corrupting memory");

        int *other = malloc(8);
        free_but_cannot_null(other);
        puts("  free_but_cannot_null(other) freed the block, but `other` still");
        puts("  holds the old address — a DANGLING pointer. Passing T* instead of");
        puts("  T** is why so many codebases have use-after-free bugs.");
        other = NULL;                 /* the caller must remember, every time */
        (void)other;

        puts("\n  Why the extra level: passing `data` copies the POINTER, so the");
        puts("  callee can only change its own copy. Passing `&data` copies the");
        puts("  ADDRESS OF the pointer, which is enough to reach the original.");
    }

    puts("\n=== ROLE 2: AN ARRAY OF POINTERS ===");
    {
        /* Each element points at a separate string, anywhere in memory. */
        const char *fruits[] = {"cherry", "apple", "banana", "date"};
        size_t n = sizeof fruits / sizeof fruits[0];

        print_string_array(fruits, n, "before sort");
        qsort(fruits, n, sizeof fruits[0], cmp_str);
        print_string_array(fruits, n, "after sort ");
        puts("  qsort moved 8-byte POINTERS, not the string bytes. Sorting an");
        puts("  array of pointers is O(n log n) pointer swaps regardless of how");
        puts("  long the strings are.");

        printf("\n  the array itself is contiguous; the strings are not:\n");
        for (size_t i = 0; i < n; i++)
            printf("    &fruits[%zu] = %p  ->  %p  \"%s\"\n",
                   i, (void *)&fruits[i], (const void *)fruits[i], fruits[i]);

        printf("\n  argv is exactly this shape: char **argv, %d entries + NULL\n", argc);
        for (int i = 0; i < argc; i++) printf("    argv[%d] = \"%s\"\n", i, argv[i]);
        printf("    argv[%d] = %p   <- guaranteed NULL terminator\n",
               argc, (void *)argv[argc]);
    }

    puts("\n=== ROLE 3: 2D DATA — THREE LAYOUTS, THREE TRADE-OFFS ===");
    {
        const size_t ROWS = 3, COLS = 4;

        /* --- (a) jagged: int**, one malloc per row --- */
        int **jag = alloc_jagged(ROWS, COLS);
        if (jag != NULL) {
            puts("  (a) int **jagged — an array of row pointers");
            for (size_t r = 0; r < ROWS; r++) {
                printf("      row %zu at %p: ", r, (void *)jag[r]);
                for (size_t c = 0; c < COLS; c++) printf("%3d ", jag[r][c]);
                puts("");
            }
            printf("      %zu allocations, %zu frees. Rows may be ANYWHERE —\n",
                   ROWS + 1, ROWS + 1);
            puts("      note the addresses are not evenly spaced. Bad for cache.");
            puts("      Only worth it when rows genuinely have different lengths.");
            free_jagged(jag, ROWS);
        }

        /* --- (b) contiguous: one block, manual indexing --- */
        int *flat = alloc_contiguous(ROWS, COLS);
        if (flat != NULL) {
            puts("\n  (b) int *contiguous — ONE block, index as r*cols + c");
            for (size_t r = 0; r < ROWS; r++) {
                printf("      ");
                for (size_t c = 0; c < COLS; c++) printf("%3d ", AT(flat, COLS, r, c));
                puts("");
            }
            puts("      1 allocation, 1 free, perfect cache locality, and you can");
            puts("      memcpy/fwrite the whole matrix in one call.");
            puts("      THIS IS THE DEFAULT CHOICE. Module 15's matrix library uses it.");
            free(flat);
        }

        /* --- (c) hybrid: contiguous data + a row index --- */
        int *block = NULL;
        int **hyb = alloc_hybrid(ROWS, COLS, &block);
        if (hyb != NULL) {
            puts("\n  (c) hybrid — one data block PLUS a row-pointer index");
            for (size_t r = 0; r < ROWS; r++) {
                printf("      ");
                for (size_t c = 0; c < COLS; c++) printf("%4d ", hyb[r][c]);
                puts("");
            }
            printf("      rows are %td ints apart — contiguous, as intended\n",
                   hyb[1] - hyb[0]);
            puts("      You get m[r][c] syntax AND locality. 2 allocations.");
            free(hyb); free(block);
        }

        /* --- the trap --- */
        int stack_2d[3][4];
        for (int r = 0; r < 3; r++) for (int c = 0; c < 4; c++) stack_2d[r][c] = r * 4 + c;
        puts("\n  THE TRAP: int m[3][4] is NOT convertible to int**.");
        printf("      int m[3][4] is %zu contiguous bytes with NO pointers in it.\n",
               sizeof stack_2d);
        puts("      int **p    is an array of ADDRESSES.");
        puts("      Passing m where int** is expected makes the callee read your");
        puts("      integers AS addresses and dereference them. Instant segfault.");
        puts("      m decays to int (*)[4] — a pointer to an array of 4 ints —");
        puts("      which is why the parameter must be declared int m[][4].");
    }

    puts("\n=== HOW MANY STARS? Count the levels of indirection you must CHANGE ===");
    puts("  int x;        a value");
    puts("  int *p;       to read/write someone else's int");
    puts("  int **pp;     to read/write someone else's POINTER");
    puts("  int ***ppp;   almost always a design smell — wrap it in a struct");
    puts("");
    puts("  If you find yourself at three stars, stop and define a type:");
    puts("      typedef struct { int **rows; size_t nrows, ncols; } Matrix;");
    puts("  A struct with named fields beats counting asterisks, every time.");

    return 0;
}
