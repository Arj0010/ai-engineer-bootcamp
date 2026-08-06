/* 06_avl_tree.c — the self-balancing BST. Rotations, and why they work.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 06_avl_tree.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * THE AVL INVARIANT: for every node, |height(left) - height(right)| <= 1.
 *
 * Maintaining that guarantees height <= 1.44 * log2(n), so every operation is
 * O(log n) in the WORST case — including the sorted-input case that turns a
 * plain BST into a linked list (see file 05).
 *
 * The whole mechanism is four cases and two primitive rotations.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Node {
    int value;
    int height;                  /* CACHED. Recomputing it would make every
                                  * insert O(n); storing it makes it O(1). */
    struct Node *left, *right;
} Node;

static size_t rotations = 0;

static int node_height(const Node *n) { return n ? n->height : 0; }
static int max_i(int a, int b) { return a > b ? a : b; }
static void update_height(Node *n) { n->height = 1 + max_i(node_height(n->left), node_height(n->right)); }

/* BALANCE FACTOR = height(left) - height(right).
 *    0, +1, -1  ->  balanced
 *    +2         ->  LEFT-heavy, needs a right rotation
 *    -2         ->  RIGHT-heavy, needs a left rotation
 * It can never exceed +/-2 if we rebalance after every single insertion. */
static int balance_factor(const Node *n) { return n ? node_height(n->left) - node_height(n->right) : 0; }

/* ================================================================= *
 * THE TWO PRIMITIVE ROTATIONS
 *
 * A rotation rewires three pointers and preserves the BST invariant
 * exactly. It is a local, O(1) operation.
 *
 *      RIGHT ROTATION about y            LEFT ROTATION about x
 *
 *          y                x                 x                y
 *         / \              / \               / \              / \
 *        x   C    ==>     A   y             A   y     ==>    x   C
 *       / \                  / \               / \          / \
 *      A   B                B   C             B   C        A   B
 *
 * In both pictures the in-order sequence is A x B y C. That is WHY the
 * BST property survives: a rotation changes the SHAPE, never the ORDER.
 * ================================================================= */
static Node *rotate_right(Node *y)
{
    Node *x = y->left;
    Node *B = x->right;

    x->right = y;               /* x becomes the new subtree root */
    y->left  = B;               /* y adopts x's old right subtree  */

    update_height(y);           /* y first — it is now the CHILD */
    update_height(x);
    rotations++;
    return x;                   /* the caller links this in as the new root */
}

static Node *rotate_left(Node *x)
{
    Node *y = x->right;
    Node *B = y->left;

    y->left  = x;
    x->right = B;

    update_height(x);           /* x first — it is now the child */
    update_height(y);
    rotations++;
    return y;
}

/* ================================================================= *
 * REBALANCE — the four cases.
 * ================================================================= */
static Node *rebalance(Node *n)
{
    update_height(n);
    int bf = balance_factor(n);

    /* LEFT-LEFT: left-heavy, and the left child is itself left-heavy.
     *      z              y
     *     /              / \
     *    y      ==>     x   z          ONE right rotation
     *   /
     *  x
     */
    if (bf > 1 && balance_factor(n->left) >= 0)
        return rotate_right(n);

    /* LEFT-RIGHT: left-heavy, but the left child leans RIGHT.
     *    z          z            x
     *   /          /            / \
     *  x    ==>   y     ==>    y   z    left on the child, THEN right
     *   \        /
     *    y      x
     */
    if (bf > 1) {
        n->left = rotate_left(n->left);
        return rotate_right(n);
    }

    /* RIGHT-RIGHT: mirror of left-left. ONE left rotation. */
    if (bf < -1 && balance_factor(n->right) <= 0)
        return rotate_left(n);

    /* RIGHT-LEFT: mirror of left-right. Right on the child, THEN left. */
    if (bf < -1) {
        n->right = rotate_right(n->right);
        return rotate_left(n);
    }

    return n;                   /* already balanced */
}

/* ================================================================= *
 * INSERT — an ordinary BST insert, then rebalance on the way back up.
 *
 * The recursion is what makes this work: as each call returns, it hands
 * its (possibly rotated) subtree root back to its parent, so the fix
 * propagates upward with no parent pointers anywhere.
 * ================================================================= */
static Node *avl_insert(Node *n, int v, bool *inserted)
{
    if (n == NULL) {
        Node *fresh = malloc(sizeof *fresh);
        if (fresh == NULL) return NULL;
        fresh->value = v; fresh->height = 1;
        fresh->left = fresh->right = NULL;
        *inserted = true;
        return fresh;
    }
    if      (v < n->value) n->left  = avl_insert(n->left,  v, inserted);
    else if (v > n->value) n->right = avl_insert(n->right, v, inserted);
    else return n;                                  /* duplicate: ignore */

    return rebalance(n);                            /* <- the only added line */
}

static Node *find_min(Node *n) { while (n->left) n = n->left; return n; }

static Node *avl_delete(Node *n, int v, bool *removed)
{
    if (n == NULL) return NULL;

    if      (v < n->value) n->left  = avl_delete(n->left,  v, removed);
    else if (v > n->value) n->right = avl_delete(n->right, v, removed);
    else {
        *removed = true;
        if (n->left == NULL || n->right == NULL) {
            Node *child = n->left ? n->left : n->right;
            free(n);
            return child;                            /* may be NULL */
        }
        Node *succ = find_min(n->right);
        n->value = succ->value;
        bool dummy = false;
        n->right = avl_delete(n->right, succ->value, &dummy);
    }
    return rebalance(n);                             /* same one line */
}

static bool avl_contains(const Node *n, int v, size_t *steps)
{
    size_t s = 0;
    while (n != NULL) {
        s++;
        if      (v < n->value) n = n->left;
        else if (v > n->value) n = n->right;
        else { if (steps) *steps = s; return true; }
    }
    if (steps) *steps = s;
    return false;
}

static void free_tree(Node *n) { if (n) { free_tree(n->left); free_tree(n->right); free(n); } }
static void inorder(const Node *n) { if (n) { inorder(n->left); printf("%d ", n->value); inorder(n->right); } }
static int  count_nodes(const Node *n) { return n ? 1 + count_nodes(n->left) + count_nodes(n->right) : 0; }

/* Verify BOTH invariants: the BST ordering AND the AVL balance condition. */
static bool is_avl(const Node *n, long lo, long hi)
{
    if (n == NULL) return true;
    if (n->value <= lo || n->value >= hi) return false;
    int bf = balance_factor(n);
    if (bf < -1 || bf > 1) return false;
    if (n->height != 1 + max_i(node_height(n->left), node_height(n->right))) return false;
    return is_avl(n->left, lo, n->value) && is_avl(n->right, n->value, hi);
}

static void print_tree(const Node *n, int depth)
{
    if (n == NULL) return;
    print_tree(n->right, depth + 1);
    for (int i = 0; i < depth; i++) printf("       ");
    printf("%d(h%d,b%+d)\n", n->value, n->height, balance_factor(n));
    print_tree(n->left, depth + 1);
}

/* A plain BST, for the side-by-side comparison. */
typedef struct BNode { int value; struct BNode *l, *r; } BNode;
static BNode *bst_insert(BNode *n, int v)
{
    if (n == NULL) { BNode *f = malloc(sizeof *f); f->value = v; f->l = f->r = NULL; return f; }
    if      (v < n->value) n->l = bst_insert(n->l, v);
    else if (v > n->value) n->r = bst_insert(n->r, v);
    return n;
}
static int bst_height(const BNode *n)
{
    if (n == NULL) return 0;
    int a = bst_height(n->l), b = bst_height(n->r);
    return 1 + (a > b ? a : b);
}
static void bst_free(BNode *n) { if (n) { bst_free(n->l); bst_free(n->r); free(n); } }

int main(void)
{
    puts("=== THE PROBLEM AVL SOLVES ===");
    puts("  A plain BST fed SORTED input degenerates into a linked list:");
    puts("  every value goes right, height becomes n, everything becomes O(n).");
    puts("  Sorted input is not exotic — timestamps, auto-increment IDs, and");
    puts("  anything read from a sorted file all look like this.\n");

    puts("=== ROTATIONS: change the SHAPE, never the ORDER ===");
    puts("        y                    x");
    puts("       / \\    rotate_right  / \\");
    puts("      x   C    ------->    A   y");
    puts("     / \\                      / \\");
    puts("    A   B                     B   C");
    puts("");
    puts("  In-order before: A x B y C");
    puts("  In-order after : A x B y C     <- IDENTICAL");
    puts("  That is the entire proof that a rotation preserves the BST property.");
    puts("  It rewires three pointers and is O(1).\n");

    puts("=== WATCHING IT REBALANCE: inserting 1,2,3 (worst case) ===");
    {
        Node *root = NULL;
        bool ins;

        for (int v = 1; v <= 3; v++) {
            ins = false;
            root = avl_insert(root, v, &ins);
            printf("\n  after inserting %d  (height %d, %zu rotations so far):\n",
                   v, node_height(root), rotations);
            print_tree(root, 1);
        }
        puts("\n  A plain BST would now be 1 -> 2 -> 3, a chain of height 3.");
        printf("  The AVL tree rotated once and is height %d, with 2 as the root.\n",
               node_height(root));
        printf("  invariants hold: %s\n", is_avl(root, -1000000, 1000000) ? "yes" : "NO");
        free_tree(root);
    }

    puts("\n=== THE FOUR CASES ===");
    {
        struct { const char *name; int vals[3]; const char *fix; } cases[] = {
            {"LEFT-LEFT   (30,20,10)", {30,20,10}, "one rotate_right"},
            {"LEFT-RIGHT  (30,10,20)", {30,10,20}, "rotate_left on the child, then rotate_right"},
            {"RIGHT-RIGHT (10,20,30)", {10,20,30}, "one rotate_left"},
            {"RIGHT-LEFT  (10,30,20)", {10,30,20}, "rotate_right on the child, then rotate_left"},
        };

        for (size_t c = 0; c < 4; c++) {
            Node *root = NULL;
            bool ins;
            size_t before = rotations;
            for (int i = 0; i < 3; i++) { ins = false; root = avl_insert(root, cases[c].vals[i], &ins); }

            printf("  %-24s -> root %d, height %d, %zu rotation(s)\n",
                   cases[c].name, root->value, node_height(root), rotations - before);
            printf("      %s\n", cases[c].fix);
            printf("      result: ");
            inorder(root);
            printf(" (always the same sorted order)\n");
            free_tree(root);
        }
        puts("");
        puts("  The two SINGLE-rotation cases are when the child leans the SAME");
        puts("  way as the parent. The two DOUBLE-rotation cases are when it");
        puts("  leans the other way — a single rotation there would just move");
        puts("  the imbalance to the other side, so you straighten the child first.");
    }

    puts("\n=== AVL vs PLAIN BST ON SORTED INPUT ===");
    {
        const int N = 10000;
        Node  *avl = NULL;
        BNode *bst = NULL;
        bool ins;

        rotations = 0;
        for (int i = 0; i < N; i++) { ins = false; avl = avl_insert(avl, i, &ins); }
        for (int i = 0; i < N; i++) bst = bst_insert(bst, i);

        printf("  inserting 0..%d IN SORTED ORDER:\n", N - 1);
        printf("    plain BST : height %5d   <- a linked list\n", bst_height(bst));
        printf("    AVL tree  : height %5d   <- %zu rotations kept it balanced\n",
               node_height(avl), rotations);
        printf("    theory    : AVL height <= 1.44 * log2(%d) = %.0f\n",
               N, 1.44 * 13.29);

        size_t steps;
        avl_contains(avl, N - 1, &steps);
        printf("  searching for the largest value:\n");
        printf("    AVL       : %zu comparisons\n", steps);
        printf("    plain BST : %d comparisons (it must walk the entire chain)\n", N);

        printf("  invariants still hold after %d inserts: %s\n",
               N, is_avl(avl, -1000000, 1000000) ? "yes" : "NO");
        printf("  node count: %d (expected %d)\n", count_nodes(avl), N);

        free_tree(avl); bst_free(bst);
    }

    puts("\n=== DELETION KEEPS IT BALANCED TOO ===");
    {
        Node *root = NULL;
        bool ins;
        for (int i = 1; i <= 15; i++) { ins = false; root = avl_insert(root, i * 10, &ins); }
        printf("  built with 15 values, height %d\n", node_height(root));

        rotations = 0;
        for (int i = 1; i <= 10; i++) {
            bool rem = false;
            root = avl_delete(root, i * 10, &rem);
        }
        printf("  after deleting 10 of them: height %d, %zu rotations, %d nodes left\n",
               node_height(root), rotations, count_nodes(root));
        printf("  remaining: ");
        inorder(root);
        printf("\n  invariants hold: %s\n", is_avl(root, -1000000, 1000000) ? "yes" : "NO");
        free_tree(root);

        puts("  Note that insert and delete each needed exactly ONE extra line:");
        puts("      return rebalance(n);");
        puts("  Because the recursion returns the new subtree root up the chain,");
        puts("  a rotation anywhere fixes up its parent automatically. That is");
        puts("  why the recursive formulation is worth the stack frames here.");
    }

    puts("\n=== THE COST AND THE ALTERNATIVES ===");
    puts("  AVL costs: 4 extra bytes per node for the height, a rebalance check");
    puts("  on every insert and delete, and up to O(log n) rotations per delete.");
    puts("  In exchange every operation is O(log n) in the WORST case.");
    puts("");
    puts("  RED-BLACK TREE    looser invariant (longest path <= 2x shortest),");
    puts("                    so FEWER rotations on write, slightly taller on");
    puts("                    read. Used by the Linux kernel scheduler, C++");
    puts("                    std::map, and Java TreeMap. Better for write-heavy.");
    puts("  AVL               stricter, so SHORTER trees and faster lookups.");
    puts("                    Better for read-heavy workloads (e.g. an index).");
    puts("  B-TREE / B+TREE   nodes hold many keys, sized to a disk page or a");
    puts("                    cache line. THE structure for databases and");
    puts("                    filesystems — height 3-4 for millions of keys.");
    puts("  SKIP LIST         randomised, probabilistically balanced, far easier");
    puts("                    to implement and to make lock-free. Used by Redis.");
    puts("  TREAP             a BST + a heap on random priorities. ~40 lines,");
    puts("                    and expected O(log n). The best effort-to-benefit");
    puts("                    ratio if you need a balanced tree in a hurry.");

    return 0;
}
