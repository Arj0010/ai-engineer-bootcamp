/* 03_safe_strings.c — how to handle strings without creating vulnerabilities.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_safe_strings.c -o t && ./t
 *
 * Run the overflow section under AddressSanitizer to see it caught:
 *   gcc -g -O0 -fsanitize=address 03_safe_strings.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

/* ================================================================= *
 * A portable strlcpy: bounded copy that ALWAYS terminates.
 * Returns the length of src — so `if (ret >= dstsize) truncated`.
 * (strlcpy is BSD/POSIX-2024, not ISO C, so here it is by hand.)
 * ================================================================= */
static size_t safe_copy(char *dst, size_t dstsize, const char *src)
{
    size_t srclen = strlen(src);
    if (dstsize > 0) {
        size_t copy = (srclen < dstsize - 1) ? srclen : dstsize - 1;
        memcpy(dst, src, copy);
        dst[copy] = '\0';                    /* ALWAYS terminated */
    }
    return srclen;                           /* what it WOULD have taken */
}

/* strnlen is POSIX, not ISO C, so under -std=c17 it is not declared.
 * It is two lines; write it rather than reaching for _GNU_SOURCE. */
static size_t bounded_len(const char *s, size_t maxlen)
{
    size_t n = 0;
    while (n < maxlen && s[n] != '\0') n++;
    return n;                       /* == maxlen means "no terminator found" */
}

/* Bounded append. Returns the total length it wanted to produce. */
static size_t safe_append(char *dst, size_t dstsize, const char *src)
{
    size_t dlen = bounded_len(dst, dstsize);
    if (dlen == dstsize) return dstsize + strlen(src);   /* dst not terminated */
    return dlen + safe_copy(dst + dlen, dstsize - dlen, src);
}

/* ================================================================= *
 * A string builder: append repeatedly into a fixed buffer, in O(total)
 * rather than O(n^2), and never overflow. This is the pattern to reach
 * for instead of repeated strcat.
 * ================================================================= */
typedef struct {
    char  *buf;
    size_t cap;         /* total buffer size, including room for the NUL */
    size_t len;         /* current length, excluding the NUL             */
    bool   truncated;   /* sticky: set once anything did not fit         */
} Builder;

static void sb_init(Builder *b, char *storage, size_t cap)
{
    b->buf = storage; b->cap = cap; b->len = 0; b->truncated = false;
    if (cap > 0) storage[0] = '\0';
}

/* printf-style append. Note vsnprintf, not snprintf — you cannot forward
 * a `...` directly; you must pass the va_list along. */
static void sb_addf(Builder *b, const char *fmt, ...)
{
    if (b->cap == 0 || b->len >= b->cap - 1) { b->truncated = true; return; }

    size_t space = b->cap - b->len;          /* includes room for the NUL */
    va_list ap;
    va_start(ap, fmt);
    int want = vsnprintf(b->buf + b->len, space, fmt, ap);
    va_end(ap);

    if (want < 0) { b->truncated = true; return; }        /* encoding error */
    if ((size_t)want >= space) {                          /* did not fit */
        b->len = b->cap - 1;                              /* buffer is full */
        b->truncated = true;
    } else {
        b->len += (size_t)want;
    }
}

static void sb_add(Builder *b, const char *s) { sb_addf(b, "%s", s); }

/* ================================================================= *
 * Splitting a string WITHOUT strtok: non-destructive, reentrant, and it
 * preserves empty fields (which strtok silently eats).
 * ================================================================= */
typedef struct { const char *start; size_t len; } Field;

static size_t split(const char *s, char delim, Field *out, size_t max_fields)
{
    size_t n = 0;
    const char *field_start = s;
    for (const char *p = s; ; p++) {
        if (*p == delim || *p == '\0') {
            if (n < max_fields) {
                out[n].start = field_start;
                out[n].len   = (size_t)(p - field_start);
            }
            n++;
            if (*p == '\0') break;
            field_start = p + 1;
        }
    }
    return n;                                /* total fields, even beyond max */
}

int main(void)
{
    puts("=== THE VULNERABILITY ===");
    puts("      char buf[8];");
    puts("      strcpy(buf, user_input);     /* input longer than 7 -> overflow */");
    puts("  This writes past buf into whatever the compiler put next on the");
    puts("  stack: other locals, the saved frame pointer, the RETURN ADDRESS.");
    puts("  Overwriting the return address is how classic exploits redirect");
    puts("  execution. Every unbounded copy is a potential one of these.\n");
    puts("  The same shape appears as: strcat, sprintf, gets (removed from C11");
    puts("  entirely because it could not be used safely), scanf(\"%s\").\n");

    puts("=== FIX 1: snprintf — always terminates, reports truncation ===");
    {
        char dst[8];
        const char *inputs[] = { "short", "exactly7", "far too long for this" };
        for (size_t i = 0; i < 3; i++) {
            int want = snprintf(dst, sizeof dst, "%s", inputs[i]);
            bool truncated = (want < 0) || ((size_t)want >= sizeof dst);
            printf("  \"%-22s\" -> \"%-7s\"  wanted %2d bytes  %s\n",
                   inputs[i], dst, want, truncated ? "TRUNCATED" : "ok");
        }
        puts("  snprintf returns what it WOULD have written. Compare against the");
        puts("  buffer size to detect truncation. Silently truncating is still a");
        puts("  bug — it is just a much less dangerous one than overflowing.");
    }

    puts("\n=== FIX 2: a bounded copy that always terminates ===");
    {
        char dst[8];
        size_t need = safe_copy(dst, sizeof dst, "abcdefghijklmnop");
        printf("  safe_copy -> \"%s\" (needed %zu, had %zu) %s\n",
               dst, need, sizeof dst, need >= sizeof dst ? "TRUNCATED" : "ok");

        char acc[16] = "";
        safe_append(acc, sizeof acc, "one ");
        safe_append(acc, sizeof acc, "two ");
        size_t total = safe_append(acc, sizeof acc, "three four five");
        printf("  safe_append -> \"%s\" (wanted %zu bytes)\n", acc, total);
    }

    puts("\n=== FIX 3: a string builder (O(n), not O(n^2)) ===");
    {
        char storage[80];
        Builder b;
        sb_init(&b, storage, sizeof storage);

        sb_add(&b, "values:");
        for (int i = 1; i <= 8; i++) sb_addf(&b, " %d", i * i);
        sb_addf(&b, " | mean=%.2f", 25.5);

        printf("  \"%s\"\n", b.buf);
        printf("  length %zu of %zu, truncated: %s\n",
               b.len, b.cap, b.truncated ? "YES" : "no");

        /* Now overflow it deliberately and watch it stay safe. */
        char tiny[16];
        Builder t;
        sb_init(&t, tiny, sizeof tiny);
        for (int i = 0; i < 20; i++) sb_addf(&t, "%d,", i);
        printf("  overflowing builder -> \"%s\" (truncated: %s, len %zu)\n",
               t.buf, t.truncated ? "YES" : "no", t.len);
        puts("  No overflow, no crash, and the caller can SEE it was truncated.");
        puts("  Compare with strcat, which would have written past `tiny`.");
    }

    puts("\n=== FIX 4: splitting without destroying the input ===");
    {
        const char *csv = "alice,30,,engineer,london";
        Field fields[8];
        size_t n = split(csv, ',', fields, 8);

        printf("  input: \"%s\"  (unchanged afterwards)\n", csv);
        printf("  %zu fields:\n", n);
        for (size_t i = 0; i < n && i < 8; i++)
            printf("    [%zu] \"%.*s\"%s\n", i, (int)fields[i].len, fields[i].start,
                   fields[i].len == 0 ? "   <- EMPTY, correctly preserved" : "");
        printf("  input after split: \"%s\"  <- untouched\n", csv);
        puts("  strtok would have collapsed the empty field and overwritten the");
        puts("  commas with NULs. For CSV, that is a data-corruption bug.");
    }

    puts("\n=== READING INPUT SAFELY ===");
    puts("  NEVER:  gets(buf)              — removed from C11, unusable safely");
    puts("  NEVER:  scanf(\"%s\", buf)       — unbounded");
    puts("  BAD:    scanf(\"%31s\", buf)     — bounded, but the count must match");
    puts("                                    sizeof buf - 1 by hand, forever");
    puts("  GOOD:   fgets(buf, sizeof buf, stdin)");
    puts("            buf[strcspn(buf, \"\\n\")] = '\\0';");
    puts("          then parse with strtol / sscanf / your own splitter.");
    puts("  fgets bounds itself with sizeof, so the buffer and the limit can");
    puts("  never drift apart.");

    puts("\n=== FORMAT STRING VULNERABILITIES ===");
    puts("  printf(user_input);              <- NEVER. If input contains %n,");
    puts("                                      it WRITES to memory. If it");
    puts("                                      contains %s, it reads arbitrary");
    puts("                                      addresses.");
    puts("  printf(\"%s\", user_input);        <- correct. The format string must");
    puts("                                      always be a literal you wrote.");

    puts("\n=== THE RULES ===");
    puts("  1. A buffer pointer and its size are ONE argument pair. Never split them.");
    puts("  2. Use sizeof buf (not a hard-coded number) wherever the array is in scope.");
    puts("  3. Prefer snprintf; check its return value.");
    puts("  4. Assume every input is longer than you expect.");
    puts("  5. Develop under -fsanitize=address. It finds these in seconds.");
    puts("  6. Run valgrind before you ship.");

    return 0;
}
