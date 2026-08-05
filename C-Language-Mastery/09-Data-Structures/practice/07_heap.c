/* 07_heap.c — binary heap, priority queue, heapsort.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 07_heap.c -o t && ./t
 *
 * A binary heap is a COMPLETE binary tree stored in a FLAT ARRAY. There are
 * no pointers at all: the tree structure is implied by arithmetic on indices.
 *
 *   parent(i) = (i - 1) / 2
 *   left(i)   = 2i + 1
 *   right(i)  = 2i + 2
 *
 * THE HEAP PROPERTY (min-heap): every node <= both of its children.
 * That is much weaker than a BST's ordering — it only says the minimum is at
 * the root — and that weakness is exactly why insert and extract are O(log n)
 * with tiny constants and perfect cache behaviour.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* ================================================================= *
 * MIN-HEAP OF ints
 * ================================================================= */
typedef struct { int *data; size_t len, cap; } Heap;

static size_t swaps = 0;

static bool heap_init(Heap *h, size_t cap)
{
    h->data = malloc(cap * sizeof *h->data);
    if (h->data == NULL) return false;
    h->len = 0; h->cap = cap;
    return true;
}
static void heap_free(Heap *h) { free(h->data); h->data = NULL; h->len = h->cap = 0; }

static void swap(int *a, int *b) { int t = *a; *a = *b; *b = t; swaps++; }

/* SIFT UP: a new element at the end may be smaller than its parent, so
 * bubble it up until the heap property holds. O(log n) — at most the height. */
static void sift_up(Heap *h, size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (h->data[parent] <= h->data[i]) break;    /* property restored */
        swap(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

/* SIFT DOWN: the root may be larger than a child, so push it down along the
 * path of the SMALLER child (picking the larger one would break the property
 * on the other side). O(log n). */
static void sift_down(Heap *h, size_t i)
{
    for (;;) {
        size_t l = 2 * i + 1, r = 2 * i + 2, smallest = i;

        if (l < h->len && h->data[l] < h->data[smallest]) smallest = l;
        if (r < h->len && h->data[r] < h->data[smallest]) smallest = r;
        if (smallest == i) break;                    /* property restored */

        swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
}

static bool heap_push(Heap *h, int v)
{
    if (h->len == h->cap) {
        size_t cap = h->cap * 2;
        int *tmp = realloc(h->data, cap * sizeof *tmp);
        if (tmp == NULL) return false;
        h->data = tmp; h->cap = cap;
    }
    h->data[h->len] = v;         /* put it at the end (keeps the tree COMPLETE) */
    sift_up(h, h->len);          /* then restore the heap property */
    h->len++;
    return true;
}

static bool heap_peek(const Heap *h, int *out)
{
    if (h->len == 0) return false;
    if (out) *out = h->data[0];   /* the minimum is ALWAYS at index 0. O(1). */
    return true;
}

static bool heap_pop(Heap *h, int *out)
{
    if (h->len == 0) return false;
    if (out) *out = h->data[0];

    h->data[0] = h->data[--h->len];   /* move the LAST element to the root, */
    if (h->len > 0) sift_down(h, 0);  /* then push it down where it belongs */
    return true;
}

/* BUILD A HEAP FROM AN ARBITRARY ARRAY IN O(n), NOT O(n log n).
 *
 * Sifting down from the last internal node backwards is O(n), not O(n log n),
 * because most nodes are near the BOTTOM and have almost no distance to
 * travel. Half the nodes are leaves (0 work), a quarter are one level up
 * (<=1 swap), and so on: sum of (n/2^k * k) converges to 2n. */
static void heapify(Heap *h)
{
    if (h->len < 2) return;
    for (size_t i = h->len / 2; i-- > 0; ) sift_down(h, i);
}

static void heap_print(const Heap *h, const char *label)
{
    printf("  %-20s [", label);
    for (size_t i = 0; i < h->len; i++) printf("%d%s", h->data[i], i + 1 < h->len ? " " : "");
    puts("]");
}

/* Show the implied tree, so the index arithmetic becomes visible. */
static void heap_print_tree(const Heap *h)
{
    size_t level_start = 0, level_size = 1;
    while (level_start < h->len) {
        printf("    ");
        for (size_t i = level_start; i < level_start + level_size && i < h->len; i++)
            printf("%d ", h->data[i]);
        puts("");
        level_start += level_size;
        level_size *= 2;
    }
}

static bool is_min_heap(const Heap *h)
{
    for (size_t i = 0; i < h->len; i++) {
        size_t l = 2 * i + 1, r = 2 * i + 2;
        if (l < h->len && h->data[l] < h->data[i]) return false;
        if (r < h->len && h->data[r] < h->data[i]) return false;
    }
    return true;
}

/* ================================================================= *
 * HEAPSORT — in place, O(n log n) GUARANTEED, O(1) extra space.
 *
 * Build a MAX-heap, then repeatedly swap the root (the maximum) with the
 * last element and shrink the heap. The sorted part grows from the right.
 * ================================================================= */
static void max_sift_down(int *a, size_t n, size_t i)
{
    for (;;) {
        size_t l = 2 * i + 1, r = 2 * i + 2, largest = i;
        if (l < n && a[l] > a[largest]) largest = l;
        if (r < n && a[r] > a[largest]) largest = r;
        if (largest == i) break;
        int t = a[i]; a[i] = a[largest]; a[largest] = t;
        i = largest;
    }
}
static void heapsort(int *a, size_t n)
{
    for (size_t i = n / 2; i-- > 0; ) max_sift_down(a, n, i);      /* build: O(n) */
    for (size_t end = n; end-- > 1; ) {                            /* extract: O(n log n) */
        int t = a[0]; a[0] = a[end]; a[end] = t;                   /* max to the back */
        max_sift_down(a, end, 0);
    }
}

/* ================================================================= *
 * A PRIORITY QUEUE with a payload — the actual reason heaps exist.
 * ================================================================= */
typedef struct { int priority; char name[24]; } Task;
typedef struct { Task *data; size_t len, cap; } PQueue;

static bool pq_init(PQueue *q, size_t cap)
{
    q->data = malloc(cap * sizeof *q->data);
    if (q->data == NULL) return false;
    q->len = 0; q->cap = cap; return true;
}
static void pq_free(PQueue *q) { free(q->data); q->data = NULL; q->len = q->cap = 0; }

static bool pq_push(PQueue *q, int priority, const char *name)
{
    if (q->len == q->cap) {
        Task *tmp = realloc(q->data, q->cap * 2 * sizeof *tmp);
        if (tmp == NULL) return false;
        q->data = tmp; q->cap *= 2;
    }
    Task *t = &q->data[q->len];
    t->priority = priority;
    snprintf(t->name, sizeof t->name, "%s", name);

    size_t i = q->len++;
    while (i > 0) {                                  /* sift up */
        size_t p = (i - 1) / 2;
        if (q->data[p].priority <= q->data[i].priority) break;
        Task tmp = q->data[p]; q->data[p] = q->data[i]; q->data[i] = tmp;
        i = p;
    }
    return true;
}
static bool pq_pop(PQueue *q, Task *out)
{
    if (q->len == 0) return false;
    if (out) *out = q->data[0];
    q->data[0] = q->data[--q->len];

    size_t i = 0;
    for (;;) {                                       /* sift down */
        size_t l = 2*i+1, r = 2*i+2, small = i;
        if (l < q->len && q->data[l].priority < q->data[small].priority) small = l;
        if (r < q->len && q->data[r].priority < q->data[small].priority) small = r;
        if (small == i) break;
        Task tmp = q->data[i]; q->data[i] = q->data[small]; q->data[small] = tmp;
        i = small;
    }
    return true;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }
static int cmp_int(const void *a, const void *b)
{ int x = *(const int*)a, y = *(const int*)b; return (x > y) - (x < y); }

int main(void)
{
    puts("=== A HEAP IS A TREE STORED IN A FLAT ARRAY ===");
    puts("  No pointers. The structure is implied by index arithmetic:");
    puts("      parent(i) = (i-1)/2      left(i) = 2i+1      right(i) = 2i+2");
    puts("");
    puts("  array : [1, 3, 6, 5, 9, 8]");
    puts("  tree  :        1              index 0");
    puts("               /   \\");
    puts("              3     6           indices 1, 2");
    puts("             / \\   /");
    puts("            5   9 8             indices 3, 4, 5");
    puts("");
    puts("  Because it is a COMPLETE tree (every level full except possibly the");
    puts("  last, filled left to right), the array has NO GAPS. That is what");
    puts("  makes the index arithmetic work, and it is why a heap has perfect");
    puts("  cache locality where a pointer-based tree does not.\n");

    Heap h; heap_init(&h, 4);

    puts("=== PUSH: append, then SIFT UP ===");
    {
        int vals[] = {9, 4, 7, 1, 8, 3, 5};
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++) {
            heap_push(&h, vals[i]);
            char label[32];
            snprintf(label, sizeof label, "push %d", vals[i]);
            heap_print(&h, label);
        }
        puts("\n  the implied tree:");
        heap_print_tree(&h);
        printf("  heap property holds: %s\n", is_min_heap(&h) ? "yes" : "NO");
        puts("  Note the array is NOT sorted — a heap is much weaker than that.");
        puts("  It guarantees only that the minimum is at index 0.");
    }

    puts("\n=== PEEK IS O(1), POP IS O(log n) ===");
    {
        int v = 0;
        heap_peek(&h, &v);
        printf("  peek -> %d (index 0, no work at all)\n", v);

        printf("  popping everything: ");
        while (heap_pop(&h, &v)) printf("%d ", v);
        puts(" <- SORTED, because each pop yields the current minimum");
        puts("  pop moves the LAST element to the root and sifts it down.");
        puts("  Using the last element keeps the tree complete, which is what");
        puts("  keeps the array gap-free.");
    }

    puts("\n=== heapify: O(n), NOT O(n log n) ===");
    {
        const size_t N = 100000;
        Heap a, b;
        heap_init(&a, N); heap_init(&b, N);

        /* Reverse-sorted input is the WORST case for building by pushes into a
         * min-heap: every new element is smaller than everything already there,
         * so it must sift all the way up to the root. */
        int *vals = malloc(N * sizeof *vals);
        for (size_t i = 0; i < N; i++) vals[i] = (int)(N - i);

        swaps = 0;
        for (size_t i = 0; i < N; i++) heap_push(&a, vals[i]);
        size_t push_swaps = swaps;

        swaps = 0;
        memcpy(b.data, vals, N * sizeof *vals);
        b.len = N;
        heapify(&b);
        size_t heapify_swaps = swaps;

        printf("  building a heap from %zu reverse-sorted values:\n", N);
        printf("    n successive pushes : %7zu swaps   (O(n log n))\n", push_swaps);
        printf("    one heapify pass    : %7zu swaps   (O(n))\n", heapify_swaps);
        printf("    ratio               : %.1fx fewer\n",
               (double)push_swaps / (double)heapify_swaps);
        printf("    for reference, n = %zu and n*log2(n) = %.0f\n",
               N, (double)N * 16.6);
        printf("    both are valid heaps: %s / %s\n",
               is_min_heap(&a) ? "yes" : "NO", is_min_heap(&b) ? "yes" : "NO");
        puts("");
        puts("  WHY heapify IS O(n): it sifts DOWN from the last internal node");
        puts("  backwards. Half the nodes are LEAVES and do zero work; a quarter");
        puts("  are one level up and move at most one step; and so on. The sum");
        puts("      n/2 * 0 + n/4 * 1 + n/8 * 2 + ...  converges to n.");
        puts("  Building by repeated push sifts UP, where most nodes are near the");
        puts("  BOTTOM and must travel the FULL height — hence O(n log n).");
        puts("  The asymmetry is that sift-down's expensive cases are RARE");
        puts("  (few nodes near the top) while sift-up's are COMMON.");
        free(vals);
        heap_free(&a); heap_free(&b);
    }

    puts("\n=== PRIORITY QUEUE (the real use case) ===");
    {
        PQueue q; pq_init(&q, 8);
        pq_push(&q, 3, "write documentation");
        pq_push(&q, 1, "fix the production outage");
        pq_push(&q, 5, "refactor for elegance");
        pq_push(&q, 2, "review the pull request");
        pq_push(&q, 1, "restore the backup");
        pq_push(&q, 4, "answer email");

        puts("  tasks in priority order (1 = most urgent):");
        Task t;
        while (pq_pop(&q, &t)) printf("    [p%d] %s\n", t.priority, t.name);
        pq_free(&q);

        puts("");
        puts("  Note the two priority-1 tasks came out in an arbitrary order.");
        puts("  A binary heap is NOT STABLE. If insertion order must break ties,");
        puts("  make the priority a struct {int prio; long seq;} and compare both.");
        puts("");
        puts("  Priority queues drive: Dijkstra's shortest path and A* (module 10),");
        puts("  Huffman coding, OS schedulers, event simulation, and every");
        puts("  'top K' problem.");
    }

    puts("\n=== TOP-K: where a heap really wins ===");
    {
        const int N = 1000000, K = 10;
        int *data = malloc((size_t)N * sizeof *data);
        unsigned seed = 42;
        for (int i = 0; i < N; i++) { seed = seed * 1103515245u + 12345u; data[i] = (int)(seed >> 8); }

        clock_t t = clock();
        int *copy = malloc((size_t)N * sizeof *copy);
        memcpy(copy, data, (size_t)N * sizeof *copy);
        qsort(copy, (size_t)N, sizeof *copy, cmp_int);
        double t_sort = seconds_since(t);
        int sorted_top = copy[N - K];

        /* A MIN-heap of size K holds the K largest seen so far: if the new
         * value beats the smallest of the K, replace it. O(n log k). */
        t = clock();
        Heap topk; heap_init(&topk, (size_t)K + 1);
        for (int i = 0; i < N; i++) {
            if (topk.len < (size_t)K) heap_push(&topk, data[i]);
            else if (data[i] > topk.data[0]) {
                int discard;
                heap_pop(&topk, &discard);
                heap_push(&topk, data[i]);
            }
        }
        double t_heap = seconds_since(t);
        int heap_top = topk.data[0];

        printf("  finding the top %d of %d values:\n", K, N);
        printf("    full sort  : %.4f s  O(n log n), and it touches everything\n", t_sort);
        printf("    size-%d heap: %.4f s  O(n log k), %.1fx faster\n",
               K, t_heap, t_sort / t_heap);
        printf("    same answer: %s (%d vs %d)\n",
               sorted_top == heap_top ? "yes" : "NO", sorted_top, heap_top);
        printf("    memory: sort needs %zu MB, the heap needs %zu bytes\n",
               (size_t)N * sizeof(int) / (1024*1024), (size_t)K * sizeof(int));
        puts("  The heap version also STREAMS: it never needs the whole dataset");
        puts("  in memory, so it works on data larger than RAM.");

        free(data); free(copy); heap_free(&topk);
    }

    puts("\n=== HEAPSORT ===");
    {
        int a[] = {38, 27, 43, 3, 9, 82, 10, 1, 55, 20};
        size_t n = sizeof a / sizeof a[0];
        printf("  before: ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        heapsort(a, n);
        printf("\n  after : ");
        for (size_t i = 0; i < n; i++) printf("%d ", a[i]);
        puts("");
        puts("  Build a MAX-heap, then repeatedly swap the root with the last");
        puts("  element and shrink. The sorted portion grows from the right.");
        puts("");
        puts("  Heapsort vs quicksort:");
        puts("    + O(n log n) GUARANTEED — no adversarial worst case");
        puts("    + O(1) extra space, fully in place");
        puts("    - not stable");
        puts("    - ~2-3x slower than quicksort in practice: the sift-down path");
        puts("      jumps around the array, which the cache hates");
        puts("  This is why real libraries use INTROSORT: quicksort, falling back");
        puts("  to heapsort when the recursion gets too deep. Best of both.");
    }

    puts("\n=== SUMMARY ===");
    puts("  peek min   O(1)        it is at index 0");
    puts("  push       O(log n)    append, then sift up");
    puts("  pop min    O(log n)    move last to root, sift down");
    puts("  heapify    O(n)        one backward pass of sift-down");
    puts("  search     O(n)        a heap is NOT a search structure");
    puts("  space      O(n)        one flat array, zero pointer overhead");
    puts("");
    puts("  A heap gives you LESS ordering than a BST and, precisely because of");
    puts("  that, it is smaller, faster and simpler. Use it whenever the only");
    puts("  question you ask is 'what is the smallest/largest right now?'");

    heap_free(&h);
    return 0;
}
