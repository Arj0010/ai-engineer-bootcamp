/* 08_ownership_patterns.c — the discipline that replaces a garbage collector.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 08_ownership_patterns.c -o t && ./t
 *   valgrind --leak-check=full ./t          # zero leaks on EVERY path, by design
 *
 * C cannot express "who frees this" in the type system. So you express it in
 * conventions, and you apply them without exception. These five patterns
 * cover essentially all real C code.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

/* ================================================================= *
 * PATTERN 1: CREATE / DESTROY PAIRS
 *
 * The allocator returns an opaque handle; a matching destroy() releases
 * everything it owns. The caller's only obligation is to call destroy
 * exactly once, on every path.
 * ================================================================= */
typedef struct {
    char   *name;        /* OWNED — destroy() frees it */
    int    *scores;      /* OWNED */
    size_t  n_scores;
} Student;

/* Returns a new Student, or NULL. CALLER MUST call student_destroy(). */
static Student *student_create(const char *name, const int *scores, size_t n)
{
    Student *s = calloc(1, sizeof *s);     /* calloc: every pointer starts NULL,
                                            * which makes the cleanup path safe */
    if (s == NULL) return NULL;

    size_t name_len = strlen(name) + 1;
    s->name = malloc(name_len);
    if (s->name == NULL) goto fail;        /* free(NULL) later is a no-op */
    memcpy(s->name, name, name_len);

    if (n > 0) {
        s->scores = malloc(n * sizeof *s->scores);
        if (s->scores == NULL) goto fail;
        memcpy(s->scores, scores, n * sizeof *s->scores);
    }
    s->n_scores = n;
    return s;

fail:
    /* ONE cleanup path that handles every partial state, because calloc
     * guaranteed the un-set pointers are NULL and free(NULL) is defined. */
    free(s->scores);
    free(s->name);
    free(s);
    return NULL;
}

static void student_destroy(Student *s)
{
    if (s == NULL) return;                 /* destroy(NULL) must be a no-op,
                                            * matching free()'s contract */
    free(s->scores);                       /* free what we own... */
    free(s->name);
    free(s);                               /* ...then ourselves */
}

/* ================================================================= *
 * PATTERN 2: INIT / DEINIT ON CALLER-PROVIDED STORAGE
 *
 * The caller decides where the object lives — stack, arena, inside another
 * struct. We only manage the resources it acquires. Strictly better than
 * create/destroy when the object does not need to outlive a scope.
 * ================================================================= */
typedef struct { int *data; size_t len, cap; } IntVec;

static bool vec_init(IntVec *v, size_t initial_cap)
{
    v->data = (initial_cap > 0) ? malloc(initial_cap * sizeof *v->data) : NULL;
    if (initial_cap > 0 && v->data == NULL) return false;
    v->len = 0; v->cap = initial_cap;
    return true;
}
static void vec_deinit(IntVec *v)
{
    free(v->data);
    v->data = NULL; v->len = v->cap = 0;   /* leave it in a safe, reusable state */
}
static bool vec_push(IntVec *v, int value)
{
    if (v->len == v->cap) {
        size_t cap = v->cap ? v->cap * 2 : 4;
        int *tmp = realloc(v->data, cap * sizeof *tmp);
        if (tmp == NULL) return false;
        v->data = tmp; v->cap = cap;
    }
    v->data[v->len++] = value;
    return true;
}

/* ================================================================= *
 * PATTERN 3: BORROWED vs OWNED RETURNS
 *
 * The single most important thing to document. A caller who frees a
 * borrowed pointer corrupts your data structure; a caller who fails to
 * free an owned one leaks.
 * ================================================================= */
typedef struct { const char *key; int value; } Entry;
typedef struct { Entry entries[8]; size_t n; } Table;

/* BORROWED. Points INTO the table. Do NOT free. Invalid after the table changes. */
static const Entry *table_find(const Table *t, const char *key)
{
    for (size_t i = 0; i < t->n; i++)
        if (strcmp(t->entries[i].key, key) == 0) return &t->entries[i];
    return NULL;
}

/* OWNED. A fresh allocation. CALLER MUST free(). */
static char *table_describe(const Table *t)
{
    size_t cap = 256;
    char *out = malloc(cap);
    if (out == NULL) return NULL;
    size_t off = 0;
    for (size_t i = 0; i < t->n && off < cap; i++) {
        int w = snprintf(out + off, cap - off, "%s=%d ", t->entries[i].key,
                         t->entries[i].value);
        if (w < 0 || (size_t)w >= cap - off) break;
        off += (size_t)w;
    }
    return out;
}

/* ================================================================= *
 * PATTERN 4: CALLER-PROVIDED BUFFER — the best option when it applies
 *
 * Nothing is allocated, so nothing can leak. The caller controls the
 * lifetime completely. snprintf, fgets, and read() all work this way.
 * Return the length NEEDED so the caller can detect truncation.
 * ================================================================= */
static int format_student(const Student *s, char *out, size_t outsize)
{
    if (s == NULL || out == NULL || outsize == 0) return -1;
    int total = 0, w;

    w = snprintf(out, outsize, "%s: ", s->name);
    if (w < 0) return -1;
    total = w;

    for (size_t i = 0; i < s->n_scores; i++) {
        size_t remaining = ((size_t)total < outsize) ? outsize - (size_t)total : 0;
        w = snprintf(out + (remaining ? (size_t)total : outsize - 1), remaining,
                     "%d%s", s->scores[i], i + 1 < s->n_scores ? "," : "");
        if (w < 0) return -1;
        total += w;
    }
    return total;                 /* >= outsize means it was TRUNCATED */
}

/* ================================================================= *
 * PATTERN 5: TRANSFER OF OWNERSHIP
 *
 * Make it explicit in the name and the signature. A function that TAKES
 * ownership should null the caller's pointer so the caller cannot use it
 * again — which is why it takes T**.
 * ================================================================= */
typedef struct { Student *items[4]; size_t n; } Roster;

/* CONSUMES *s: the roster now owns it, and *s is set to NULL. */
static bool roster_adopt(Roster *r, Student **s)
{
    if (r->n >= 4) return false;              /* refuse, caller keeps ownership */
    r->items[r->n++] = *s;
    *s = NULL;                                /* the caller can no longer touch it */
    return true;
}
static void roster_destroy(Roster *r)
{
    for (size_t i = 0; i < r->n; i++) student_destroy(r->items[i]);
    r->n = 0;
}

/* ================================================================= *
 * THE ERROR PATH: goto cleanup. C has no destructors and no `finally`.
 * ================================================================= */
static int process_pipeline(bool fail_at_step_2, bool fail_at_step_3)
{
    int      rc     = -1;
    Student *a      = NULL;
    Student *b      = NULL;
    char    *report = NULL;
    FILE    *out    = NULL;

    int scores[] = {90, 85, 78};

    a = student_create("alice", scores, 3);
    if (a == NULL) goto cleanup;

    if (fail_at_step_2) { fprintf(stderr, "    (failing at step 2)\n"); goto cleanup; }

    b = student_create("bob", scores, 2);
    if (b == NULL) goto cleanup;

    report = malloc(128);
    if (report == NULL) goto cleanup;
    snprintf(report, 128, "%s and %s", a->name, b->name);

    if (fail_at_step_3) { fprintf(stderr, "    (failing at step 3)\n"); goto cleanup; }

    out = fopen("/dev/null", "w");
    if (out == NULL) goto cleanup;
    fprintf(out, "%s\n", report);

    rc = 0;                                   /* success */

cleanup:
    /* ONE exit path. Release in REVERSE order of acquisition. Every branch
     * above lands here, so nothing can leak regardless of where we failed. */
    if (out != NULL) fclose(out);
    free(report);
    student_destroy(b);
    student_destroy(a);
    return rc;
}

int main(void)
{
    puts("=== THE RULE THAT REPLACES A GARBAGE COLLECTOR ===");
    puts("  Every allocation has EXACTLY ONE OWNER.");
    puts("  The owner frees it.");
    puts("  Ownership is documented in the header, next to the function.");
    puts("  If you cannot say who owns a pointer, you have a bug you have not");
    puts("  found yet.\n");

    puts("=== PATTERN 1: create / destroy ===");
    {
        int scores[] = {95, 88, 76, 92};
        Student *s = student_create("alice", scores, 4);
        if (s != NULL) {
            printf("  created \"%s\" with %zu scores\n", s->name, s->n_scores);
            student_destroy(s);
            s = NULL;                        /* so a later use crashes loudly */
            puts("  destroyed — it freed name, scores, and the struct itself");
        }
        student_destroy(NULL);
        puts("  destroy(NULL) is a no-op, matching free()'s own contract.");
        puts("  That property is what lets the goto-cleanup pattern work.");
        puts("\n  Note student_create uses CALLOC so every pointer member starts");
        puts("  NULL. Its single `fail:` label can then free all of them blindly,");
        puts("  no matter how far construction got.");
    }

    puts("\n=== PATTERN 2: init / deinit on caller storage ===");
    {
        IntVec v;                            /* ON THE STACK — we chose where */
        if (vec_init(&v, 4)) {
            for (int i = 1; i <= 10; i++) vec_push(&v, i * i);
            printf("  vec: ");
            for (size_t i = 0; i < v.len; i++) printf("%d ", v.data[i]);
            printf("(len %zu, cap %zu)\n", v.len, v.cap);
            vec_deinit(&v);
            printf("  after deinit: data=%p len=%zu cap=%zu\n",
                   (void *)v.data, v.len, v.cap);
        }
        puts("  The struct itself was never malloc'd — only its buffer was.");
        puts("  Prefer this to create/destroy whenever the object does not need");
        puts("  to outlive the scope: one less allocation, one less failure mode.");
    }

    puts("\n=== PATTERN 3: borrowed vs owned returns ===");
    {
        Table t = { { {"alpha", 1}, {"beta", 2}, {"gamma", 3} }, 3 };

        const Entry *found = table_find(&t, "beta");     /* BORROWED */
        printf("  table_find(\"beta\") -> %s=%d\n", found->key, found->value);
        puts("    BORROWED: points into the table. Do NOT free it. It becomes");
        puts("    invalid the moment the table is modified or destroyed.");

        char *desc = table_describe(&t);                 /* OWNED */
        printf("  table_describe()  -> \"%s\"\n", desc);
        puts("    OWNED: a fresh allocation. The caller must free it.");
        free(desc);

        puts("\n  Say which one it is IN THE HEADER, always:");
        puts("      /* Returns a pointer INTO the table. Do not free. */");
        puts("      const Entry *table_find(const Table *t, const char *key);");
        puts("      /* Returns a new string. CALLER MUST free(). */");
        puts("      char *table_describe(const Table *t);");
        puts("  Returning `const T *` is a useful hint that it is borrowed —");
        puts("  free() takes a non-const pointer, so the compiler pushes back.");
    }

    puts("\n=== PATTERN 4: caller-provided buffer (nothing to leak) ===");
    {
        int scores[] = {100, 95, 90};
        Student *s = student_create("carol", scores, 3);
        if (s != NULL) {
            char buf[64];
            int need = format_student(s, buf, sizeof buf);
            printf("  format_student -> \"%s\" (needed %d bytes)\n", buf, need);

            char tiny[12];
            need = format_student(s, tiny, sizeof tiny);
            printf("  into a %zu-byte buffer -> \"%s\" (needed %d) %s\n",
                   sizeof tiny, tiny, need,
                   (need >= (int)sizeof tiny) ? "TRUNCATED" : "");
            student_destroy(s);
        }
        puts("  Nothing was allocated, so nothing can leak. The caller picks the");
        puts("  storage — stack, arena, or a reused buffer in a loop.");
        puts("  This is how snprintf, fgets, read, and recv all work.");
        puts("  PREFER THIS. Return the length NEEDED so truncation is detectable.");
    }

    puts("\n=== PATTERN 5: explicit transfer of ownership ===");
    {
        Roster r = {0};
        int scores[] = {80, 85};

        for (int i = 0; i < 3; i++) {
            char name[16];
            snprintf(name, sizeof name, "student%d", i);
            Student *s = student_create(name, scores, 2);
            if (s == NULL) continue;

            if (roster_adopt(&r, &s)) {
                printf("  adopted %s; caller's pointer is now %p\n",
                       r.items[r.n - 1]->name, (void *)s);
            } else {
                student_destroy(s);          /* refused: we still own it */
            }
        }
        printf("  roster holds %zu students\n", r.n);
        roster_destroy(&r);
        puts("  roster_adopt takes Student** and NULLs the caller's pointer, so");
        puts("  the caller physically cannot use or free it afterwards.");
        puts("  If it REFUSES, ownership stays with the caller — and the caller");
        puts("  must handle that. Make the contract obvious in the signature.");
    }

    puts("\n=== THE ERROR PATH: goto cleanup ===");
    {
        printf("  success path       : rc = %d\n", process_pipeline(false, false));
        printf("  fail at step 2     : rc = %d\n", process_pipeline(true, false));
        printf("  fail at step 3     : rc = %d\n", process_pipeline(false, true));
        puts("  All three paths free everything they acquired. Run this under");
        puts("  valgrind: zero leaks on every branch.");
        puts("");
        puts("  The rules that make it safe:");
        puts("    1. Declare and initialise every resource to NULL up front.");
        puts("    2. Jump FORWARD to a single cleanup label. Never backward,");
        puts("       and never INTO a scope.");
        puts("    3. Release in REVERSE order of acquisition.");
        puts("    4. Rely on free(NULL) and destroy(NULL) being no-ops, so the");
        puts("       cleanup block needs no per-resource if-guards.");
        puts("    5. Set the success rc only at the very end.");
        puts("  This is the dominant idiom in the Linux kernel for exactly these");
        puts("  reasons. It is not a code smell in C; it is the tool for the job.");
    }

    puts("\n=== NAMING CONVENTIONS THAT CARRY THE CONTRACT ===");
    puts("  X_create / X_destroy   allocates the object itself");
    puts("  X_init   / X_deinit    caller supplies the storage");
    puts("  X_acquire/ X_release   reference counted (module 05, file 07)");
    puts("  X_take / X_adopt       consumes ownership (takes T**, NULLs it)");
    puts("  X_borrow / X_peek      returns a non-owning pointer");
    puts("  const T *              a strong hint: borrowed, do not free");
    puts("");
    puts("  Be consistent across the whole codebase. A reader should be able to");
    puts("  tell the ownership of any pointer from its function's NAME.");

    puts("\n=== THE HIERARCHY: prefer the earliest option that works ===");
    puts("  1. Automatic (stack) storage      — free, impossible to leak");
    puts("  2. Caller-provided buffer         — no allocation at all");
    puts("  3. Single ownership + create/destroy — covers most real C");
    puts("  4. Arena: everything dies together   — file 04");
    puts("  5. Pool: many objects of one size    — file 06");
    puts("  6. Reference counting                — genuinely shared lifetimes, file 07");
    puts("  Every step down adds a way to get it wrong. Do not skip ahead.");

    return 0;
}
