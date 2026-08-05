/* 09_graph.c — graphs: representation, BFS, DFS, topological sort, cycles.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 09_graph.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * A graph is V vertices and E edges. Everything else — how you STORE it and
 * how you WALK it — follows from those two numbers and from whether the graph
 * is sparse (E ~ V) or dense (E ~ V^2).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_V 32

/* ================================================================= *
 * REPRESENTATION 1: ADJACENCY LIST
 * For each vertex, a list of its neighbours. O(V + E) space.
 * THE DEFAULT — real graphs are almost always sparse.
 * ================================================================= */
typedef struct Edge { int to; int weight; struct Edge *next; } Edge;

typedef struct {
    Edge  *adj[MAX_V];
    char   name[MAX_V][16];
    int    n_vertices;
    bool   directed;
} Graph;

static void graph_init(Graph *g, int n, bool directed)
{
    memset(g, 0, sizeof *g);
    g->n_vertices = n;
    g->directed = directed;
    for (int i = 0; i < n; i++) snprintf(g->name[i], sizeof g->name[i], "%c", 'A' + i);
}

static void graph_free(Graph *g)
{
    for (int i = 0; i < g->n_vertices; i++) {
        Edge *e = g->adj[i];
        while (e) { Edge *next = e->next; free(e); e = next; }
        g->adj[i] = NULL;
    }
}

static bool graph_add_edge(Graph *g, int from, int to, int weight)
{
    Edge *e = malloc(sizeof *e);
    if (e == NULL) return false;
    e->to = to; e->weight = weight;
    e->next = g->adj[from];              /* prepend: O(1) */
    g->adj[from] = e;

    if (!g->directed) {                  /* undirected = the edge both ways */
        Edge *back = malloc(sizeof *back);
        if (back == NULL) return false;
        back->to = from; back->weight = weight;
        back->next = g->adj[to];
        g->adj[to] = back;
    }
    return true;
}

static void graph_print(const Graph *g, const char *label)
{
    printf("  %s (%s, %d vertices):\n", label,
           g->directed ? "directed" : "undirected", g->n_vertices);
    for (int i = 0; i < g->n_vertices; i++) {
        printf("    %s ->", g->name[i]);
        for (const Edge *e = g->adj[i]; e; e = e->next)
            printf(" %s(%d)", g->name[e->to], e->weight);
        puts("");
    }
}

/* ================================================================= *
 * REPRESENTATION 2: ADJACENCY MATRIX
 * m[i][j] = weight, or 0 for no edge. O(V^2) space regardless of E.
 * Right only for DENSE graphs, or when you need O(1) "is there an edge?".
 * ================================================================= */
typedef struct { int m[MAX_V][MAX_V]; int n; } Matrix;

static void matrix_init(Matrix *g, int n) { memset(g, 0, sizeof *g); g->n = n; }
static void matrix_add_edge(Matrix *g, int a, int b, int w, bool directed)
{ g->m[a][b] = w; if (!directed) g->m[b][a] = w; }
static void matrix_print(const Matrix *g)
{
    printf("      ");
    for (int j = 0; j < g->n; j++) printf("%c ", 'A' + j);
    puts("");
    for (int i = 0; i < g->n; i++) {
        printf("    %c ", 'A' + i);
        for (int j = 0; j < g->n; j++) printf("%d ", g->m[i][j] ? 1 : 0);
        puts("");
    }
}

/* ================================================================= *
 * BFS — breadth first, using a QUEUE.
 *
 * Visits vertices in order of DISTANCE from the source, so on an
 * UNWEIGHTED graph it finds the SHORTEST PATH. That is its whole point.
 * O(V + E).
 * ================================================================= */
static void bfs(const Graph *g, int start, int *dist, int *parent)
{
    bool visited[MAX_V] = {false};
    int  queue[MAX_V], head = 0, tail = 0;

    for (int i = 0; i < g->n_vertices; i++) { dist[i] = -1; parent[i] = -1; }

    visited[start] = true;
    dist[start] = 0;
    queue[tail++] = start;

    printf("    visit order: ");
    while (head < tail) {
        int v = queue[head++];                     /* dequeue */
        printf("%s ", g->name[v]);

        for (const Edge *e = g->adj[v]; e; e = e->next) {
            if (visited[e->to]) continue;
            visited[e->to] = true;                 /* mark ON ENQUEUE, not on */
            dist[e->to]   = dist[v] + 1;           /* dequeue — otherwise a */
            parent[e->to] = v;                     /* vertex can be queued twice */
            queue[tail++] = e->to;
        }
    }
    puts("");
}

/* ================================================================= *
 * DFS — depth first, using the CALL STACK (or an explicit one).
 * Goes as deep as possible before backtracking. O(V + E).
 * ================================================================= */
static void dfs_rec(const Graph *g, int v, bool *visited, int depth)
{
    visited[v] = true;
    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s\n", g->name[v]);
    for (const Edge *e = g->adj[v]; e; e = e->next)
        if (!visited[e->to]) dfs_rec(g, e->to, visited, depth + 1);
}

/* The iterative form: an explicit stack instead of the call stack. Use this
 * when the graph could be deep enough to overflow the real stack. */
static void dfs_iterative(const Graph *g, int start)
{
    bool visited[MAX_V] = {false};
    int  stack[MAX_V * MAX_V], top = 0;

    stack[top++] = start;
    printf("    visit order: ");
    while (top > 0) {
        int v = stack[--top];                      /* POP — the only difference */
        if (visited[v]) continue;                  /* from BFS is stack vs queue */
        visited[v] = true;
        printf("%s ", g->name[v]);
        for (const Edge *e = g->adj[v]; e; e = e->next)
            if (!visited[e->to]) stack[top++] = e->to;
    }
    puts("");
}

/* ================================================================= *
 * TOPOLOGICAL SORT (Kahn's algorithm)
 *
 * Order the vertices so every edge points forward. Only possible on a DAG:
 * if a cycle exists, some vertices never reach in-degree 0, and that is
 * exactly how this doubles as a cycle detector.
 * ================================================================= */
static bool topological_sort(const Graph *g, int *order)
{
    int in_degree[MAX_V] = {0};

    for (int v = 0; v < g->n_vertices; v++)
        for (const Edge *e = g->adj[v]; e; e = e->next) in_degree[e->to]++;

    int queue[MAX_V], head = 0, tail = 0;
    for (int v = 0; v < g->n_vertices; v++)
        if (in_degree[v] == 0) queue[tail++] = v;   /* no prerequisites */

    int count = 0;
    while (head < tail) {
        int v = queue[head++];
        order[count++] = v;
        for (const Edge *e = g->adj[v]; e; e = e->next)
            if (--in_degree[e->to] == 0) queue[tail++] = e->to;
    }
    return count == g->n_vertices;         /* false => a cycle exists */
}

/* ================================================================= *
 * CYCLE DETECTION with three colours (the DFS way).
 *   WHITE  not visited
 *   GREY   on the current recursion stack
 *   BLACK  fully explored
 * An edge to a GREY vertex is a BACK EDGE, which means a cycle.
 * ================================================================= */
typedef enum { WHITE, GREY, BLACK } Colour;

static bool has_cycle_rec(const Graph *g, int v, Colour *colour, int *cycle_at)
{
    colour[v] = GREY;
    for (const Edge *e = g->adj[v]; e; e = e->next) {
        if (colour[e->to] == GREY) { *cycle_at = e->to; return true; }   /* back edge */
        if (colour[e->to] == WHITE && has_cycle_rec(g, e->to, colour, cycle_at))
            return true;
    }
    colour[v] = BLACK;                     /* done with this whole subtree */
    return false;
}
static bool has_cycle(const Graph *g, int *cycle_at)
{
    Colour colour[MAX_V] = {WHITE};
    for (int v = 0; v < g->n_vertices; v++)
        if (colour[v] == WHITE && has_cycle_rec(g, v, colour, cycle_at)) return true;
    return false;
}

/* Connected components, via repeated DFS from every unvisited vertex. */
static void mark_component(const Graph *g, int v, int *component, int id)
{
    component[v] = id;
    for (const Edge *e = g->adj[v]; e; e = e->next)
        if (component[e->to] == -1) mark_component(g, e->to, component, id);
}
static int count_components(const Graph *g, int *component)
{
    for (int i = 0; i < g->n_vertices; i++) component[i] = -1;
    int n = 0;
    for (int v = 0; v < g->n_vertices; v++)
        if (component[v] == -1) mark_component(g, v, component, n++);
    return n;
}

/* Walk the parent chain BACK to the source, printing on the way out of the
 * recursion — which is what puts the path in forward order. */
static void print_path(const Graph *g, const int *parent, int target)
{
    if (parent[target] != -1) {
        print_path(g, parent, parent[target]);
        printf(" -> ");
    }
    printf("%s", g->name[target]);
}

int main(void)
{
    puts("=== TWO REPRESENTATIONS ===");
    {
        /* An undirected graph:  A-B, A-C, B-D, C-D, D-E */
        Graph g;
        graph_init(&g, 6, false);
        graph_add_edge(&g, 0, 1, 1);
        graph_add_edge(&g, 0, 2, 1);
        graph_add_edge(&g, 1, 3, 1);
        graph_add_edge(&g, 2, 3, 1);
        graph_add_edge(&g, 3, 4, 1);
        /* F (vertex 5) is deliberately left ISOLATED */

        graph_print(&g, "adjacency list");

        Matrix m;
        matrix_init(&m, 6);
        matrix_add_edge(&m, 0, 1, 1, false); matrix_add_edge(&m, 0, 2, 1, false);
        matrix_add_edge(&m, 1, 3, 1, false); matrix_add_edge(&m, 2, 3, 1, false);
        matrix_add_edge(&m, 3, 4, 1, false);
        puts("\n  adjacency matrix:");
        matrix_print(&m);

        printf("\n  space: list %zu bytes for %d edges, matrix %zu bytes ALWAYS\n",
               10 * sizeof(Edge), 5, (size_t)(6 * 6 * sizeof(int)));
        puts("");
        puts("                    ADJACENCY LIST     ADJACENCY MATRIX");
        puts("    space           O(V + E)           O(V^2)");
        puts("    add edge        O(1)               O(1)");
        puts("    has edge(u,v)?  O(degree(u))       O(1)");
        puts("    iterate u's     O(degree(u))       O(V) — must scan the row");
        puts("      neighbours");
        puts("    remove edge     O(degree(u))       O(1)");
        puts("");
        puts("  Real graphs are SPARSE: a road network, a social graph, a web");
        puts("  crawl all have E ~ V, not V^2. For a million vertices, a matrix");
        puts("  needs 4 TB and a list needs a few megabytes. USE THE LIST unless");
        puts("  the graph is genuinely dense or you need O(1) edge tests.");

        puts("\n=== BFS: shortest path on an UNWEIGHTED graph ===");
        int dist[MAX_V], parent[MAX_V];
        bfs(&g, 0, dist, parent);
        puts("    distances from A:");
        for (int i = 0; i < g.n_vertices; i++) {
            if (dist[i] < 0) { printf("      %s: unreachable\n", g.name[i]); continue; }
            printf("      %s: %d hop(s), path ", g.name[i], dist[i]);
            print_path(&g, parent, i);
            puts("");
        }
        puts("    BFS explores in order of DISTANCE, so the first time it reaches");
        puts("    a vertex it has found the SHORTEST path there. That is only true");
        puts("    for unweighted graphs — with weights you need Dijkstra (module 10).");
        puts("    Mark a vertex visited when you ENQUEUE it, not when you dequeue,");
        puts("    or it can enter the queue several times.");

        puts("\n=== DFS: go deep, then backtrack ===");
        {
            bool visited[MAX_V] = {false};
            puts("    recursive (indentation shows depth):");
            dfs_rec(&g, 0, visited, 2);
            puts("    iterative, with an explicit stack:");
            dfs_iterative(&g, 0);
            puts("    BFS and DFS are THE SAME ALGORITHM with a different container:");
            puts("    a QUEUE gives you breadth-first, a STACK gives you depth-first.");
            puts("    Swap the container and nothing else changes.");
        }

        puts("\n=== CONNECTED COMPONENTS ===");
        {
            int component[MAX_V];
            int n = count_components(&g, component);
            printf("    %d component(s):\n", n);
            for (int c = 0; c < n; c++) {
                printf("      component %d: ", c);
                for (int v = 0; v < g.n_vertices; v++)
                    if (component[v] == c) printf("%s ", g.name[v]);
                puts("");
            }
            puts("    F is isolated, so it forms its own component. Repeated DFS");
            puts("    from every unvisited vertex is the whole algorithm, O(V+E).");
        }

        graph_free(&g);
    }

    puts("\n=== DIRECTED ACYCLIC GRAPHS AND TOPOLOGICAL SORT ===");
    {
        /* A build-dependency graph: an edge X -> Y means "X must come before Y". */
        Graph dag;
        graph_init(&dag, 6, true);
        const char *names[] = {"config", "compile", "test", "docs", "package", "deploy"};
        for (int i = 0; i < 6; i++)
            snprintf(dag.name[i], sizeof dag.name[i], "%s", names[i]);

        graph_add_edge(&dag, 0, 1, 0);   /* config  -> compile */
        graph_add_edge(&dag, 1, 2, 0);   /* compile -> test    */
        graph_add_edge(&dag, 1, 4, 0);   /* compile -> package */
        graph_add_edge(&dag, 2, 5, 0);   /* test    -> deploy  */
        graph_add_edge(&dag, 4, 5, 0);   /* package -> deploy  */
        graph_add_edge(&dag, 0, 3, 0);   /* config  -> docs    */

        graph_print(&dag, "build dependencies");

        int order[MAX_V];
        if (topological_sort(&dag, order)) {
            printf("\n    a valid build order: ");
            for (int i = 0; i < dag.n_vertices; i++) printf("%s ", dag.name[order[i]]);
            puts("");
            puts("    Every edge points FORWARD in this list, so each step's");
            puts("    prerequisites are already done. (The order is not unique —");
            puts("    docs and compile are independent, so either may come first.)");
        }

        int cycle_at = -1;
        printf("    contains a cycle: %s\n", has_cycle(&dag, &cycle_at) ? "yes" : "no");

        /* Now introduce a cycle and watch both detectors fire. */
        puts("\n    adding deploy -> config (a circular dependency):");
        graph_add_edge(&dag, 5, 0, 0);
        printf("      topological_sort -> %s\n",
               topological_sort(&dag, order) ? "succeeded?!" : "FAILED (cycle detected)");
        printf("      has_cycle        -> %s",
               has_cycle(&dag, &cycle_at) ? "cycle found" : "no cycle");
        if (cycle_at >= 0) printf(" at \"%s\"", dag.name[cycle_at]);
        puts("");
        puts("      Kahn's algorithm detects it for free: with a cycle, those");
        puts("      vertices never reach in-degree 0, so fewer than V vertices");
        puts("      come out. That is the same check `make` performs when it says");
        puts("      \"Circular dependency dropped\".");

        graph_free(&dag);
    }

    puts("\n=== THREE-COLOUR CYCLE DETECTION ===");
    puts("    WHITE = not visited");
    puts("    GREY  = on the CURRENT recursion stack");
    puts("    BLACK = fully explored, done");
    puts("  An edge to a GREY vertex is a BACK EDGE — you have looped round to");
    puts("  something you are still in the middle of. That is a cycle.");
    puts("  An edge to a BLACK vertex is fine: it is a different, finished branch.");
    puts("  Using only a `visited` flag would wrongly report a cycle for any");
    puts("  diamond-shaped DAG. The distinction between GREY and BLACK is the");
    puts("  whole algorithm.");

    puts("\n=== WHERE THESE SHOW UP ===");
    puts("  BFS       shortest path (unweighted), web crawling, network broadcast,");
    puts("            'friends within N hops', garbage-collection mark phase");
    puts("  DFS       cycle detection, topological sort, connected components,");
    puts("            maze solving, backtracking, strongly connected components");
    puts("  TOPO SORT build systems (make, ninja), package managers, spreadsheet");
    puts("            recalculation, course prerequisites, task scheduling");
    puts("  Module 10 continues with Dijkstra, Bellman-Ford, and minimum");
    puts("  spanning trees.");

    return 0;
}
