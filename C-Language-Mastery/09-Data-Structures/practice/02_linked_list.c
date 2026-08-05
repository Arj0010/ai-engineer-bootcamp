/* 02_linked_list.c — singly and doubly linked lists, and the classic problems.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 02_linked_list.c -o t && ./t
 *   valgrind --leak-check=full ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* ================================================================= *
 * SINGLY LINKED LIST
 * ================================================================= */
typedef struct SNode { int value; struct SNode *next; } SNode;
typedef struct { SNode *head; SNode *tail; size_t len; } SList;

static void slist_init(SList *l) { l->head = l->tail = NULL; l->len = 0; }

static void slist_free(SList *l)
{
    SNode *n = l->head;
    while (n != NULL) {
        SNode *next = n->next;      /* save it BEFORE freeing — classic bug */
        free(n);
        n = next;
    }
    slist_init(l);
}

static bool slist_push_front(SList *l, int v)
{
    SNode *n = malloc(sizeof *n);
    if (n == NULL) return false;
    n->value = v;
    n->next  = l->head;
    l->head  = n;
    if (l->tail == NULL) l->tail = n;    /* the list was empty */
    l->len++;
    return true;
}

/* O(1) only because we keep a tail pointer. Without one this is O(n). */
static bool slist_push_back(SList *l, int v)
{
    SNode *n = malloc(sizeof *n);
    if (n == NULL) return false;
    n->value = v;
    n->next  = NULL;
    if (l->tail) l->tail->next = n; else l->head = n;
    l->tail = n;
    l->len++;
    return true;
}

static bool slist_pop_front(SList *l, int *out)
{
    if (l->head == NULL) return false;
    SNode *n = l->head;
    if (out) *out = n->value;
    l->head = n->next;
    if (l->head == NULL) l->tail = NULL;
    free(n);
    l->len--;
    return true;
}

/* Remove the first node with this value. O(n).
 *
 * THE TECHNIQUE WORTH LEARNING: a pointer-to-pointer walks the list and
 * removes the head with no special case at all. `pp` points at whatever
 * field references the current node — either l->head or some node's ->next —
 * so writing through it fixes up the right one automatically. */
static bool slist_remove_value(SList *l, int v)
{
    SNode *prev = NULL;                     /* only needed to fix up `tail` */

    for (SNode **pp = &l->head; *pp != NULL; prev = *pp, pp = &(*pp)->next) {
        if ((*pp)->value != v) continue;

        SNode *dead = *pp;
        *pp = dead->next;                   /* unlink — works even at the head */
        if (dead == l->tail) l->tail = prev; /* NULL if the list is now empty */
        free(dead);
        l->len--;
        return true;
    }
    return false;
}

/* Reverse in place. Three pointers, one pass, O(n) time and O(1) space. */
static void slist_reverse(SList *l)
{
    SNode *prev = NULL, *cur = l->head;
    l->tail = l->head;
    while (cur != NULL) {
        SNode *next = cur->next;    /* 1. remember where we were going */
        cur->next = prev;           /* 2. point backwards */
        prev = cur;                 /* 3. advance both */
        cur = next;
    }
    l->head = prev;
}

/* Find the middle in ONE pass with two pointers: fast moves twice per step,
 * so when it reaches the end, slow is halfway. */
static SNode *slist_middle(const SList *l)
{
    SNode *slow = l->head, *fast = l->head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    return slow;
}

/* Floyd's cycle detection ("tortoise and hare"). If there is a loop, the fast
 * pointer eventually laps the slow one; if there is not, it hits NULL.
 * O(n) time, O(1) space — no visited set needed. */
static bool slist_has_cycle(const SNode *head)
{
    const SNode *slow = head, *fast = head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

/* Merge sort on a linked list: O(n log n) time, O(log n) stack, and NO
 * extra array. This is where lists genuinely beat arrays — there is no
 * data movement at all, only pointer rewiring. */
static SNode *slist_split_half(SNode *head)
{
    if (head == NULL || head->next == NULL) return NULL;
    SNode *slow = head, *fast = head->next;
    while (fast != NULL && fast->next != NULL) { slow = slow->next; fast = fast->next->next; }
    SNode *second = slow->next;
    slow->next = NULL;                     /* cut the list in two */
    return second;
}
static SNode *slist_merge(SNode *a, SNode *b)
{
    SNode dummy;                            /* a DUMMY HEAD removes every */
    SNode *tail = &dummy;                   /* special case from this loop */
    dummy.next = NULL;

    while (a != NULL && b != NULL) {
        if (a->value <= b->value) { tail->next = a; a = a->next; }
        else                      { tail->next = b; b = b->next; }
        tail = tail->next;
    }
    tail->next = (a != NULL) ? a : b;       /* attach whatever remains */
    return dummy.next;
}
static SNode *slist_msort(SNode *head)
{
    if (head == NULL || head->next == NULL) return head;
    SNode *second = slist_split_half(head);
    return slist_merge(slist_msort(head), slist_msort(second));
}
static void slist_sort(SList *l)
{
    l->head = slist_msort(l->head);
    l->tail = l->head;
    while (l->tail != NULL && l->tail->next != NULL) l->tail = l->tail->next;
}

static void slist_print(const SList *l, const char *label)
{
    printf("  %-24s ", label);
    for (const SNode *n = l->head; n != NULL; n = n->next) printf("%d -> ", n->value);
    printf("NULL  (len %zu)\n", l->len);
}

/* ================================================================= *
 * DOUBLY LINKED LIST — with a SENTINEL node.
 *
 * The sentinel's next is the first element and its prev is the last, so the
 * list is a ring. There is never a NULL to check and never an empty-list
 * special case: insert and remove are the same four assignments every time.
 * ================================================================= */
typedef struct DNode {
    int value;
    struct DNode *prev, *next;
} DNode;

typedef struct { DNode sentinel; size_t len; } DList;

static void dlist_init(DList *l)
{
    l->sentinel.next = l->sentinel.prev = &l->sentinel;   /* points at itself */
    l->sentinel.value = 0;
    l->len = 0;
}
static bool dlist_is_empty(const DList *l) { return l->sentinel.next == &l->sentinel; }

/* Insert `n` before `pos`. With a sentinel this covers push_front (pos =
 * sentinel.next), push_back (pos = &sentinel), and insert-in-the-middle,
 * with ZERO branches. */
static bool dlist_insert_before(DList *l, DNode *pos, int v)
{
    DNode *n = malloc(sizeof *n);
    if (n == NULL) return false;
    n->value = v;
    n->prev = pos->prev;
    n->next = pos;
    pos->prev->next = n;
    pos->prev = n;
    l->len++;
    return true;
}
static bool dlist_push_back (DList *l, int v) { return dlist_insert_before(l, &l->sentinel, v); }
static bool dlist_push_front(DList *l, int v) { return dlist_insert_before(l, l->sentinel.next, v); }

/* O(1) removal GIVEN THE NODE. This is the one thing a doubly linked list
 * does that nothing else can, and it is why LRU caches use one. */
static void dlist_unlink(DList *l, DNode *n)
{
    n->prev->next = n->next;
    n->next->prev = n->prev;
    free(n);
    l->len--;
}
static void dlist_free(DList *l)
{
    DNode *n = l->sentinel.next;
    while (n != &l->sentinel) { DNode *next = n->next; free(n); n = next; }
    dlist_init(l);
}
static void dlist_print(const DList *l, const char *label)
{
    printf("  %-24s ", label);
    for (const DNode *n = l->sentinel.next; n != &l->sentinel; n = n->next)
        printf("%d <-> ", n->value);
    printf("(sentinel)  len=%zu\n", l->len);
}
static void dlist_print_reverse(const DList *l, const char *label)
{
    printf("  %-24s ", label);
    for (const DNode *n = l->sentinel.prev; n != &l->sentinel; n = n->prev)
        printf("%d <-> ", n->value);
    printf("(sentinel)\n");
}

#include <stddef.h>

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== SINGLY LINKED LIST ===");
    {
        SList l; slist_init(&l);
        for (int i = 1; i <= 5; i++) slist_push_back(&l, i * 10);
        slist_print(&l, "push_back 10..50");

        slist_push_front(&l, 5);
        slist_print(&l, "push_front 5");

        int out = 0;
        slist_pop_front(&l, &out);
        printf("  pop_front -> %d\n", out);
        slist_print(&l, "after pop_front");

        slist_remove_value(&l, 30);
        slist_print(&l, "remove value 30");

        printf("  middle element: %d\n", slist_middle(&l)->value);

        slist_reverse(&l);
        slist_print(&l, "reversed");

        slist_free(&l);
    }

    puts("\n=== THE POINTER-TO-POINTER REMOVAL TECHNIQUE ===");
    puts("      for (SNode **pp = &l->head; *pp; pp = &(*pp)->next)");
    puts("          if ((*pp)->value == v) { *pp = (*pp)->next; ... }");
    puts("  `pp` points at the FIELD that references the current node — either");
    puts("  l->head or some node's ->next. Writing through it fixes whichever");
    puts("  one it is, so removing the head needs no special case.");
    puts("  The naive version needs a `prev` pointer and an `if (prev == NULL)`");
    puts("  branch, and that branch is where the bug always is.");

    puts("\n=== FLOYD'S CYCLE DETECTION ===");
    {
        SList l; slist_init(&l);
        for (int i = 1; i <= 6; i++) slist_push_back(&l, i);
        printf("  a normal list has a cycle? %s\n",
               slist_has_cycle(l.head) ? "yes" : "no");

        /* Make the tail point back to the third node. */
        SNode *third = l.head->next->next;
        l.tail->next = third;
        printf("  after linking tail -> node 3: %s\n",
               slist_has_cycle(l.head) ? "CYCLE DETECTED" : "no cycle");
        l.tail->next = NULL;                  /* undo, so we can free it */
        slist_free(&l);

        puts("  Two pointers, one moving twice as fast. If there is a loop the");
        puts("  fast one laps the slow one; if not, it runs off the end.");
        puts("  O(n) time, O(1) space — no visited set, no marking.");
        puts("  The same trick finds the middle in one pass (see above).");
    }

    puts("\n=== MERGE SORT ON A LINKED LIST ===");
    {
        SList l; slist_init(&l);
        int vals[] = {38, 27, 43, 3, 9, 82, 10, 1, 55};
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++)
            slist_push_back(&l, vals[i]);
        slist_print(&l, "unsorted");
        slist_sort(&l);
        slist_print(&l, "merge sorted");
        printf("  tail correctly updated to %d\n", l.tail->value);
        slist_free(&l);

        puts("  Merge sort is THE list sort: O(n log n), and it moves no data at");
        puts("  all — only pointers are rewired. Quicksort needs random access,");
        puts("  so it degrades badly on a list.");
        puts("  The dummy head in slist_merge removes every empty/first-element");
        puts("  special case from the merge loop.");
    }

    puts("\n=== DOUBLY LINKED LIST WITH A SENTINEL ===");
    {
        DList l; dlist_init(&l);
        printf("  empty? %s\n", dlist_is_empty(&l) ? "yes" : "no");

        for (int i = 1; i <= 4; i++) dlist_push_back(&l, i * 10);
        dlist_push_front(&l, 5);
        dlist_print(&l, "forwards");
        dlist_print_reverse(&l, "backwards");

        /* O(1) removal given the node — the defining feature. */
        DNode *third = l.sentinel.next->next->next;
        printf("  unlinking the node holding %d (O(1), no search)\n", third->value);
        dlist_unlink(&l, third);
        dlist_print(&l, "after unlink");

        dlist_free(&l);

        puts("\n  The SENTINEL is the trick. Its next is the first element and its");
        puts("  prev is the last, so the list is a RING and there is never a NULL.");
        puts("  insert_before covers push_front, push_back and insert-in-the-");
        puts("  middle with the SAME four assignments and no branches:");
        puts("      n->prev = pos->prev;  n->next = pos;");
        puts("      pos->prev->next = n;  pos->prev = n;");
        puts("  A NULL-terminated doubly linked list needs four special cases");
        puts("  (empty, front, back, middle) in both insert and remove. Every one");
        puts("  of those is a place to forget to update `prev`.");
    }

    puts("\n=== WHEN A LINKED LIST IS ACTUALLY THE RIGHT CHOICE ===");
    {
        const int N = 300000;
        clock_t t;

        /* Traversal: array vs list, same element count. */
        int *arr = malloc((size_t)N * sizeof *arr);
        SList l; slist_init(&l);
        for (int i = 0; i < N; i++) { arr[i] = i; slist_push_back(&l, i); }

        t = clock();
        long long sa = 0;
        for (int r = 0; r < 20; r++) for (int i = 0; i < N; i++) sa += arr[i];
        double t_arr = seconds_since(t);

        t = clock();
        long long sl = 0;
        for (int r = 0; r < 20; r++)
            for (const SNode *n = l.head; n; n = n->next) sl += n->value;
        double t_list = seconds_since(t);

        printf("  20 traversals of %d elements (sums match: %s)\n",
               N, sa == sl ? "yes" : "no");
        printf("    array : %.4f s\n", t_arr);
        printf("    list  : %.4f s  (%.1fx slower)\n", t_list,
               t_arr > 0 ? t_list / t_arr : 0);
        printf("  memory: array %zu KB, list %zu KB (%.1fx)\n",
               (size_t)N * sizeof(int) / 1024,
               (size_t)N * sizeof(SNode) / 1024,
               (double)sizeof(SNode) / sizeof(int));

        free(arr); slist_free(&l);

        puts("");
        puts("  The array wins traversal because it is CONTIGUOUS: one cache line");
        puts("  brings in 16 ints, and the prefetcher predicts the next line.");
        puts("  Each list node is a separate allocation, so every step is a");
        puts("  potential cache miss the CPU cannot predict — it does not know");
        puts("  the next address until the current node has arrived.");
        puts("");
        puts("  USE A LINKED LIST WHEN:");
        puts("    - you need O(1) removal given a node pointer (LRU caches,");
        puts("      free lists, intrusive kernel lists) — this is the real one");
        puts("    - elements must never move (pointers into them stay valid,");
        puts("      unlike a dynamic array's)");
        puts("    - you are splicing whole sublists together in O(1)");
        puts("    - allocation must be incremental with no large contiguous block");
        puts("");
        puts("  OTHERWISE USE A DYNAMIC ARRAY. 'Insert in the middle is O(1)' is");
        puts("  only true if you ALREADY have the node pointer; finding it is");
        puts("  O(n), and that search is slower than the array's memmove.");
    }

    return 0;
}
