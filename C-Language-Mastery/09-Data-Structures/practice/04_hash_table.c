/* 04_hash_table.c — hash tables, both collision strategies, from scratch.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 04_hash_table.c -o t && ./t
 *   valgrind --leak-check=full ./t
 *
 * A hash table is an array plus a function from keys to indices. Everything
 * hard about it is what happens when two keys land on the same index.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* ================================================================= *
 * HASH FUNCTIONS
 *
 * A good hash spreads keys uniformly and changes a lot of output bits for
 * a one-bit input change (the "avalanche" property).
 * ================================================================= */

/* FNV-1a: multiply then XOR. Simple, fast, good distribution for short
 * strings. The constants are not arbitrary — the prime is chosen so that
 * multiplication mixes every byte across the whole word. */
static uint64_t hash_fnv1a(const char *s)
{
    uint64_t h = 1469598103934665603ULL;          /* FNV offset basis */
    for (; *s; s++) {
        h ^= (uint64_t)(unsigned char)*s;         /* XOR the byte in FIRST... */
        h *= 1099511628211ULL;                    /* ...then multiply (that is */
    }                                             /* the "1a" variant)         */
    return h;
}

/* djb2: h = h*33 + c. Dan Bernstein's. Slightly worse distribution than
 * FNV-1a but famously short. */
static uint64_t hash_djb2(const char *s)
{
    uint64_t h = 5381;
    for (; *s; s++) h = ((h << 5) + h) + (uint64_t)(unsigned char)*s;  /* h*33 + c */
    return h;
}

/* A deliberately AWFUL hash, to show what collisions do to performance. */
static uint64_t hash_terrible(const char *s)
{
    uint64_t h = 0;
    for (; *s; s++) h += (uint64_t)(unsigned char)*s;   /* anagrams collide! */
    return h;
}

/* ================================================================= *
 * STRATEGY 1: SEPARATE CHAINING
 * Each bucket holds a linked list of every entry that hashed there.
 * ================================================================= */
typedef struct ChainEntry {
    char              *key;        /* OWNED */
    int                value;
    uint64_t           hash;       /* cached: avoids rehashing on resize and
                                    * lets us skip strcmp when hashes differ */
    struct ChainEntry *next;
} ChainEntry;

typedef struct {
    ChainEntry **buckets;
    size_t       n_buckets;
    size_t       count;
    uint64_t   (*hash_fn)(const char *);
    size_t       collisions;       /* for instrumentation */
} ChainTable;

static bool chain_init(ChainTable *t, size_t n_buckets, uint64_t (*fn)(const char *))
{
    t->buckets = calloc(n_buckets, sizeof *t->buckets);   /* calloc: all NULL */
    if (t->buckets == NULL) return false;
    t->n_buckets = n_buckets;
    t->count = 0;
    t->hash_fn = fn;
    t->collisions = 0;
    return true;
}

static void chain_free(ChainTable *t)
{
    for (size_t i = 0; i < t->n_buckets; i++) {
        ChainEntry *e = t->buckets[i];
        while (e != NULL) {
            ChainEntry *next = e->next;
            free(e->key);
            free(e);
            e = next;
        }
    }
    free(t->buckets);
    t->buckets = NULL; t->n_buckets = t->count = 0;
}

static bool chain_put(ChainTable *t, const char *key, int value);

/* Double the bucket count and redistribute. Because we CACHED each hash, we
 * do not have to recompute it — just re-mask it. */
static bool chain_resize(ChainTable *t, size_t new_n)
{
    ChainEntry **new_buckets = calloc(new_n, sizeof *new_buckets);
    if (new_buckets == NULL) return false;

    for (size_t i = 0; i < t->n_buckets; i++) {
        ChainEntry *e = t->buckets[i];
        while (e != NULL) {
            ChainEntry *next = e->next;
            size_t idx = (size_t)(e->hash % new_n);       /* re-mask, not rehash */
            e->next = new_buckets[idx];
            new_buckets[idx] = e;
            e = next;
        }
    }
    free(t->buckets);
    t->buckets = new_buckets;
    t->n_buckets = new_n;
    return true;
}

static bool chain_put(ChainTable *t, const char *key, int value)
{
    /* LOAD FACTOR = count / buckets. Above ~0.75 the chains get long enough
     * that lookups stop looking O(1). Resizing keeps them short. */
    if ((double)(t->count + 1) / (double)t->n_buckets > 0.75)
        if (!chain_resize(t, t->n_buckets * 2)) return false;

    uint64_t h = t->hash_fn(key);
    size_t idx = (size_t)(h % t->n_buckets);

    /* Update if the key already exists. Compare the cheap cached hash FIRST;
     * only call strcmp when it matches. */
    for (ChainEntry *e = t->buckets[idx]; e != NULL; e = e->next) {
        if (e->hash == h && strcmp(e->key, key) == 0) { e->value = value; return true; }
    }

    if (t->buckets[idx] != NULL) t->collisions++;

    ChainEntry *e = malloc(sizeof *e);
    if (e == NULL) return false;
    size_t klen = strlen(key) + 1;
    e->key = malloc(klen);
    if (e->key == NULL) { free(e); return false; }
    memcpy(e->key, key, klen);              /* the table OWNS a copy of the key */
    e->value = value;
    e->hash  = h;
    e->next  = t->buckets[idx];             /* prepend: O(1) */
    t->buckets[idx] = e;
    t->count++;
    return true;
}

static bool chain_get(const ChainTable *t, const char *key, int *out, size_t *probes)
{
    uint64_t h = t->hash_fn(key);
    size_t idx = (size_t)(h % t->n_buckets);
    size_t steps = 0;

    for (ChainEntry *e = t->buckets[idx]; e != NULL; e = e->next) {
        steps++;
        if (e->hash == h && strcmp(e->key, key) == 0) {
            if (out) *out = e->value;
            if (probes) *probes = steps;
            return true;
        }
    }
    if (probes) *probes = steps;
    return false;
}

static bool chain_remove(ChainTable *t, const char *key)
{
    uint64_t h = t->hash_fn(key);
    size_t idx = (size_t)(h % t->n_buckets);

    /* The pointer-to-pointer trick again: no head special case. */
    for (ChainEntry **pp = &t->buckets[idx]; *pp != NULL; pp = &(*pp)->next) {
        if ((*pp)->hash == h && strcmp((*pp)->key, key) == 0) {
            ChainEntry *dead = *pp;
            *pp = dead->next;
            free(dead->key);
            free(dead);
            t->count--;
            return true;
        }
    }
    return false;
}

static void chain_stats(const ChainTable *t, const char *label)
{
    size_t used = 0, longest = 0;
    for (size_t i = 0; i < t->n_buckets; i++) {
        size_t len = 0;
        for (ChainEntry *e = t->buckets[i]; e != NULL; e = e->next) len++;
        if (len > 0) used++;
        if (len > longest) longest = len;
    }
    printf("  %-16s %zu entries in %zu buckets | load %.2f | %zu used (%.0f%%) | "
           "longest chain %zu | %zu collisions\n",
           label, t->count, t->n_buckets,
           (double)t->count / (double)t->n_buckets,
           used, 100.0 * (double)used / (double)t->n_buckets, longest, t->collisions);
}

/* ================================================================= *
 * STRATEGY 2: OPEN ADDRESSING (linear probing)
 *
 * No linked lists at all: on a collision, walk forward to the next free
 * slot. Everything lives in one contiguous array, which is dramatically
 * more cache-friendly — this is what most modern hash tables do.
 *
 * The catch is DELETION. Simply emptying a slot would break the probe
 * chain for every key that walked past it, so deleted slots get a
 * TOMBSTONE: "empty, but keep looking".
 * ================================================================= */
typedef enum { SLOT_EMPTY = 0, SLOT_FULL, SLOT_TOMB } SlotState;

typedef struct {
    char      *key;
    int        value;
    uint64_t   hash;
    SlotState  state;
} Slot;

typedef struct {
    Slot   *slots;
    size_t  n_slots;
    size_t  count;       /* live entries      */
    size_t  used;        /* live + tombstones */
} OpenTable;

static bool open_init(OpenTable *t, size_t n)
{
    t->slots = calloc(n, sizeof *t->slots);      /* state 0 == SLOT_EMPTY */
    if (t->slots == NULL) return false;
    t->n_slots = n; t->count = 0; t->used = 0;
    return true;
}
static void open_free(OpenTable *t)
{
    for (size_t i = 0; i < t->n_slots; i++)
        if (t->slots[i].state == SLOT_FULL) free(t->slots[i].key);
    free(t->slots);
    t->slots = NULL; t->n_slots = t->count = t->used = 0;
}

static bool open_put(OpenTable *t, const char *key, int value);

static bool open_resize(OpenTable *t, size_t new_n)
{
    OpenTable fresh;
    if (!open_init(&fresh, new_n)) return false;

    /* Reinsert every live entry. Tombstones are dropped — this is the only
     * thing that ever cleans them up. */
    for (size_t i = 0; i < t->n_slots; i++)
        if (t->slots[i].state == SLOT_FULL)
            if (!open_put(&fresh, t->slots[i].key, t->slots[i].value)) {
                open_free(&fresh); return false;
            }

    open_free(t);
    *t = fresh;
    return true;
}

static bool open_put(OpenTable *t, const char *key, int value)
{
    /* Resize on USED, not COUNT: tombstones occupy probe slots too, and a
     * table full of tombstones probes forever even with few live entries. */
    if ((double)(t->used + 1) / (double)t->n_slots > 0.7)
        if (!open_resize(t, t->n_slots * 2)) return false;

    uint64_t h = hash_fnv1a(key);
    size_t idx = (size_t)(h % t->n_slots);
    size_t first_tomb = SIZE_MAX;

    for (size_t i = 0; i < t->n_slots; i++) {
        Slot *s = &t->slots[idx];

        if (s->state == SLOT_EMPTY) {
            /* Reuse the first tombstone we walked past, if any. */
            size_t target = (first_tomb != SIZE_MAX) ? first_tomb : idx;
            Slot *dst = &t->slots[target];
            size_t klen = strlen(key) + 1;
            dst->key = malloc(klen);
            if (dst->key == NULL) return false;
            memcpy(dst->key, key, klen);
            dst->value = value;
            dst->hash  = h;
            if (dst->state != SLOT_TOMB) t->used++;   /* a tombstone was already counted */
            dst->state = SLOT_FULL;
            t->count++;
            return true;
        }
        if (s->state == SLOT_TOMB) {
            if (first_tomb == SIZE_MAX) first_tomb = idx;
        } else if (s->hash == h && strcmp(s->key, key) == 0) {
            s->value = value;                          /* update in place */
            return true;
        }
        idx = (idx + 1) % t->n_slots;                  /* LINEAR PROBE */
    }
    return false;                                      /* table completely full */
}

static bool open_get(const OpenTable *t, const char *key, int *out, size_t *probes)
{
    uint64_t h = hash_fnv1a(key);
    size_t idx = (size_t)(h % t->n_slots);

    for (size_t i = 0; i < t->n_slots; i++) {
        const Slot *s = &t->slots[idx];
        if (probes) *probes = i + 1;

        if (s->state == SLOT_EMPTY) return false;      /* a true gap: stop */
        /* A TOMBSTONE does NOT stop the search — that is its entire purpose. */
        if (s->state == SLOT_FULL && s->hash == h && strcmp(s->key, key) == 0) {
            if (out) *out = s->value;
            return true;
        }
        idx = (idx + 1) % t->n_slots;
    }
    return false;
}

static bool open_remove(OpenTable *t, const char *key)
{
    uint64_t h = hash_fnv1a(key);
    size_t idx = (size_t)(h % t->n_slots);

    for (size_t i = 0; i < t->n_slots; i++) {
        Slot *s = &t->slots[idx];
        if (s->state == SLOT_EMPTY) return false;
        if (s->state == SLOT_FULL && s->hash == h && strcmp(s->key, key) == 0) {
            free(s->key);
            s->key = NULL;
            s->state = SLOT_TOMB;      /* NOT EMPTY — leave the chain intact */
            t->count--;                /* `used` stays: the slot is still taken */
            return true;
        }
        idx = (idx + 1) % t->n_slots;
    }
    return false;
}

static double seconds_since(clock_t t) { return (double)(clock() - t) / CLOCKS_PER_SEC; }

int main(void)
{
    puts("=== WHAT A HASH TABLE IS ===");
    puts("  An array, plus a function that turns a key into an index:");
    puts("      index = hash(key) % n_buckets");
    puts("  Everything difficult about it is what happens when two different");
    puts("  keys produce the same index. That is a COLLISION, and by the");
    puts("  pigeonhole principle it is unavoidable.\n");

    puts("=== HASH FUNCTIONS ===");
    {
        const char *keys[] = {"apple", "apply", "banana", "a", "b"};
        printf("  %-8s %-22s %-22s %s\n", "key", "FNV-1a", "djb2", "terrible");
        for (size_t i = 0; i < 5; i++)
            printf("  %-8s %-22llu %-22llu %llu\n", keys[i],
                   (unsigned long long)hash_fnv1a(keys[i]),
                   (unsigned long long)hash_djb2(keys[i]),
                   (unsigned long long)hash_terrible(keys[i]));
        puts("  Note \"apple\" and \"apply\" — one byte apart — produce completely");
        puts("  different FNV-1a and djb2 values. That is the AVALANCHE property,");
        puts("  and it is what makes a hash function good.");
        printf("  The terrible hash just sums bytes, so every anagram collides:\n");
        printf("    hash_terrible(\"abc\") = %llu, (\"cba\") = %llu\n",
               (unsigned long long)hash_terrible("abc"),
               (unsigned long long)hash_terrible("cba"));
    }

    puts("\n=== SEPARATE CHAINING ===");
    {
        ChainTable t;
        chain_init(&t, 8, hash_fnv1a);

        const char *fruits[] = {"apple","banana","cherry","date","elderberry",
                                "fig","grape","honeydew","kiwi","lemon"};
        for (int i = 0; i < 10; i++) chain_put(&t, fruits[i], (i + 1) * 100);

        chain_stats(&t, "after 10 puts");
        printf("  (it resized from 8 buckets automatically as the load factor rose)\n");

        int v; size_t probes;
        chain_get(&t, "cherry", &v, &probes);
        printf("  get(\"cherry\") = %d in %zu probe(s)\n", v, probes);
        printf("  get(\"missing\") -> %s\n",
               chain_get(&t, "missing", &v, &probes) ? "found" : "not found");

        chain_put(&t, "apple", 999);
        chain_get(&t, "apple", &v, NULL);
        printf("  put(\"apple\", 999) then get -> %d (updated, not duplicated)\n", v);
        printf("  count is still %zu\n", t.count);

        chain_remove(&t, "banana");
        printf("  after remove(\"banana\"): count=%zu, get -> %s\n",
               t.count, chain_get(&t, "banana", &v, NULL) ? "found" : "not found");

        chain_free(&t);
    }

    puts("\n=== WHY THE HASH FUNCTION MATTERS ===");
    {
        ChainTable good, bad;
        chain_init(&good, 64, hash_fnv1a);
        chain_init(&bad,  64, hash_terrible);

        char key[16];
        for (int i = 0; i < 200; i++) {
            snprintf(key, sizeof key, "key%d", i);
            chain_put(&good, key, i);
            chain_put(&bad,  key, i);
        }
        chain_stats(&good, "FNV-1a");
        chain_stats(&bad,  "terrible");

        /* Measure the actual lookup cost. */
        size_t total_good = 0, total_bad = 0, p;
        for (int i = 0; i < 200; i++) {
            snprintf(key, sizeof key, "key%d", i);
            chain_get(&good, key, NULL, &p); total_good += p;
            chain_get(&bad,  key, NULL, &p); total_bad  += p;
        }
        printf("  average probes per lookup: FNV-1a %.2f, terrible %.2f\n",
               (double)total_good / 200.0, (double)total_bad / 200.0);
        puts("  A bad hash turns O(1) into O(n): every key ends up in one bucket");
        puts("  and the table degenerates into a linked list.");
        puts("  This is also a DENIAL OF SERVICE vector — if an attacker can");
        puts("  choose your keys and knows your hash, they can force worst-case");
        puts("  behaviour. Real systems use a SEEDED hash (SipHash) for");
        puts("  attacker-controlled input.");

        chain_free(&good); chain_free(&bad);
    }

    puts("\n=== OPEN ADDRESSING (linear probing) ===");
    {
        OpenTable t;
        open_init(&t, 16);

        const char *words[] = {"alpha","bravo","charlie","delta","echo",
                               "foxtrot","golf","hotel"};
        for (int i = 0; i < 8; i++) open_put(&t, words[i], i * 10);

        printf("  %zu entries in %zu slots (load %.2f)\n",
               t.count, t.n_slots, (double)t.count / (double)t.n_slots);

        int v; size_t probes;
        for (int i = 0; i < 4; i++) {
            open_get(&t, words[i], &v, &probes);
            printf("  get(\"%-8s\") = %2d in %zu probe(s)\n", words[i], v, probes);
        }

        puts("\n  THE DELETION PROBLEM:");
        printf("    before: count=%zu used=%zu\n", t.count, t.used);
        open_remove(&t, words[2]);
        printf("    after remove(\"%s\"): count=%zu used=%zu  <- used did NOT drop\n",
               words[2], t.count, t.used);
        puts("    The slot became a TOMBSTONE, not EMPTY.");
        puts("    If it were emptied, a lookup for any key that had probed PAST");
        puts("    it would stop at the gap and wrongly report 'not found'.");
        puts("    The tombstone says: 'nothing here, but keep looking'.");

        /* Prove the probe chain still works. */
        bool still_findable = true;
        for (int i = 0; i < 8; i++)
            if (i != 2 && !open_get(&t, words[i], &v, NULL)) still_findable = false;
        printf("    every other key still findable: %s\n",
               still_findable ? "yes — the chain survived" : "NO");
        printf("    the removed key: %s\n",
               open_get(&t, words[2], &v, NULL) ? "still found?!" : "correctly gone");

        puts("    Tombstones accumulate, so a resize is what finally clears them —");
        puts("    which is why the load factor is computed on `used`, not `count`.");

        open_free(&t);
    }

    puts("\n=== CHAINING vs OPEN ADDRESSING ===");
    {
        const int N = 200000;
        clock_t t;
        char key[24];

        ChainTable c; chain_init(&c, 16, hash_fnv1a);
        OpenTable  o; open_init(&o, 16);

        t = clock();
        for (int i = 0; i < N; i++) { snprintf(key, sizeof key, "key%d", i); chain_put(&c, key, i); }
        double t_cput = seconds_since(t);

        t = clock();
        for (int i = 0; i < N; i++) { snprintf(key, sizeof key, "key%d", i); open_put(&o, key, i); }
        double t_oput = seconds_since(t);

        t = clock();
        long long sum1 = 0;
        for (int i = 0; i < N; i++) {
            snprintf(key, sizeof key, "key%d", i);
            int v; if (chain_get(&c, key, &v, NULL)) sum1 += v;
        }
        double t_cget = seconds_since(t);

        t = clock();
        long long sum2 = 0;
        for (int i = 0; i < N; i++) {
            snprintf(key, sizeof key, "key%d", i);
            int v; if (open_get(&o, key, &v, NULL)) sum2 += v;
        }
        double t_oget = seconds_since(t);

        printf("  %d insert + %d lookup (checksums match: %s)\n",
               N, N, sum1 == sum2 ? "yes" : "no");
        printf("    chaining       : %.4f s insert, %.4f s lookup\n", t_cput, t_cget);
        printf("    open addressing: %.4f s insert, %.4f s lookup\n", t_oput, t_oget);
        printf("    memory: chaining %zu entries x %zu bytes + %zu buckets\n",
               c.count, sizeof(ChainEntry), c.n_buckets);
        printf("            open     %zu slots x %zu bytes, contiguous\n",
               o.n_slots, sizeof(Slot));

        chain_free(&c); open_free(&o);

        puts("");
        puts("  CHAINING            OPEN ADDRESSING");
        puts("  simple to implement  more subtle (tombstones)");
        puts("  handles load > 1     must stay under ~0.7 load");
        puts("  one malloc per entry ONE contiguous array");
        puts("  pointer chasing      CACHE FRIENDLY — the big win");
        puts("  easy deletion        deletion needs tombstones");
        puts("  stable pointers      entries MOVE on resize");
        puts("");
        puts("  Modern tables mostly use open addressing for the cache behaviour.");
        puts("  Better probe strategies than linear: quadratic probing, double");
        puts("  hashing, and Robin Hood hashing (steal from the rich: keep probe");
        puts("  distances even). Google's SwissTable adds SIMD to scan 16 slots");
        puts("  of metadata at once.");
    }

    puts("\n=== THE RULES ===");
    puts("  1. Keep the load factor under 0.75 (chaining) or 0.7 (open).");
    puts("  2. Resize by DOUBLING, and rehash everything.");
    puts("  3. Cache the hash in the entry: resize becomes a re-mask, and");
    puts("     lookups skip strcmp when the hashes differ.");
    puts("  4. Decide who owns the keys. Here the table copies them.");
    puts("  5. A resize INVALIDATES every pointer into the table.");
    puts("  6. Use a seeded hash for attacker-controlled keys.");
    puts("  7. Prefer a power-of-two bucket count and `& (n-1)` over `% n` —");
    puts("     a modulo is a division, which is ~20-40 cycles.");

    return 0;
}
