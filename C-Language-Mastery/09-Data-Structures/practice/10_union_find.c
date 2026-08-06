/* 10_union_find.c — disjoint-set union. Nearly O(1) per operation, from
 * two tiny optimisations applied to an obvious idea.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 10_union_find.c -o t && ./t
 *
 * THE PROBLEM: maintain a partition of n elements under two operations:
 *   find(x)      which group is x in?
 *   union(x, y)  merge x's group with y's
 *
 * The naive answer (a group id per element, rewritten on every union) makes
 * union O(n). The trick is to represent each group as a TREE, with the root
 * as the group's identity — and then keep those trees flat.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

typedef struct {
    int    *parent;    /* parent[i] == i means i IS a root */
    int    *rank;      /* an UPPER BOUND on the tree's height */
    size_t  n;
    size_t  n_sets;    /* how many disjoint sets remain */
    size_t  path_steps;/* instrumentation */
} DSU;

static bool dsu_init(DSU *d, size_t n)
{
    d->parent = malloc(n * sizeof *d->parent);
    d->rank   = calloc(n, sizeof *d->rank);
    if (d->parent == NULL || d->rank == NULL) { free(d->parent); free(d->rank); return false; }
    for (size_t i = 0; i < n; i++) d->parent[i] = (int)i;   /* everyone alone */
    d->n = n; d->n_sets = n; d->path_steps = 0;
    return true;
}
static void dsu_free(DSU *d) { free(d->parent); free(d->rank); memset(d, 0, sizeof *d); }

/* FIND without any optimisation: walk to the root. O(height). */
static int find_plain(DSU *d, int x)
{
    while (d->parent[x] != x) { d->path_steps++; x = d->parent[x]; }
    return x;
}

/* OPTIMISATION 1: PATH COMPRESSION.
 *
 * On the way back from the root, point EVERY node on the path directly at
 * the root. The next find on any of them is a single step. The work you had
 * to do anyway also flattens the tree for everyone behind you. */
static int find(DSU *d, int x)
{
    int root = x;
    while (d->parent[root] != root) { d->path_steps++; root = d->parent[root]; }

    while (d->parent[x] != root) {          /* second pass: re-point everything */
        int next = d->parent[x];
        d->parent[x] = root;
        x = next;
    }
    return root;
}

/* OPTIMISATION 2: UNION BY RANK.
 *
 * Always hang the SHORTER tree under the taller one. Doing it the other way
 * would grow the height; this way the height only grows when both trees are
 * the same height, so it grows at most log n times. */
static bool dsu_union(DSU *d, int a, int b)
{
    int ra = find(d, a), rb = find(d, b);
    if (ra == rb) return false;             /* already together */

    if (d->rank[ra] < d->rank[rb])      d->parent[ra] = rb;
    else if (d->rank[ra] > d->rank[rb]) d->parent[rb] = ra;
    else { d->parent[rb] = ra; d->rank[ra]++; }   /* equal: pick one, height +1 */

    d->n_sets--;
    return true;
}

static bool dsu_connected(DSU *d, int a, int b) { return find(d, a) == find(d, b); }

static void dsu_print(DSU *d, const char *label)
{
    printf("  %-28s parent: [", label);
    for (size_t i = 0; i < d->n; i++) printf("%d%s", d->parent[i], i + 1 < d->n ? " " : "");
    printf("]  %zu set(s)\n", d->n_sets);
}

/* ================================================================= *
 * KRUSKAL'S MINIMUM SPANNING TREE — the classic union-find application.
 *
 * Sort every edge by weight, then greedily take each one UNLESS it would
 * form a cycle. "Would it form a cycle?" is exactly "are these two vertices
 * already connected?", which is what union-find answers in ~O(1).
 * ================================================================= */
typedef struct { int u, v, weight; } WEdge;

static int cmp_edge(const void *a, const void *b)
{
    const WEdge *x = a, *y = b;
    return (x->weight > y->weight) - (x->weight < y->weight);
}

static int kruskal(WEdge *edges, size_t n_edges, size_t n_vertices,
                   WEdge *mst_out, size_t *mst_count)
{
    qsort(edges, n_edges, sizeof *edges, cmp_edge);      /* cheapest first */

    DSU d;
    if (!dsu_init(&d, n_vertices)) return -1;

    int total = 0;
    *mst_count = 0;

    for (size_t i = 0; i < n_edges; i++) {
        /* If u and v are already connected, adding this edge makes a CYCLE. */
        if (!dsu_union(&d, edges[i].u, edges[i].v)) continue;

        mst_out[(*mst_count)++] = edges[i];
        total += edges[i].weight;
        if (*mst_count == n_vertices - 1) break;         /* a tree is done */
    }
    dsu_free(&d);
    return total;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== THE PROBLEM ===");
    puts("  Maintain a partition of n elements under:");
    puts("      find(x)      which group is x in?");
    puts("      union(x, y)  merge their groups");
    puts("");
    puts("  Naive approach: store a group id per element. find is O(1), but");
    puts("  union has to rewrite every member of one group — O(n).");
    puts("");
    puts("  The trick: represent each group as a TREE. The ROOT is the group's");
    puts("  identity, so union is just 'point one root at the other' — O(1),");
    puts("  provided you can find the roots quickly.\n");

    puts("=== THE BASIC STRUCTURE ===");
    {
        DSU d; dsu_init(&d, 8);
        dsu_print(&d, "8 elements, all separate");
        puts("      parent[i] == i means i is a ROOT (its own group)");

        dsu_union(&d, 0, 1);  dsu_print(&d, "union(0,1)");
        dsu_union(&d, 2, 3);  dsu_print(&d, "union(2,3)");
        dsu_union(&d, 0, 2);  dsu_print(&d, "union(0,2)");
        dsu_union(&d, 4, 5);  dsu_print(&d, "union(4,5)");

        printf("\n  connected(1, 3)? %s   (through 0 and 2)\n",
               dsu_connected(&d, 1, 3) ? "yes" : "no");
        printf("  connected(1, 5)? %s\n", dsu_connected(&d, 1, 5) ? "yes" : "no");
        printf("  connected(6, 6)? %s\n", dsu_connected(&d, 6, 6) ? "yes" : "no");
        printf("  union(0,1) again -> %s\n",
               dsu_union(&d, 0, 1) ? "merged?!" : "correctly rejected (same set)");
        printf("  %zu disjoint sets remain\n", d.n_sets);
        dsu_free(&d);
    }

    puts("\n=== OPTIMISATION 1: PATH COMPRESSION ===");
    {
        DSU d; dsu_init(&d, 10);

        /* Build a deliberately tall chain by always attaching to the tail. */
        for (int i = 1; i < 10; i++) d.parent[i] = i - 1;
        d.n_sets = 1;
        dsu_print(&d, "a chain 9->8->...->0");

        d.path_steps = 0;
        int r = find_plain(&d, 9);
        printf("  find_plain(9) = %d after walking %zu edges\n", r, d.path_steps);
        dsu_print(&d, "tree UNCHANGED");

        d.path_steps = 0;
        r = find(&d, 9);
        printf("\n  find(9) with compression = %d, walked %zu edges\n", r, d.path_steps);
        dsu_print(&d, "tree FLATTENED");
        puts("      every node on the path now points DIRECTLY at the root");

        d.path_steps = 0;
        find(&d, 9);
        printf("  find(9) again: %zu edge(s)  <- one step now\n", d.path_steps);
        puts("");
        puts("  Path compression does the flattening as a SIDE EFFECT of work you");
        puts("  had to do anyway. You were walking to the root regardless; a");
        puts("  second pass re-points the whole path, and every future find on");
        puts("  any of those nodes is O(1).");
        dsu_free(&d);
    }

    puts("\n=== OPTIMISATION 2: UNION BY RANK ===");
    puts("  When merging two trees, hang the SHORTER one under the TALLER one.");
    puts("");
    puts("    WRONG                  RIGHT");
    puts("    tall under short       short under tall");
    puts("        s                      t");
    puts("        |                     / \\");
    puts("        t                    ...  s");
    puts("       / \\                   height UNCHANGED");
    puts("      ... ...");
    puts("      height GREW");
    puts("");
    puts("  The height only increases when both trees are the SAME height, and");
    puts("  that can happen at most log2(n) times — because a tree of height h");
    puts("  built this way has at least 2^h nodes.");
    puts("");
    puts("  `rank` is an UPPER BOUND on the height, not the exact height:");
    puts("  path compression flattens trees without decrementing it. That is");
    puts("  fine — it only ever makes the bound conservative.");

    puts("\n=== THE COMBINED EFFECT ===");
    {
        const size_t N = 1000000;
        DSU d; dsu_init(&d, N);

        clock_t t = clock();
        /* A pathological union pattern: chain everything together. */
        for (size_t i = 1; i < N; i++) dsu_union(&d, (int)(i - 1), (int)i);
        double t_union = seconds_since(t);

        d.path_steps = 0;
        t = clock();
        for (size_t i = 0; i < N; i++) find(&d, (int)i);
        double t_find = seconds_since(t);

        printf("  %zu elements, %zu unions, %zu finds:\n", N, N - 1, N);
        printf("    unions : %.4f s\n", t_union);
        printf("    finds  : %.4f s, %zu total tree steps (%.2f per find)\n",
               t_find, d.path_steps, (double)d.path_steps / (double)N);
        printf("    sets remaining: %zu\n", d.n_sets);
        puts("");
        puts("  With BOTH optimisations, m operations on n elements cost");
        puts("  O(m * alpha(n)), where alpha is the INVERSE ACKERMANN function.");
        puts("  alpha(n) <= 4 for any n that fits in the observable universe, so");
        puts("  this is effectively constant time — but it is provably NOT");
        puts("  constant, which is one of the more beautiful results in the field.");
        dsu_free(&d);
    }

    puts("\n=== KRUSKAL'S MINIMUM SPANNING TREE ===");
    {
        /* 7 vertices A..G */
        WEdge edges[] = {
            {0,1,7}, {0,3,5}, {1,2,8}, {1,3,9}, {1,4,7},
            {2,4,5}, {3,4,15}, {3,5,6}, {4,5,8}, {4,6,9}, {5,6,11},
        };
        size_t n_edges = sizeof edges / sizeof edges[0];
        const size_t V = 7;

        puts("  a weighted undirected graph (the classic textbook example):");
        for (size_t i = 0; i < n_edges; i++)
            printf("    %c-%c weight %2d\n", 'A' + edges[i].u, 'A' + edges[i].v, edges[i].weight);

        WEdge mst[16];
        size_t mst_n = 0;
        int total = kruskal(edges, n_edges, V, mst, &mst_n);

        printf("\n  minimum spanning tree (%zu edges for %zu vertices):\n", mst_n, V);
        for (size_t i = 0; i < mst_n; i++)
            printf("    %c-%c weight %2d\n", 'A' + mst[i].u, 'A' + mst[i].v, mst[i].weight);
        printf("  total weight: %d\n", total);
        printf("  edge count is V-1 = %zu, as any spanning TREE must be\n", V - 1);
        puts("");
        puts("  THE ALGORITHM, in full:");
        puts("    1. sort the edges by weight            O(E log E)");
        puts("    2. for each edge, cheapest first:");
        puts("         if its endpoints are ALREADY connected, skip it");
        puts("         (adding it would create a cycle)");
        puts("         otherwise take it and union the two sets");
        puts("    3. stop after V-1 edges");
        puts("");
        puts("  Step 2's question — 'are these already connected?' — is exactly");
        puts("  what union-find answers in ~O(1). Without it you would run a DFS");
        puts("  per edge, turning O(E log E) into O(V*E).");
    }

    puts("\n=== WHERE UNION-FIND SHOWS UP ===");
    puts("  Kruskal's MST                network design, clustering");
    puts("  connected components         incrementally, as edges arrive");
    puts("  cycle detection              in an UNDIRECTED graph");
    puts("  image segmentation           merge adjacent similar pixels");
    puts("  percolation                  does a path exist top to bottom?");
    puts("  type inference               Hindley-Milner unification");
    puts("  'friend circles' problems    the standard interview framing");
    puts("");
    puts("  ITS LIMITATION: there is no efficient SPLIT. Union-find merges only.");
    puts("  If you need to remove edges, you need link-cut trees or an offline");
    puts("  algorithm that processes the operations in reverse.");
    puts("");
    puts("  The whole structure is about 25 lines. It is the best");
    puts("  power-to-complexity ratio of anything in this module.");

    return 0;
}
