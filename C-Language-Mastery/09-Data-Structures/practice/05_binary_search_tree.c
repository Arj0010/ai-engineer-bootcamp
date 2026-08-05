/* 05_binary_search_tree.c — the BST, all three delete cases, and its failure mode.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 05_binary_search_tree.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * THE INVARIANT: for every node, everything in the left subtree is smaller
 * and everything in the right subtree is larger. That single rule gives you
 * O(h) search — and the whole difficulty is that h is only log n if the tree
 * is BALANCED, which a plain BST does not guarantee. File 06 fixes that.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct Node {
    int value;
    struct Node *left, *right;
} Node;

typedef struct { Node *root; size_t count; } BST;

static size_t comparisons = 0;      /* instrumentation */

static void bst_init(BST *t) { t->root = NULL; t->count = 0; }

/* ---------------- insert ---------------- */

/* The recursive form: clean, and returns the (possibly new) subtree root.
 * That "return the new root" convention removes every parent-pointer fixup,
 * and it is what makes the AVL rotations in file 06 tractable. */
static Node *insert_rec(Node *n, int v, bool *inserted)
{
    if (n == NULL) {
        Node *fresh = malloc(sizeof *fresh);
        if (fresh == NULL) return NULL;
        fresh->value = v;
        fresh->left = fresh->right = NULL;
        *inserted = true;
        return fresh;
    }
    comparisons++;
    if      (v < n->value) n->left  = insert_rec(n->left,  v, inserted);
    else if (v > n->value) n->right = insert_rec(n->right, v, inserted);
    /* equal: do nothing — this is a SET, duplicates are ignored */
    return n;
}
static bool bst_insert(BST *t, int v)
{
    bool inserted = false;
    t->root = insert_rec(t->root, v, &inserted);
    if (inserted) t->count++;
    return inserted;
}

/* ---------------- search ---------------- */

/* Iterative: no stack frames, and it shows the shape of the algorithm plainly.
 * O(h) — h is the HEIGHT, not the count. */
static bool bst_contains(const BST *t, int v, size_t *steps)
{
    size_t n_steps = 0;
    for (const Node *n = t->root; n != NULL; ) {
        n_steps++;
        if      (v < n->value) n = n->left;      /* discard the whole right side */
        else if (v > n->value) n = n->right;
        else { if (steps) *steps = n_steps; return true; }
    }
    if (steps) *steps = n_steps;
    return false;
}

static Node *find_min(Node *n) { while (n && n->left)  n = n->left;  return n; }
static Node *find_max(Node *n) { while (n && n->right) n = n->right; return n; }

/* ---------------- delete: the interesting one ---------------- */

/* THREE CASES:
 *   0 children — just free it and return NULL to the parent
 *   1 child    — splice: return the child, the parent adopts it
 *   2 children — the hard one. Replace the value with its IN-ORDER
 *                SUCCESSOR (the smallest value in the right subtree),
 *                then delete that successor from the right subtree.
 *                The successor has at most one child by construction, so
 *                the recursion terminates in one more step.
 */
static Node *delete_rec(Node *n, int v, bool *removed)
{
    if (n == NULL) return NULL;

    if      (v < n->value) { n->left  = delete_rec(n->left,  v, removed); return n; }
    else if (v > n->value) { n->right = delete_rec(n->right, v, removed); return n; }

    /* found it */
    *removed = true;

    if (n->left == NULL) {                 /* 0 or 1 (right) child */
        Node *child = n->right;
        free(n);
        return child;                      /* NULL if it was a leaf */
    }
    if (n->right == NULL) {                /* exactly 1 (left) child */
        Node *child = n->left;
        free(n);
        return child;
    }

    /* TWO CHILDREN */
    Node *successor = find_min(n->right);  /* smallest value greater than n */
    n->value = successor->value;           /* copy the VALUE up */
    bool dummy = false;
    n->right = delete_rec(n->right, successor->value, &dummy);  /* remove the old node */
    return n;
}
static bool bst_delete(BST *t, int v)
{
    bool removed = false;
    t->root = delete_rec(t->root, v, &removed);
    if (removed) t->count--;
    return removed;
}

/* ---------------- traversals ---------------- */

static void inorder(const Node *n, void (*visit)(int, void *), void *ctx)
{
    if (n == NULL) return;
    inorder(n->left, visit, ctx);          /* left  */
    visit(n->value, ctx);                  /* self  -> SORTED ORDER */
    inorder(n->right, visit, ctx);         /* right */
}
static void preorder(const Node *n, void (*visit)(int, void *), void *ctx)
{
    if (n == NULL) return;
    visit(n->value, ctx);                  /* self first -> good for COPYING a tree */
    preorder(n->left, visit, ctx);
    preorder(n->right, visit, ctx);
}
static void postorder(const Node *n, void (*visit)(int, void *), void *ctx)
{
    if (n == NULL) return;
    postorder(n->left, visit, ctx);
    postorder(n->right, visit, ctx);
    visit(n->value, ctx);                  /* self LAST -> the only safe order to FREE */
}
static void print_int(int v, void *ctx) { (void)ctx; printf("%d ", v); }

/* ---------------- measurements ---------------- */

static size_t height(const Node *n)
{
    if (n == NULL) return 0;
    size_t l = height(n->left), r = height(n->right);
    return 1 + (l > r ? l : r);
}
static size_t count_nodes(const Node *n)
{
    return n ? 1 + count_nodes(n->left) + count_nodes(n->right) : 0;
}

/* Verify the BST invariant holds — the property, not just the output. */
static bool is_bst(const Node *n, long lo, long hi)
{
    if (n == NULL) return true;
    if (n->value <= lo || n->value >= hi) return false;
    return is_bst(n->left, lo, n->value) && is_bst(n->right, n->value, hi);
}

static void free_tree(Node *n)
{
    if (n == NULL) return;
    free_tree(n->left);                    /* POST-ORDER: children before parent. */
    free_tree(n->right);                   /* Pre-order would free the node and */
    free(n);                               /* then read n->left out of freed memory. */
}

/* An ASCII picture, rotated 90 degrees: root at the left, deeper to the right. */
static void print_tree(const Node *n, int depth)
{
    if (n == NULL) return;
    print_tree(n->right, depth + 1);
    for (int i = 0; i < depth; i++) printf("      ");
    printf("%d\n", n->value);
    print_tree(n->left, depth + 1);
}

int main(void)
{
    puts("=== THE BST INVARIANT ===");
    puts("  For EVERY node: left subtree < node < right subtree.");
    puts("  Search then discards half the remaining tree at each step, which");
    puts("  is O(h). The catch is that h is only log(n) if the tree is BALANCED.\n");

    BST t; bst_init(&t);

    puts("=== BUILDING FROM RANDOM-ISH INPUT ===");
    {
        int values[] = {50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
        for (size_t i = 0; i < sizeof values / sizeof values[0]; i++) bst_insert(&t, values[i]);

        printf("  inserted %zu values\n", t.count);
        printf("  height %zu (ideal for %zu nodes is %d)\n",
               height(t.root), t.count, 4);
        puts("\n  the tree (rotated 90 degrees, root at the left):");
        print_tree(t.root, 1);

        printf("\n  invariant holds: %s\n",
               is_bst(t.root, -1000000, 1000000) ? "yes" : "NO");
    }

    puts("\n=== TRAVERSALS ===");
    {
        printf("  in-order   (L,self,R): "); inorder(t.root, print_int, NULL);
        puts("\n               ^ SORTED. This is the defining property: an");
        puts("                 in-order walk of a BST yields sorted output.");
        printf("  pre-order  (self,L,R): "); preorder(t.root, print_int, NULL);
        puts("\n               ^ root first — use this to COPY or SERIALISE a tree,");
        puts("                 because reinserting in this order rebuilds the shape.");
        printf("  post-order (L,R,self): "); postorder(t.root, print_int, NULL);
        puts("\n               ^ children before parent — the ONLY safe order to");
        puts("                 FREE a tree. Freeing pre-order reads n->left out");
        puts("                 of a block you already released.");
    }

    puts("\n=== SEARCH ===");
    {
        int probes[] = {50, 35, 65, 99, 20};
        for (size_t i = 0; i < 5; i++) {
            size_t steps;
            bool found = bst_contains(&t, probes[i], &steps);
            printf("  contains(%2d) -> %-9s in %zu comparison(s)\n",
                   probes[i], found ? "found" : "not found", steps);
        }
        printf("  min = %d, max = %d\n", find_min(t.root)->value, find_max(t.root)->value);
    }

    puts("\n=== DELETION: THREE CASES ===");
    {
        puts("  CASE 1 — a LEAF (no children): just free it.");
        printf("    delete(35): ");
        bst_delete(&t, 35);
        inorder(t.root, print_int, NULL);
        puts("");

        puts("\n  CASE 2 — ONE child: splice it out, the parent adopts the child.");
        bst_insert(&t, 25);          /* 20 now has a right child, 25 */
        printf("    (inserted 25 under 20)  delete(20): ");
        bst_delete(&t, 20);
        inorder(t.root, print_int, NULL);
        puts("");

        puts("\n  CASE 3 — TWO children: the interesting one.");
        puts("    You cannot just remove the node — two subtrees would be orphaned.");
        puts("    Instead: find the IN-ORDER SUCCESSOR (the smallest value in the");
        puts("    right subtree), copy its VALUE into this node, then delete THAT");
        puts("    node from the right subtree. The successor has at most one child");
        puts("    by construction, so it reduces to case 1 or 2 immediately.");
        printf("    before delete(30): ");
        inorder(t.root, print_int, NULL);
        puts("");
        Node *node30 = t.root->left;
        printf("    node 30 has children %d and %d; its successor is %d\n",
               node30->left ? node30->left->value : -1,
               node30->right ? node30->right->value : -1,
               find_min(node30->right)->value);
        bst_delete(&t, 30);
        printf("    after  delete(30): ");
        inorder(t.root, print_int, NULL);
        puts("");
        printf("    still sorted, and the invariant holds: %s\n",
               is_bst(t.root, -1000000, 1000000) ? "yes" : "NO");
        printf("    count = %zu, actual nodes = %zu (must match)\n",
               t.count, count_nodes(t.root));

        printf("\n    delete a value that is not present: %s\n",
               bst_delete(&t, 999) ? "removed?!" : "correctly refused");
    }

    puts("\n=== THE FAILURE MODE: SORTED INPUT ===");
    {
        BST balanced, degenerate;
        bst_init(&balanced); bst_init(&degenerate);

        const int N = 1000;

        /* Insert in an order that keeps it roughly balanced: bisect the range. */
        int *order = malloc((size_t)N * sizeof *order);
        for (int i = 0; i < N; i++) order[i] = i;
        /* A simple shuffle with a fixed seed, so the result is reproducible. */
        unsigned seed = 12345;
        for (int i = N - 1; i > 0; i--) {
            seed = seed * 1103515245u + 12345u;
            int j = (int)((seed >> 16) % (unsigned)(i + 1));
            int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
        }
        for (int i = 0; i < N; i++) bst_insert(&balanced, order[i]);
        free(order);

        /* Insert in SORTED order: every value is larger than the last, so it
         * always goes right. The tree becomes a linked list. */
        for (int i = 0; i < N; i++) bst_insert(&degenerate, i);

        printf("  %d nodes: a PERFECTLY balanced tree would be height %d,\n", N, 10);
        puts ("  and a randomly built one averages about 3*ln(n) ~ 21.");
        printf("    shuffled insertion : height %3zu   <- O(log n) territory\n",
               height(balanced.root));
        printf("    SORTED insertion   : height %3zu   <- a LINKED LIST\n",
               height(degenerate.root));

        size_t s1, s2;
        bst_contains(&balanced,   N - 1, &s1);
        bst_contains(&degenerate, N - 1, &s2);
        printf("  searching for the last value:\n");
        printf("    shuffled : %3zu comparisons\n", s1);
        printf("    sorted   : %3zu comparisons  (%.0fx worse)\n",
               s2, (double)s2 / (double)s1);

        puts("");
        puts("  THIS IS THE PROBLEM WITH A PLAIN BST. Sorted or nearly-sorted");
        puts("  input — which is extremely common in practice: timestamps, IDs,");
        puts("  anything from a sorted file — degenerates it into a linked list.");
        puts("  Every operation becomes O(n) and you have paid for pointers and");
        puts("  cache misses to get worse-than-array behaviour.");
        puts("");
        puts("  THE FIXES:");
        puts("    - AVL tree      strict balance via rotations (file 06)");
        puts("    - red-black     looser balance, fewer rotations (Linux, std::map)");
        puts("    - B-tree        wide nodes, built for disk and cache (databases)");
        puts("    - treap/skiplist  randomised balance, much simpler to implement");
        puts("    - or just use a HASH TABLE if you do not need ordering");

        free_tree(balanced.root);
        free_tree(degenerate.root);
    }

    puts("\n=== WHAT A BST GIVES YOU THAT A HASH TABLE DOES NOT ===");
    puts("  - ORDERED iteration (in-order traversal is sorted, for free)");
    puts("  - min / max in O(h)");
    puts("  - RANGE QUERIES: every key between x and y");
    puts("  - predecessor / successor of a key");
    puts("  - no hash function needed, no resize pauses, no worst-case collisions");
    puts("");
    puts("  A hash table is faster for pure lookup. Use a tree when ORDER matters.");

    free_tree(t.root);
    printf("\n  (tree freed post-order; %zu comparisons performed overall)\n", comparisons);
    return 0;
}
