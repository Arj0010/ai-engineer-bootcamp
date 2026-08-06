/* 06_graph_algorithms.c — shortest paths and minimum spanning trees.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 06_graph_algorithms.c -o t && ./t
 *
 * Module 09 covered BFS and DFS on UNWEIGHTED graphs. Adding weights changes
 * everything: BFS's "first time I reach a vertex is the shortest way" is no
 * longer true, because a longer path in HOPS can be cheaper in WEIGHT.
 *
 *   Dijkstra        non-negative weights      O((V+E) log V)
 *   Bellman-Ford    any weights               O(VE), detects negative cycles
 *   Floyd-Warshall  all pairs                 O(V^3)
 *   Prim / Kruskal  minimum spanning tree     O(E log V)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_V 16
#define INF   INT_MAX

/* ================================================================= *
 * GRAPH: adjacency list of weighted edges
 * ================================================================= */
typedef struct Edge { int to, weight; struct Edge *next; } Edge;
typedef struct {
    Edge *adj[MAX_V];
    char  name[MAX_V][8];
    int   n;
    bool  directed;
} Graph;

static void graph_init(Graph *g, int n, bool directed)
{
    memset(g, 0, sizeof *g);
    g->n = n; g->directed = directed;
    for (int i = 0; i < n; i++) snprintf(g->name[i], sizeof g->name[i], "%c", 'A' + i);
}
static void graph_free(Graph *g)
{
    for (int i = 0; i < g->n; i++) {
        Edge *e = g->adj[i];
        while (e) { Edge *nx = e->next; free(e); e = nx; }
        g->adj[i] = NULL;
    }
}
static void add_edge(Graph *g, int u, int v, int w)
{
    Edge *e = malloc(sizeof *e);
    e->to = v; e->weight = w; e->next = g->adj[u]; g->adj[u] = e;
    if (!g->directed) {
        Edge *b = malloc(sizeof *b);
        b->to = u; b->weight = w; b->next = g->adj[v]; g->adj[v] = b;
    }
}

/* ================================================================= *
 * A MIN-HEAP keyed by distance, for Dijkstra and Prim.
 * ================================================================= */
typedef struct { int vertex, key; } HeapItem;
typedef struct { HeapItem *a; int len, cap; } MinHeap;

static void heap_init(MinHeap *h, int cap) { h->a = malloc((size_t)cap * sizeof *h->a); h->len = 0; h->cap = cap; }
static void heap_free(MinHeap *h) { free(h->a); h->a = NULL; h->len = h->cap = 0; }
static void heap_push(MinHeap *h, int vertex, int key)
{
    if (h->len == h->cap) { h->cap *= 2; h->a = realloc(h->a, (size_t)h->cap * sizeof *h->a); }
    int i = h->len++;
    h->a[i] = (HeapItem){vertex, key};
    while (i > 0) {                                   /* sift up */
        int p = (i - 1) / 2;
        if (h->a[p].key <= h->a[i].key) break;
        HeapItem t = h->a[p]; h->a[p] = h->a[i]; h->a[i] = t;
        i = p;
    }
}
static bool heap_pop(MinHeap *h, HeapItem *out)
{
    if (h->len == 0) return false;
    *out = h->a[0];
    h->a[0] = h->a[--h->len];
    int i = 0;
    for (;;) {                                        /* sift down */
        int l = 2*i+1, r = 2*i+2, small = i;
        if (l < h->len && h->a[l].key < h->a[small].key) small = l;
        if (r < h->len && h->a[r].key < h->a[small].key) small = r;
        if (small == i) break;
        HeapItem t = h->a[i]; h->a[i] = h->a[small]; h->a[small] = t;
        i = small;
    }
    return true;
}

/* ================================================================= *
 * DIJKSTRA — always expand the CLOSEST unvisited vertex.
 *
 * WHY IT NEEDS NON-NEGATIVE WEIGHTS: the algorithm finalises a vertex the
 * moment it pops it, on the argument that no cheaper route can exist —
 * every other route goes through something already further away. A negative
 * edge breaks that argument outright, and the answer is silently wrong.
 *
 * "LAZY DELETION": rather than decrease a key inside the heap (which needs
 * an index map), push a NEW entry and skip stale pops. The heap may hold up
 * to E entries instead of V, which costs a little memory and saves a great
 * deal of complexity. This is what most real implementations do.
 * ================================================================= */
static void dijkstra(const Graph *g, int src, int *dist, int *parent)
{
    bool done[MAX_V] = {false};
    for (int i = 0; i < g->n; i++) { dist[i] = INF; parent[i] = -1; }
    dist[src] = 0;

    MinHeap h; heap_init(&h, 64);
    heap_push(&h, src, 0);

    HeapItem item;
    while (heap_pop(&h, &item)) {
        int u = item.vertex;
        if (done[u]) continue;                 /* a stale entry — skip it */
        done[u] = true;                        /* FINALISED: dist[u] is optimal */

        for (const Edge *e = g->adj[u]; e; e = e->next) {
            if (dist[u] == INF) continue;
            int alt = dist[u] + e->weight;     /* RELAXATION */
            if (alt < dist[e->to]) {
                dist[e->to] = alt;
                parent[e->to] = u;
                heap_push(&h, e->to, alt);     /* lazy decrease-key */
            }
        }
    }
    heap_free(&h);
}

/* ================================================================= *
 * BELLMAN-FORD — relax EVERY edge, V-1 times.
 *
 * WHY V-1 PASSES: any shortest path has at most V-1 edges (more would mean
 * revisiting a vertex, i.e. a cycle). Each pass guarantees at least one more
 * edge of every shortest path is settled.
 *
 * A V-th pass that still improves something proves a NEGATIVE CYCLE exists —
 * you can loop forever and keep getting cheaper, so "shortest path" is
 * undefined. Detecting that is Bellman-Ford's real selling point.
 * ================================================================= */
typedef struct { int u, v, w; } WEdge;

static bool bellman_ford(const WEdge *edges, int n_edges, int n_vertices,
                         int src, int *dist, int *parent, int *neg_cycle_vertex)
{
    for (int i = 0; i < n_vertices; i++) { dist[i] = INF; parent[i] = -1; }
    dist[src] = 0;

    for (int pass = 0; pass < n_vertices - 1; pass++) {
        bool changed = false;
        for (int e = 0; e < n_edges; e++) {
            if (dist[edges[e].u] == INF) continue;
            int alt = dist[edges[e].u] + edges[e].w;
            if (alt < dist[edges[e].v]) {
                dist[edges[e].v] = alt;
                parent[edges[e].v] = edges[e].u;
                changed = true;
            }
        }
        if (!changed) break;                   /* converged early */
    }

    /* One more pass: any further improvement means a negative cycle. */
    for (int e = 0; e < n_edges; e++) {
        if (dist[edges[e].u] == INF) continue;
        if (dist[edges[e].u] + edges[e].w < dist[edges[e].v]) {
            if (neg_cycle_vertex) *neg_cycle_vertex = edges[e].v;
            return false;
        }
    }
    return true;
}

/* ================================================================= *
 * FLOYD-WARSHALL — all pairs, in three nested loops.
 *
 * THE INSIGHT: after considering k as an intermediate vertex,
 *     dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])
 * The k loop MUST be outermost. Getting the loop order wrong is the
 * classic bug and produces plausible-looking wrong answers.
 * ================================================================= */
static void floyd_warshall(int dist[MAX_V][MAX_V], int next[MAX_V][MAX_V], int n)
{
    for (int k = 0; k < n; k++)                /* k OUTERMOST */
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                if (dist[i][k] == INF || dist[k][j] == INF) continue;
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next[i][j] = next[i][k];   /* for path reconstruction */
                }
            }
}

/* ================================================================= *
 * PRIM — grow ONE tree, always adding the cheapest edge leaving it.
 * (Kruskal, the other MST algorithm, is in module 09's union-find file.)
 * ================================================================= */
static int prim(const Graph *g, int start, int *parent)
{
    int key[MAX_V];
    bool in_tree[MAX_V] = {false};
    for (int i = 0; i < g->n; i++) { key[i] = INF; parent[i] = -1; }
    key[start] = 0;

    MinHeap h; heap_init(&h, 64);
    heap_push(&h, start, 0);

    int total = 0;
    HeapItem item;
    while (heap_pop(&h, &item)) {
        int u = item.vertex;
        if (in_tree[u]) continue;
        in_tree[u] = true;
        total += key[u];

        for (const Edge *e = g->adj[u]; e; e = e->next)
            if (!in_tree[e->to] && e->weight < key[e->to]) {
                key[e->to] = e->weight;        /* note: the EDGE weight, not a */
                parent[e->to] = u;             /* running total — that is the  */
                heap_push(&h, e->to, e->weight); /* only difference from Dijkstra */
            }
    }
    heap_free(&h);
    return total;
}

static void print_path(const Graph *g, const int *parent, int v)
{
    if (parent[v] != -1) { print_path(g, parent, parent[v]); printf(" -> "); }
    printf("%s", g->name[v]);
}

int main(void)
{
    puts("=== WHY BFS IS NOT ENOUGH ONCE EDGES HAVE WEIGHTS ===");
    puts("  BFS finds the path with the fewest HOPS. With weights, the cheapest");
    puts("  path may have MORE hops:");
    puts("      A --100--> C          1 hop,  cost 100");
    puts("      A --1--> B --1--> C   2 hops, cost 2      <- cheaper!");
    puts("  So you must track accumulated COST, not depth.\n");

    /* A weighted directed graph, all weights non-negative. */
    Graph g;
    graph_init(&g, 6, true);
    add_edge(&g, 0, 1, 4);  add_edge(&g, 0, 2, 1);
    add_edge(&g, 2, 1, 2);  add_edge(&g, 1, 3, 5);
    add_edge(&g, 2, 3, 8);  add_edge(&g, 3, 4, 3);
    add_edge(&g, 2, 4, 10); add_edge(&g, 4, 5, 1);
    add_edge(&g, 3, 5, 6);

    puts("=== DIJKSTRA ===");
    {
        puts("  graph (directed, weighted):");
        for (int v = 0; v < g.n; v++) {
            printf("    %s ->", g.name[v]);
            for (const Edge *e = g.adj[v]; e; e = e->next)
                printf(" %s(%d)", g.name[e->to], e->weight);
            puts("");
        }

        int dist[MAX_V], parent[MAX_V];
        dijkstra(&g, 0, dist, parent);

        puts("\n  shortest paths from A:");
        for (int v = 0; v < g.n; v++) {
            if (dist[v] == INF) { printf("    %s: unreachable\n", g.name[v]); continue; }
            printf("    %s: cost %2d   path ", g.name[v], dist[v]);
            print_path(&g, parent, v);
            puts("");
        }
        puts("\n  Note A -> B: the direct edge costs 4, but going A -> C -> B");
        puts("  costs 1 + 2 = 3. Dijkstra found the cheaper two-hop route, which");
        puts("  BFS would have missed entirely.");
        puts("");
        puts("  HOW IT WORKS: keep a frontier in a min-heap keyed by best-known");
        puts("  distance. Repeatedly pop the CLOSEST unfinished vertex and");
        puts("  RELAX its edges (try to improve each neighbour's distance).");
        puts("  When a vertex is popped, its distance is final.");
        puts("");
        puts("  WHY IT NEEDS NON-NEGATIVE WEIGHTS: finalising on pop assumes no");
        puts("  cheaper route can appear later, because every alternative goes");
        puts("  through something already further away. A negative edge destroys");
        puts("  that argument, and Dijkstra returns a WRONG ANSWER with no error.");
    }

    puts("\n=== BELLMAN-FORD AND NEGATIVE WEIGHTS ===");
    {
        /* The same shape, but with a negative edge. */
        WEdge edges[] = {
            {0,1,4}, {0,2,1}, {2,1,-3}, {1,3,5}, {2,3,8},
            {3,4,3}, {2,4,10}, {4,5,1}, {3,5,6},
        };
        int n_edges = (int)(sizeof edges / sizeof edges[0]);
        int dist[MAX_V], parent[MAX_V], bad = -1;

        printf("  the same graph, but C -> B now costs -3\n");
        if (bellman_ford(edges, n_edges, 6, 0, dist, parent, &bad)) {
            puts("  shortest paths from A (negative edge handled correctly):");
            for (int v = 0; v < 6; v++)
                if (dist[v] != INF) printf("    %c: cost %d\n", 'A' + v, dist[v]);
        }

        puts("\n  now adding a NEGATIVE CYCLE (D -> C costing -20):");
        WEdge cyc[16];
        memcpy(cyc, edges, sizeof edges);
        cyc[n_edges] = (WEdge){3, 2, -20};
        if (!bellman_ford(cyc, n_edges + 1, 6, 0, dist, parent, &bad))
            printf("    NEGATIVE CYCLE DETECTED (reachable at vertex %c)\n", 'A' + bad);
        puts("    With a negative cycle, 'shortest path' has no meaning: loop");
        puts("    round it forever and the cost keeps dropping. Bellman-Ford");
        puts("    reports this; Dijkstra would loop or return nonsense.");
        puts("");
        puts("  HOW IT WORKS: relax EVERY edge, V-1 times. Any shortest path has");
        puts("  at most V-1 edges (more would repeat a vertex), and each pass");
        puts("  settles at least one more edge of every shortest path.");
        puts("  A V-th pass that STILL improves something proves a negative cycle.");
        puts("");
        puts("  O(VE) versus Dijkstra's O((V+E) log V) — several times slower.");
        puts("  Use it only when negative weights are genuinely possible");
        puts("  (currency arbitrage, some flow problems, distance-vector routing).");
    }

    puts("\n=== FLOYD-WARSHALL: ALL PAIRS ===");
    {
        int dist[MAX_V][MAX_V], next[MAX_V][MAX_V];
        int n = 5;

        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++) {
                dist[i][j] = (i == j) ? 0 : INF;
                next[i][j] = -1;
            }

        struct { int u, v, w; } es[] = {
            {0,1,3}, {0,3,7}, {1,0,8}, {1,2,2},
            {2,0,5}, {2,3,1}, {3,0,2}, {3,4,4}, {4,2,3},
        };
        for (size_t i = 0; i < sizeof es / sizeof es[0]; i++) {
            dist[es[i].u][es[i].v] = es[i].w;
            next[es[i].u][es[i].v] = es[i].v;
        }

        floyd_warshall(dist, next, n);

        printf("  all-pairs shortest distances:\n        ");
        for (int j = 0; j < n; j++) printf("%4c", 'A' + j);
        puts("");
        for (int i = 0; i < n; i++) {
            printf("      %c ", 'A' + i);
            for (int j = 0; j < n; j++) {
                if (dist[i][j] == INF) printf("   .");
                else                   printf("%4d", dist[i][j]);
            }
            puts("");
        }
        puts("");
        puts("  Three nested loops and it is done. The k loop MUST be OUTERMOST:");
        puts("      for k: for i: for j:");
        puts("          dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])");
        puts("  It means 'having now allowed k as an intermediate stop, what is");
        puts("  the best i->j route?'. Putting k inside gives plausible-looking");
        puts("  WRONG answers, and it is the classic Floyd-Warshall bug.");
        puts("");
        puts("  O(V^3) time and O(V^2) space regardless of edge count, so it is");
        puts("  only for small or dense graphs. For V=500 it is 125 million");
        puts("  operations — fine. For V=10000 it is a trillion — not fine.");
        puts("  It handles negative edges (but not negative cycles: a negative");
        puts("  value on the diagonal is exactly how you detect one).");
    }

    puts("\n=== PRIM'S MINIMUM SPANNING TREE ===");
    {
        Graph u;
        graph_init(&u, 7, false);                    /* UNDIRECTED */
        add_edge(&u, 0, 1, 7);  add_edge(&u, 0, 3, 5);
        add_edge(&u, 1, 2, 8);  add_edge(&u, 1, 3, 9);
        add_edge(&u, 1, 4, 7);  add_edge(&u, 2, 4, 5);
        add_edge(&u, 3, 4, 15); add_edge(&u, 3, 5, 6);
        add_edge(&u, 4, 5, 8);  add_edge(&u, 4, 6, 9);
        add_edge(&u, 5, 6, 11);

        int parent[MAX_V];
        int total = prim(&u, 0, parent);

        printf("  MST edges (%d vertices, so %d edges):\n", u.n, u.n - 1);
        for (int v = 0; v < u.n; v++)
            if (parent[v] != -1) printf("    %s - %s\n", u.name[parent[v]], u.name[v]);
        printf("  total weight: %d\n", total);
        puts("  (Kruskal on the same graph also gives 39 — see module 09's");
        puts("   union-find file. When weights tie, the two algorithms may pick");
        puts("   DIFFERENT edges, but the TOTAL is always identical.)");
        puts("");
        puts("  PRIM vs DIJKSTRA are nearly the same code. The ONE difference:");
        puts("    Dijkstra: key[v] = dist[u] + weight   (distance from the SOURCE)");
        puts("    Prim:     key[v] = weight             (distance from the TREE)");
        puts("  Dijkstra builds a shortest-path tree; Prim builds a minimum-");
        puts("  weight tree. One term in one line separates them.");
        puts("");
        puts("  PRIM vs KRUSKAL:");
        puts("    Prim    grows ONE tree; better for DENSE graphs; needs a heap");
        puts("    Kruskal sorts all edges and merges FORESTS; better for SPARSE");
        puts("            graphs; needs union-find");

        graph_free(&u);
    }

    puts("\n=== CHOOSING A SHORTEST-PATH ALGORITHM ===");
    puts("  unweighted                   BFS              O(V+E)");
    puts("  non-negative weights         Dijkstra         O((V+E) log V)");
    puts("  negative weights possible    Bellman-Ford     O(VE)");
    puts("  all pairs, small/dense V     Floyd-Warshall   O(V^3)");
    puts("  all pairs, sparse            Dijkstra from every vertex, or Johnson's");
    puts("  a good heuristic available   A* (Dijkstra + estimated remaining cost)");
    puts("  DAG only                     topological order + one relaxation pass, O(V+E)");
    puts("");
    puts("  A* deserves a mention: it is Dijkstra with the priority changed from");
    puts("  dist[v] to dist[v] + h(v), where h estimates the remaining distance.");
    puts("  If h never OVERESTIMATES (it is 'admissible'), A* is still optimal");
    puts("  but explores far fewer nodes. It is what every game pathfinder and");
    puts("  route planner actually runs.");

    graph_free(&g);
    return 0;
}
