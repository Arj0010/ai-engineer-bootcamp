/* 08_trie.c — the prefix tree. Lookup in O(key length), independent of n.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 08_trie.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * A trie stores keys in the SHAPE of the tree rather than in the nodes: the
 * path from the root spells the key. Two consequences follow immediately:
 *
 *   1. Lookup is O(k) where k is the KEY LENGTH — it does not depend on how
 *      many keys are stored. A million-word trie searches as fast as a
 *      ten-word one.
 *   2. Every PREFIX is a node, so "all words starting with X" is free.
 *      That is what a hash table cannot do at any price.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define ALPHABET 26          /* 'a'..'z' only, to keep the demo readable */

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET];
    bool             is_word;      /* does a key END here? */
    int              value;        /* the payload for that key */
    size_t           n_children;   /* maintained so we can prune on delete */
} TrieNode;

typedef struct { TrieNode *root; size_t n_words, n_nodes; } Trie;

static TrieNode *node_new(Trie *t)
{
    TrieNode *n = calloc(1, sizeof *n);      /* calloc: children all NULL */
    if (n != NULL) t->n_nodes++;
    return n;
}

static bool trie_init(Trie *t)
{
    t->n_words = 0; t->n_nodes = 0;
    t->root = node_new(t);
    return t->root != NULL;
}

static int char_index(char c) { return tolower((unsigned char)c) - 'a'; }
static bool valid_key(const char *s)
{
    if (*s == '\0') return false;
    for (; *s; s++) { int i = char_index(*s); if (i < 0 || i >= ALPHABET) return false; }
    return true;
}

/* INSERT: walk down, creating nodes for characters that do not exist yet.
 * O(k). Note that keys sharing a prefix SHARE those nodes — that is where
 * the memory saving comes from. */
static bool trie_insert(Trie *t, const char *key, int value)
{
    if (!valid_key(key)) return false;

    TrieNode *n = t->root;
    for (const char *c = key; *c; c++) {
        int i = char_index(*c);
        if (n->children[i] == NULL) {
            n->children[i] = node_new(t);
            if (n->children[i] == NULL) return false;
            n->n_children++;
        }
        n = n->children[i];
    }
    if (!n->is_word) { n->is_word = true; t->n_words++; }
    n->value = value;                 /* update if it already existed */
    return true;
}

/* Walk to the node for a prefix, or NULL. Both search and prefix-match
 * are built on this. */
static TrieNode *trie_walk(const Trie *t, const char *prefix, size_t *steps)
{
    TrieNode *n = t->root;
    size_t s = 0;
    for (const char *c = prefix; *c; c++) {
        int i = char_index(*c);
        if (i < 0 || i >= ALPHABET) return NULL;
        s++;
        n = n->children[i];
        if (n == NULL) { if (steps) *steps = s; return NULL; }
    }
    if (steps) *steps = s;
    return n;
}

/* SEARCH: walk, then check is_word. The distinction matters — "car" being a
 * PREFIX of "carpet" does not make "car" a stored key. */
static bool trie_search(const Trie *t, const char *key, int *out, size_t *steps)
{
    TrieNode *n = trie_walk(t, key, steps);
    if (n == NULL || !n->is_word) return false;
    if (out) *out = n->value;
    return true;
}

static bool trie_has_prefix(const Trie *t, const char *prefix)
{
    return trie_walk(t, prefix, NULL) != NULL;
}

/* Collect every key under a node. This is the operation that justifies a
 * trie's existence: autocomplete, in O(number of matches). */
static void collect(const TrieNode *n, char *buf, size_t depth, size_t bufsize,
                    void (*emit)(const char *, int, void *), void *ctx)
{
    if (n == NULL || depth + 1 >= bufsize) return;
    if (n->is_word) { buf[depth] = '\0'; emit(buf, n->value, ctx); }
    for (int i = 0; i < ALPHABET; i++) {
        if (n->children[i] == NULL) continue;
        buf[depth] = (char)('a' + i);
        collect(n->children[i], buf, depth + 1, bufsize, emit, ctx);
    }
}

static void trie_complete(const Trie *t, const char *prefix,
                          void (*emit)(const char *, int, void *), void *ctx)
{
    const TrieNode *n = trie_walk(t, prefix, NULL);
    if (n == NULL) return;

    char buf[128];
    size_t plen = strlen(prefix);
    if (plen >= sizeof buf) return;
    memcpy(buf, prefix, plen);
    collect(n, buf, plen, sizeof buf, emit, ctx);
}

/* DELETE: unmark the word, then PRUNE upward — free any node that is now
 * neither a word nor on the path to one. Returns true if the child should
 * be removed by the caller. */
static bool delete_rec(Trie *t, TrieNode *n, const char *key, size_t depth, bool *removed)
{
    if (n == NULL) return false;

    if (key[depth] == '\0') {
        if (!n->is_word) return false;         /* it was only a prefix */
        n->is_word = false;
        t->n_words--;
        *removed = true;
        return n->n_children == 0;             /* prunable if it has no children */
    }

    int i = char_index(key[depth]);
    if (i < 0 || i >= ALPHABET) return false;

    if (delete_rec(t, n->children[i], key, depth + 1, removed)) {
        free(n->children[i]);
        n->children[i] = NULL;
        n->n_children--;
        t->n_nodes--;
    }
    /* Prune this node too if it is now useless. */
    return !n->is_word && n->n_children == 0;
}

static bool trie_delete(Trie *t, const char *key)
{
    bool removed = false;
    delete_rec(t, t->root, key, 0, &removed);   /* never prune the root */
    return removed;
}

static void trie_free_rec(TrieNode *n)
{
    if (n == NULL) return;
    for (int i = 0; i < ALPHABET; i++) trie_free_rec(n->children[i]);
    free(n);
}
static void trie_free(Trie *t) { trie_free_rec(t->root); t->root = NULL; t->n_words = t->n_nodes = 0; }

/* Longest stored key that is a prefix of `text` — the operation behind IP
 * routing tables, dictionary-based word segmentation, and URL routing. */
static size_t trie_longest_prefix(const Trie *t, const char *text, int *out)
{
    const TrieNode *n = t->root;
    size_t best = 0;
    for (size_t i = 0; text[i]; i++) {
        int idx = char_index(text[i]);
        if (idx < 0 || idx >= ALPHABET) break;
        n = n->children[idx];
        if (n == NULL) break;
        if (n->is_word) { best = i + 1; if (out) *out = n->value; }
    }
    return best;
}

/* ---- callbacks for collect ---- */
static void print_word(const char *w, int v, void *ctx)
{ (void)ctx; printf("    %s (%d)\n", w, v); }

typedef struct { int count; } Counter;
static void count_word(const char *w, int v, void *ctx)
{ (void)w; (void)v; ((Counter *)ctx)->count++; }

int main(void)
{
    Trie t;
    if (!trie_init(&t)) return 1;

    puts("=== A TRIE STORES KEYS IN THE SHAPE OF THE TREE ===");
    puts("  The PATH from the root spells the key; the nodes hold no characters.");
    puts("");
    puts("      (root)");
    puts("        |c");
    puts("        c--a--r*        \"car\"  (* = a word ends here)");
    puts("              |");
    puts("              p--e--t*  \"carpet\"");
    puts("              |");
    puts("              d*        \"card\"");
    puts("");
    puts("  \"car\", \"card\" and \"carpet\" SHARE the c-a-r nodes. Every prefix is");
    puts("  a real node, which is what makes prefix queries free.\n");

    puts("=== INSERT AND SEARCH ===");
    {
        struct { const char *w; int v; } words[] = {
            {"car",6}, {"card",4}, {"care",5}, {"careful",7}, {"carpet",6},
            {"cat",3}, {"cats",4}, {"dog",3}, {"door",4}, {"do",2},
        };
        for (size_t i = 0; i < sizeof words / sizeof words[0]; i++)
            trie_insert(&t, words[i].w, words[i].v);

        printf("  inserted %zu words using %zu nodes\n", t.n_words, t.n_nodes);
        size_t naive = 0;
        for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) naive += strlen(words[i].w);
        printf("  storing them separately would need %zu character slots;\n", naive);
        printf("  the trie used %zu nodes because prefixes are SHARED\n", t.n_nodes - 1);

        const char *probes[] = {"car", "cat", "ca", "carp", "carpet", "dodo"};
        puts("");
        for (size_t i = 0; i < 6; i++) {
            int v; size_t steps = 0;
            bool found = trie_search(&t, probes[i], &v, &steps);
            printf("  search(\"%-7s\") -> %-11s in %zu step(s)   prefix exists: %s\n",
                   probes[i], found ? "FOUND" : "not a word", steps,
                   trie_has_prefix(&t, probes[i]) ? "yes" : "no");
        }
        puts("");
        puts("  Note \"ca\" and \"carp\": both are valid PREFIXES but neither is a");
        puts("  stored WORD. That distinction is what the is_word flag encodes,");
        puts("  and forgetting it is the classic trie bug.");
    }

    puts("\n=== AUTOCOMPLETE — the reason tries exist ===");
    {
        const char *prefixes[] = {"car", "ca", "do", "z"};
        for (size_t i = 0; i < 4; i++) {
            printf("  words starting with \"%s\":\n", prefixes[i]);
            Counter c = {0};
            trie_complete(&t, prefixes[i], count_word, &c);
            if (c.count == 0) { puts("    (none)"); continue; }
            trie_complete(&t, prefixes[i], print_word, NULL);
        }
        puts("");
        puts("  Cost: walk k nodes to reach the prefix, then enumerate only the");
        puts("  subtree beneath it. It never touches an unrelated key.");
        puts("  A HASH TABLE CANNOT DO THIS AT ANY PRICE — hashing destroys the");
        puts("  relationship between \"car\" and \"carpet\", so the only option is");
        puts("  to scan every key. That is the trade you are making.");
    }

    puts("\n=== LONGEST-PREFIX MATCH ===");
    {
        const char *texts[] = {"carpeting", "cats", "doorway", "carbon", "xyz"};
        for (size_t i = 0; i < 5; i++) {
            int v = 0;
            size_t len = trie_longest_prefix(&t, texts[i], &v);
            if (len > 0)
                printf("  \"%-10s\" -> longest stored prefix is \"%.*s\" (value %d)\n",
                       texts[i], (int)len, texts[i], v);
            else
                printf("  \"%-10s\" -> no stored prefix\n", texts[i]);
        }
        puts("  This exact operation is: IP routing table lookup (longest matching");
        puts("  subnet), URL route matching, and dictionary-based word segmentation");
        puts("  for languages without spaces.");
    }

    puts("\n=== DELETE WITH PRUNING ===");
    {
        printf("  before: %zu words, %zu nodes\n", t.n_words, t.n_nodes);

        printf("  delete(\"careful\") -> %s\n", trie_delete(&t, "careful") ? "ok" : "not found");
        printf("    %zu words, %zu nodes  <- the 'ful' nodes were freed\n", t.n_words, t.n_nodes);

        printf("  delete(\"car\")     -> %s\n", trie_delete(&t, "car") ? "ok" : "not found");
        printf("    %zu words, %zu nodes  <- NO nodes freed: c-a-r is still on\n",
               t.n_words, t.n_nodes);
        puts("       the path to \"card\", \"care\" and \"carpet\"");
        printf("  is \"card\" still findable? %s\n",
               trie_search(&t, "card", NULL, NULL) ? "yes" : "NO");
        printf("  is \"car\" still a word?    %s\n",
               trie_search(&t, "car", NULL, NULL) ? "yes" : "no (correct)");

        printf("  delete(\"nonexistent\") -> %s\n",
               trie_delete(&t, "nonexistent") ? "ok?!" : "correctly refused");

        puts("");
        puts("  Deletion is the subtle operation: you may only free a node if it");
        puts("  is neither a word nor on the path to one. The recursion returns");
        puts("  'prune me' upward, and the parent frees the child and asks the");
        puts("  same question of itself.");
    }

    puts("\n=== THE MEMORY COST ===");
    {
        printf("  sizeof(TrieNode) = %zu bytes\n", sizeof(TrieNode));
        printf("    %d child pointers x %zu bytes = %zu bytes,\n",
               ALPHABET, sizeof(void *), ALPHABET * sizeof(void *));
        printf("    plus the flag, the value, and the child count\n");
        printf("  %zu nodes currently = %zu KB for %zu words\n",
               t.n_nodes, t.n_nodes * sizeof(TrieNode) / 1024, t.n_words);
        puts("");
        puts("  MOST OF THAT IS NULL POINTERS. A node with two children still");
        puts("  carries 26 slots. For a full 256-byte alphabet it is 2 KB per node.");
        puts("  This is the trie's real weakness, and there are three standard fixes:");
        puts("");
        puts("  RADIX / PATRICIA TREE  collapse every chain of single-child nodes");
        puts("                         into one node holding the whole substring.");
        puts("                         Usually a 5-10x memory reduction.");
        puts("  TERNARY SEARCH TREE    each node holds ONE character plus three");
        puts("                         pointers (lo, eq, hi). Much smaller, only");
        puts("                         slightly slower.");
        puts("  HASH MAP PER NODE      or a sorted array of (char, child) pairs.");
        puts("                         Pay a small lookup cost, save enormously on");
        puts("                         sparse nodes.");
        puts("  DAWG                   also merge identical SUFFIXES, turning the");
        puts("                         tree into a DAG. Smallest of all, but static.");
    }

    puts("\n=== TRIE vs HASH TABLE ===");
    puts("                          TRIE                  HASH TABLE");
    puts("  lookup                  O(k)                  O(k) to hash + O(1)");
    puts("  depends on n?           NO                    only via collisions");
    puts("  worst case              O(k), always          O(n) on collisions");
    puts("  prefix queries          YES, natural          impossible");
    puts("  sorted iteration        YES, free             no");
    puts("  longest-prefix match    YES                   no");
    puts("  memory                  high (sparse nodes)   low");
    puts("  cache behaviour         poor (pointer chasing) good");
    puts("");
    puts("  Use a trie when you need PREFIX operations or ORDERED traversal.");
    puts("  Use a hash table for pure key-value lookup. In practice a hash table");
    puts("  wins on raw speed; a trie wins on the queries a hash cannot answer.");
    puts("");
    puts("  Real uses: autocomplete, spell checking, IP routing tables,");
    puts("  T9 predictive text, and the Aho-Corasick multi-pattern string matcher.");

    trie_free(&t);
    return 0;
}
