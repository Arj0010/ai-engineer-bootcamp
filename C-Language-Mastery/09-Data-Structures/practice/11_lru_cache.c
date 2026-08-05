/* 11_lru_cache.c — O(1) get and put, by COMPOSING two structures.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 11_lru_cache.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * THE REQUIREMENT: a fixed-capacity cache. On a miss with a full cache,
 * evict the LEAST RECENTLY USED entry. Both get and put must be O(1).
 *
 * WHY NEITHER STRUCTURE ALONE WORKS:
 *   a hash map alone      -> O(1) lookup, but no notion of order, so finding
 *                            the least-recently-used entry is O(n)
 *   a linked list alone   -> O(1) reordering, but finding a key is O(n)
 *
 * THE ANSWER: use BOTH, pointing at the SAME nodes.
 *   - a doubly linked list holds the entries in recency order (most recent
 *     at the front), giving O(1) "move to front" and O(1) "evict the back"
 *   - a hash map maps key -> the list node, giving O(1) lookup
 *
 * The doubly linked list is essential: you must unlink a node from the
 * MIDDLE in O(1), and a singly linked list cannot (it has no `prev`).
 *
 * This composition-of-two-structures move is the main lesson of the file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct LRUNode {
    char           *key;         /* OWNED */
    int             value;
    struct LRUNode *prev, *next; /* the RECENCY list */
    struct LRUNode *hash_next;   /* the hash bucket chain — a separate list! */
} LRUNode;

typedef struct {
    LRUNode **buckets;
    size_t    n_buckets;

    LRUNode  *head, *tail;       /* head = most recent, tail = least recent */
    size_t    size, capacity;

    size_t    hits, misses, evictions;
} LRUCache;

static uint64_t hash_key(const char *s)
{
    uint64_t h = 1469598103934665603ULL;
    for (; *s; s++) { h ^= (uint64_t)(unsigned char)*s; h *= 1099511628211ULL; }
    return h;
}

static bool lru_init(LRUCache *c, size_t capacity, size_t n_buckets)
{
    c->buckets = calloc(n_buckets, sizeof *c->buckets);
    if (c->buckets == NULL) return false;
    c->n_buckets = n_buckets;
    c->head = c->tail = NULL;
    c->size = 0; c->capacity = capacity;
    c->hits = c->misses = c->evictions = 0;
    return true;
}

static void lru_free(LRUCache *c)
{
    LRUNode *n = c->head;
    while (n) { LRUNode *next = n->next; free(n->key); free(n); n = next; }
    free(c->buckets);
    memset(c, 0, sizeof *c);
}

/* ---- the recency list: three O(1) primitives ---- */

static void list_unlink(LRUCache *c, LRUNode *n)
{
    if (n->prev) n->prev->next = n->next; else c->head = n->next;
    if (n->next) n->next->prev = n->prev; else c->tail = n->prev;
    n->prev = n->next = NULL;
}
static void list_push_front(LRUCache *c, LRUNode *n)
{
    n->prev = NULL;
    n->next = c->head;
    if (c->head) c->head->prev = n;
    c->head = n;
    if (c->tail == NULL) c->tail = n;
}
/* "This entry was just used." O(1) precisely because the list is DOUBLY
 * linked — we can unlink from the middle without searching for the node
 * that precedes it. */
static void list_touch(LRUCache *c, LRUNode *n)
{
    if (c->head == n) return;               /* already most recent */
    list_unlink(c, n);
    list_push_front(c, n);
}

/* ---- the hash index ---- */

static LRUNode *hash_find(const LRUCache *c, const char *key)
{
    size_t idx = (size_t)(hash_key(key) % c->n_buckets);
    for (LRUNode *n = c->buckets[idx]; n; n = n->hash_next)
        if (strcmp(n->key, key) == 0) return n;
    return NULL;
}
static void hash_insert(LRUCache *c, LRUNode *n)
{
    size_t idx = (size_t)(hash_key(n->key) % c->n_buckets);
    n->hash_next = c->buckets[idx];
    c->buckets[idx] = n;
}
static void hash_remove(LRUCache *c, const char *key)
{
    size_t idx = (size_t)(hash_key(key) % c->n_buckets);
    for (LRUNode **pp = &c->buckets[idx]; *pp; pp = &(*pp)->hash_next)
        if (strcmp((*pp)->key, key) == 0) { *pp = (*pp)->hash_next; return; }
}

/* ---- the public API ---- */

static bool lru_get(LRUCache *c, const char *key, int *out)
{
    LRUNode *n = hash_find(c, key);          /* O(1) via the hash */
    if (n == NULL) { c->misses++; return false; }

    list_touch(c, n);                        /* O(1) via the doubly linked list */
    c->hits++;
    if (out) *out = n->value;
    return true;
}

static bool lru_put(LRUCache *c, const char *key, int value)
{
    LRUNode *existing = hash_find(c, key);
    if (existing != NULL) {                  /* update: no eviction needed */
        existing->value = value;
        list_touch(c, existing);
        return true;
    }

    if (c->size == c->capacity) {            /* EVICT the least recent */
        LRUNode *victim = c->tail;           /* O(1): it is at the tail */
        hash_remove(c, victim->key);
        list_unlink(c, victim);
        free(victim->key);
        free(victim);
        c->size--;
        c->evictions++;
    }

    LRUNode *n = calloc(1, sizeof *n);
    if (n == NULL) return false;
    size_t klen = strlen(key) + 1;
    n->key = malloc(klen);
    if (n->key == NULL) { free(n); return false; }
    memcpy(n->key, key, klen);
    n->value = value;

    hash_insert(c, n);                       /* into the index */
    list_push_front(c, n);                   /* most recently used */
    c->size++;
    return true;
}

static void lru_print(const LRUCache *c, const char *label)
{
    printf("  %-26s [", label);
    for (const LRUNode *n = c->head; n; n = n->next)
        printf("%s=%d%s", n->key, n->value, n->next ? ", " : "");
    printf("]  (%zu/%zu)\n", c->size, c->capacity);
}

/* Verify the two structures still agree — the invariant that composition
 * puts at risk, and the thing to assert in tests. */
static bool lru_check(const LRUCache *c)
{
    size_t forward = 0;
    for (const LRUNode *n = c->head; n; n = n->next) {
        forward++;
        if (n->next && n->next->prev != n) return false;   /* list is consistent */
        if (hash_find(c, n->key) != n)     return false;   /* hash agrees */
    }
    size_t backward = 0;
    for (const LRUNode *n = c->tail; n; n = n->prev) backward++;
    return forward == c->size && backward == c->size;
}

int main(void)
{
    puts("=== WHY ONE STRUCTURE IS NOT ENOUGH ===");
    puts("  hash map alone : O(1) lookup, but NO ORDER — finding the least");
    puts("                   recently used entry means scanning everything");
    puts("  linked list alone: O(1) reordering, but finding a key is O(n)");
    puts("");
    puts("  So use BOTH, pointing at THE SAME NODES:");
    puts("");
    puts("    hash buckets            recency list (most recent first)");
    puts("    [0] -> \"b\" ------------\\");
    puts("    [1] -> \"a\" ---------\\   \\");
    puts("    [2] -> NULL          v    v");
    puts("    [3] -> \"c\" ---> head:a <-> b <-> c :tail");
    puts("                          ^least recently used is at the TAIL");
    puts("");
    puts("  Each node carries TWO sets of links: prev/next for recency, and");
    puts("  hash_next for its bucket chain. They are independent lists over");
    puts("  the same objects.\n");

    LRUCache c;
    if (!lru_init(&c, 3, 16)) return 1;            /* capacity 3, so eviction is visible */

    puts("=== FILLING THE CACHE ===");
    lru_put(&c, "a", 1); lru_print(&c, "put a=1");
    lru_put(&c, "b", 2); lru_print(&c, "put b=2");
    lru_put(&c, "c", 3); lru_print(&c, "put c=3");
    puts("      most recent is on the LEFT, least recent on the RIGHT");

    puts("\n=== GET PROMOTES TO MOST-RECENT ===");
    {
        int v;
        lru_get(&c, "a", &v);
        printf("  get(a) = %d\n", v);
        lru_print(&c, "after get(a)");
        puts("      \"a\" moved to the front. This is list_touch: unlink + push_front,");
        puts("      both O(1) because the list is DOUBLY linked.");
    }

    puts("\n=== EVICTION ===");
    {
        lru_put(&c, "d", 4);
        lru_print(&c, "put d=4 (cache was full)");
        printf("      \"b\" was evicted — it was at the TAIL, i.e. least recently used\n");
        int v;
        printf("      get(b) -> %s\n", lru_get(&c, "b", &v) ? "found?!" : "MISS, correctly gone");
        printf("      get(a) -> %s\n", lru_get(&c, "a", &v) ? "still cached" : "gone");
        puts("      Eviction is O(1): the victim is always c->tail. No search.");
    }

    puts("\n=== UPDATING AN EXISTING KEY ===");
    {
        lru_put(&c, "c", 33);
        lru_print(&c, "put c=33 (already present)");
        puts("      value updated, promoted to front, and NOTHING was evicted —");
        puts("      the size did not change.");
    }

    printf("\n  both structures still consistent: %s\n", lru_check(&c) ? "yes" : "NO");

    puts("\n=== A REALISTIC ACCESS PATTERN ===");
    {
        LRUCache big;
        if (!lru_init(&big, 4, 32)) return 1;

        /* A working set of 3 keys, with occasional outliers — the shape of
         * most real workloads, and the shape LRU is designed for. */
        const char *sequence[] = {
            "user1","user2","user3","user1","user2","user4","user1",
            "user2","user3","user5","user1","user2","user3","user1"
        };
        size_t n = sizeof sequence / sizeof sequence[0];

        for (size_t i = 0; i < n; i++) {
            int v;
            if (!lru_get(&big, sequence[i], &v))
                lru_put(&big, sequence[i], (int)i);      /* miss: fetch and cache */
        }

        printf("  %zu accesses to 5 distinct keys, capacity 4\n", n);
        printf("    hits      : %zu\n", big.hits);
        printf("    misses    : %zu\n", big.misses);
        printf("    evictions : %zu\n", big.evictions);
        printf("    hit rate  : %.0f%%\n",
               100.0 * (double)big.hits / (double)(big.hits + big.misses));
        lru_print(&big, "final state");
        printf("  consistent: %s\n", lru_check(&big) ? "yes" : "NO");
        puts("");
        puts("  LRU works because real access patterns have TEMPORAL LOCALITY:");
        puts("  what you touched recently, you are likely to touch again. When");
        puts("  that assumption fails, LRU fails badly — a full linear scan of");
        puts("  data larger than the cache evicts everything useful and achieves");
        puts("  a 0% hit rate. That is called CACHE THRASHING, and it is why");
        puts("  databases use scan-resistant policies instead.");

        lru_free(&big);
    }

    puts("\n=== COMPLEXITY ===");
    puts("    get   O(1)   hash lookup + list_touch");
    puts("    put   O(1)   hash lookup + insert + possible eviction from the tail");
    puts("    space O(capacity)");
    puts("  Nothing here is amortised or average-case except the hash lookup.");

    puts("\n=== WHY THE LIST MUST BE DOUBLY LINKED ===");
    puts("  list_touch has to unlink a node from the MIDDLE. With a singly");
    puts("  linked list you would have to search for the node BEFORE it to fix");
    puts("  up its `next` — O(n), and the whole design collapses.");
    puts("  The `prev` pointer is what buys O(1). That is the one reason a");
    puts("  doubly linked list exists.");

    puts("\n=== OTHER EVICTION POLICIES ===");
    puts("  LRU    least recently used     the default; good temporal locality");
    puts("  LFU    least FREQUENTLY used   better for stable popularity, but it");
    puts("                                 clings to old hot items (needs ageing)");
    puts("  FIFO   oldest inserted         trivial (a plain queue), often worse");
    puts("  CLOCK  approximate LRU         one reference bit per entry, no list");
    puts("                                 reordering. What OS page caches use.");
    puts("  ARC    adaptive                balances recency and frequency;");
    puts("                                 scan-resistant. Used by ZFS.");
    puts("  2Q / LIRS                      scan-resistant variants of LRU");
    puts("  RANDOM                         genuinely competitive, and O(1) with");
    puts("                                 no bookkeeping at all");
    puts("");
    puts("  THE TRANSFERABLE LESSON: when no single structure gives you every");
    puts("  operation in the complexity you need, COMPOSE TWO and keep them");
    puts("  pointing at the same objects. The cost is the invariant between");
    puts("  them — which is exactly what lru_check() exists to assert.");

    lru_free(&c);
    return 0;
}
