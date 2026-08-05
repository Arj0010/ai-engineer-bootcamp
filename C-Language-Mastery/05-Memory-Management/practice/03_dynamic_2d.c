/* 03_dynamic_2d.c — allocating 2D data three ways, with cleanup and timings.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 03_dynamic_2d.c -o t && ./t
 *   valgrind --leak-check=full ./t          # must report zero leaks
 *
 * The layout you pick decides your cache behaviour, your allocation count,
 * and how easily you can write the data to a file. Pick deliberately.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ================================================================= *
 * LAYOUT A: contiguous — ONE allocation, index by hand.
 * This is the right default. Module 15's matrix library uses it.
 * ================================================================= */
typedef struct { double *data; size_t rows, cols; } Matrix;

static int mat_alloc(Matrix *m, size_t rows, size_t cols)
{
    /* Overflow-check before multiplying — rows and cols may come from a file. */
    if (rows != 0 && cols > SIZE_MAX / rows) return -1;
    if (rows * cols > SIZE_MAX / sizeof *m->data) return -1;

    m->data = malloc(rows * cols * sizeof *m->data);
    if (m->data == NULL) return -1;
    m->rows = rows; m->cols = cols;
    return 0;
}
static void mat_free(Matrix *m) { free(m->data); m->data = NULL; m->rows = m->cols = 0; }

/* row-major indexing: element (r,c) is at r*cols + c */
#define MAT(m, r, c) ((m)->data[(r) * (m)->cols + (c)])

/* ================================================================= *
 * LAYOUT B: jagged — an array of row pointers, one malloc per row.
 * Only worth it when rows genuinely have DIFFERENT lengths.
 * ================================================================= */
static double **jagged_alloc(size_t rows, size_t cols)
{
    double **m = malloc(rows * sizeof *m);
    if (m == NULL) return NULL;

    for (size_t r = 0; r < rows; r++) {
        m[r] = malloc(cols * sizeof *m[r]);
        if (m[r] == NULL) {
            /* PARTIAL FAILURE CLEANUP — the part everyone forgets. Unwind
             * exactly what we already allocated, then fail. */
            while (r-- > 0) free(m[r]);
            free(m);
            return NULL;
        }
    }
    return m;
}
static void jagged_free(double **m, size_t rows)
{
    if (m == NULL) return;
    for (size_t r = 0; r < rows; r++) free(m[r]);   /* rows first... */
    free(m);                                         /* ...then the index */
    /* Freeing in the other order is a use-after-free: you would be reading
     * m[r] out of a block you already released. */
}

/* ================================================================= *
 * LAYOUT C: hybrid — one contiguous data block PLUS a row-pointer index.
 * You get m[r][c] syntax AND locality, for two allocations.
 * ================================================================= */
typedef struct { double **rows; double *block; size_t nrows, ncols; } Hybrid;

static int hybrid_alloc(Hybrid *h, size_t rows, size_t cols)
{
    if (rows != 0 && cols > SIZE_MAX / rows) return -1;
    h->block = malloc(rows * cols * sizeof *h->block);
    h->rows  = malloc(rows * sizeof *h->rows);
    if (h->block == NULL || h->rows == NULL) {
        free(h->block); free(h->rows);
        h->block = NULL; h->rows = NULL;
        return -1;
    }
    for (size_t r = 0; r < rows; r++) h->rows[r] = h->block + r * cols;
    h->nrows = rows; h->ncols = cols;
    return 0;
}
static void hybrid_free(Hybrid *h)
{
    free(h->rows); free(h->block);
    h->rows = NULL; h->block = NULL; h->nrows = h->ncols = 0;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    const size_t R = 4, C = 5;

    puts("=== LAYOUT A: CONTIGUOUS (one allocation) ===");
    {
        Matrix m;
        if (mat_alloc(&m, R, C) != 0) { perror("alloc"); return 1; }
        for (size_t r = 0; r < R; r++)
            for (size_t c = 0; c < C; c++) MAT(&m, r, c) = (double)(r * C + c);

        for (size_t r = 0; r < R; r++) {
            printf("  ");
            for (size_t c = 0; c < C; c++) printf("%5.1f ", MAT(&m, r, c));
            puts("");
        }
        printf("  1 malloc of %zu bytes at %p\n", R * C * sizeof(double), (void *)m.data);
        printf("  element (2,3) is at data[2*%zu + 3] = data[%zu]\n", C, 2 * C + 3);
        puts("  + one malloc, one free — nothing to leak");
        puts("  + perfectly contiguous: ideal for the cache and the prefetcher");
        puts("  + fwrite(m.data, sizeof(double), R*C, f) writes the whole matrix");
        puts("  + memcpy copies it in one call");
        puts("  - you index by hand (a macro or an accessor fixes that)");
        mat_free(&m);
    }

    puts("\n=== LAYOUT B: JAGGED (array of row pointers) ===");
    {
        double **m = jagged_alloc(R, C);
        if (m == NULL) { perror("alloc"); return 1; }
        for (size_t r = 0; r < R; r++)
            for (size_t c = 0; c < C; c++) m[r][c] = (double)(r * C + c);

        for (size_t r = 0; r < R; r++) {
            printf("  row %zu at %p: ", r, (void *)m[r]);
            for (size_t c = 0; c < C; c++) printf("%5.1f ", m[r][c]);
            puts("");
        }
        printf("  %zu mallocs, %zu frees\n", R + 1, R + 1);
        puts("  + natural m[r][c] syntax");
        puts("  + rows can have DIFFERENT lengths — the only real reason to use it");
        puts("  - rows scattered in memory: every row change is a cache miss");
        puts("  - R+1 allocations and R+1 frees to get right");
        puts("  - an extra pointer dereference per access");
        puts("  - cannot fwrite or memcpy it in one call");
        jagged_free(m, R);
    }

    puts("\n=== LAYOUT C: HYBRID (contiguous data + row index) ===");
    {
        Hybrid h;
        if (hybrid_alloc(&h, R, C) != 0) { perror("alloc"); return 1; }
        for (size_t r = 0; r < R; r++)
            for (size_t c = 0; c < C; c++) h.rows[r][c] = (double)(r * C + c);

        for (size_t r = 0; r < R; r++) {
            printf("  ");
            for (size_t c = 0; c < C; c++) printf("%5.1f ", h.rows[r][c]);
            puts("");
        }
        printf("  2 mallocs; rows are %td doubles apart — contiguous\n",
               h.rows[1] - h.rows[0]);
        puts("  + m[r][c] syntax AND contiguous memory");
        puts("  + still one block to fwrite");
        puts("  - two allocations, and one extra dereference per access");
        hybrid_free(&h);
    }

    puts("\n=== THE ORDER OF FREES MATTERS ===");
    puts("  jagged_free must free the ROWS before the row-pointer array:");
    puts("      for (r...) free(m[r]);     /* needs m to still be valid */");
    puts("      free(m);");
    puts("  Reversing those two lines reads m[r] out of a block you already");
    puts("  released — a use-after-free that ASan catches instantly.");

    puts("\n=== PARTIAL-FAILURE CLEANUP ===");
    puts("  If malloc fails on row 7 of 100, you must free rows 0..6 and the");
    puts("  index before returning. The `while (r-- > 0) free(m[r]);` unwind");
    puts("  in jagged_alloc is that. Skipping it is the single most common");
    puts("  leak in allocation code, because the path is almost never tested.");
    puts("  Layout A has no such path — which is itself an argument for it.");

    puts("\n=== PERFORMANCE: LAYOUT DECIDES SPEED ===");
    {
        const size_t N = 1000;                /* 1000x1000 doubles = 8 MB */
        clock_t t;

        Matrix m;
        if (mat_alloc(&m, N, N) != 0) { perror("alloc"); return 1; }
        double **j = jagged_alloc(N, N);
        if (j == NULL) { mat_free(&m); return 1; }

        for (size_t r = 0; r < N; r++)
            for (size_t c = 0; c < N; c++) MAT(&m, r, c) = j[r][c] = 1.0;

        /* Row-major traversal: sequential in memory for layout A. */
        t = clock();
        double s1 = 0;
        for (size_t r = 0; r < N; r++)
            for (size_t c = 0; c < N; c++) s1 += MAT(&m, r, c);
        double t_contig_rows = seconds_since(t);

        /* Column-major traversal of a row-major array: strides by N*8 bytes,
         * so every single access is a cache miss. */
        t = clock();
        double s2 = 0;
        for (size_t c = 0; c < N; c++)
            for (size_t r = 0; r < N; r++) s2 += MAT(&m, r, c);
        double t_contig_cols = seconds_since(t);

        t = clock();
        double s3 = 0;
        for (size_t r = 0; r < N; r++)
            for (size_t c = 0; c < N; c++) s3 += j[r][c];
        double t_jagged_rows = seconds_since(t);

        t = clock();
        double s4 = 0;
        for (size_t c = 0; c < N; c++)
            for (size_t r = 0; r < N; r++) s4 += j[r][c];
        double t_jagged_cols = seconds_since(t);

        printf("  summing a %zux%zu matrix (%zu MB), same %0.f elements each time:\n",
               N, N, N * N * sizeof(double) / (1024 * 1024), s1);
        printf("    contiguous, row-major order : %.4f s   <- fastest\n", t_contig_rows);
        printf("    contiguous, column order    : %.4f s   (%.1fx slower)\n",
               t_contig_cols, t_contig_rows > 0 ? t_contig_cols / t_contig_rows : 0);
        printf("    jagged, row-major order     : %.4f s\n", t_jagged_rows);
        printf("    jagged, column order        : %.4f s\n", t_jagged_cols);
        (void)s2; (void)s3; (void)s4;
        puts("");
        puts("  Identical arithmetic, identical results, very different times.");
        puts("  Row-major traversal reads memory sequentially: one cache line");
        puts("  (64 bytes) brings in 8 doubles and the prefetcher predicts the");
        puts("  next line. Column traversal touches one useful double per line.");
        puts("  THE LOOP ORDER IS THE OPTIMISATION. Module 13 measures this properly.");

        mat_free(&m);
        jagged_free(j, N);
    }

    puts("\n=== WHICH TO USE ===");
    puts("  CONTIGUOUS  the default. Rectangular data, numerical work, anything");
    puts("              you will write to a file or hand to a library (BLAS,");
    puts("              OpenGL, and every ML framework expect this layout).");
    puts("  JAGGED      only when rows genuinely differ in length — a list of");
    puts("              strings, a sparse adjacency list, ragged records.");
    puts("  HYBRID      when you want m[r][c] syntax over contiguous memory and");
    puts("              can afford the extra indirection.");
    puts("");
    puts("  For a compile-time-known size, just use int m[R][C] on the stack —");
    puts("  it is already contiguous and there is nothing to free.");

    return 0;
}
