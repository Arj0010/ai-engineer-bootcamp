/* 01_text_io.c — reading and writing text files without the classic bugs.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_text_io.c -o t && ./t
 *
 * Creates and deletes its own temporary files, so it is safe to run anywhere.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

#define TMPFILE "07_text_io_demo.txt"

/* ---------------------------------------------------------------- *
 * A word-frequency counter: the standard "real" text-processing task.
 * ---------------------------------------------------------------- */
#define MAX_WORDS 256
typedef struct { char word[32]; int count; } WordCount;

static int wc_find_or_add(WordCount *table, int *n, const char *word)
{
    for (int i = 0; i < *n; i++)
        if (strcmp(table[i].word, word) == 0) return i;
    if (*n >= MAX_WORDS) return -1;
    snprintf(table[*n].word, sizeof table[*n].word, "%s", word);
    table[*n].count = 0;
    return (*n)++;
}
static int wc_cmp(const void *a, const void *b)
{
    const WordCount *x = a, *y = b;
    if (x->count != y->count) return y->count - x->count;   /* count descending */
    return strcmp(x->word, y->word);                        /* then alphabetical */
}

static int write_sample_file(void)
{
    FILE *f = fopen(TMPFILE, "w");
    if (f == NULL) { perror("fopen for writing"); return -1; }

    fprintf(f, "the quick brown fox\n");
    fprintf(f, "jumps over the lazy dog\n");
    fprintf(f, "the dog barks and the fox runs\n");
    fprintf(f, "\n");                            /* an empty line, on purpose */
    fprintf(f, "a line with    irregular   spacing\n");
    fprintf(f, "line without a trailing newline");   /* also on purpose */

    if (fclose(f) != 0) { perror("fclose"); return -1; }
    return 0;
}

int main(void)
{
    if (write_sample_file() != 0) return 1;

    puts("=== OPENING A FILE ===");
    {
        FILE *f = fopen(TMPFILE, "r");
        if (f == NULL) { perror("fopen"); return 1; }
        printf("  opened \"%s\" successfully\n", TMPFILE);
        fclose(f);

        /* The failure path is not optional. errno tells you WHY. */
        errno = 0;
        FILE *missing = fopen("this-file-does-not-exist.txt", "r");
        if (missing == NULL)
            printf("  opening a missing file -> NULL, errno=%d (%s)\n",
                   errno, strerror(errno));
        else fclose(missing);

        puts("  Modes: r (must exist)  w (TRUNCATES or creates)  a (append)");
        puts("         r+ w+ a+ add the other direction");
        puts("         add 'b' for binary: no-op on POSIX, ESSENTIAL on Windows");
        puts("  \"w\" destroys the file's contents the instant it succeeds.");
    }

    puts("\n=== READING LINE BY LINE (the correct loop) ===");
    {
        FILE *f = fopen(TMPFILE, "r");
        if (f == NULL) return 1;

        char line[256];
        int n = 0;
        /* Test the RETURN VALUE of the read. This is the correct idiom. */
        while (fgets(line, sizeof line, f) != NULL) {
            size_t len = strcspn(line, "\n");
            bool had_newline = (line[len] == '\n');
            line[len] = '\0';                       /* strip it */
            printf("  line %d (%2zu chars)%s: \"%s\"\n",
                   ++n, len, had_newline ? "" : " [no trailing newline]", line);
        }

        /* AFTER the loop, distinguish a clean EOF from a real error. */
        if (ferror(f))     puts("  a read ERROR occurred");
        else if (feof(f))  puts("  reached end of file cleanly");
        fclose(f);
    }

    puts("\n=== WHY `while (!feof(f))` IS WRONG ===");
    {
        FILE *f = fopen(TMPFILE, "r");
        char line[256];
        int n = 0;

        /* feof only becomes TRUE AFTER a read has already failed. So the body
         * runs one extra time with whatever `line` held from last iteration. */
        while (!feof(f)) {
            /* The bug IS ignoring this return value. The (void) cast only
             * silences -Wunused-result so the file builds clean; the logic
             * error it demonstrates is entirely intact. */
            (void)!fgets(line, sizeof line, f);
            n++;
        }
        fclose(f);
        printf("  the !feof loop ran %d times for a 6-line file\n", n);
        puts("  The last iteration processed STALE data from the previous read.");
        puts("  Worse, if fgets fails for a real error the loop spins forever.");
        puts("  ALWAYS test the read itself:  while (fgets(...) != NULL)");
    }

    puts("\n=== fgetc RETURNS int, NOT char ===");
    {
        FILE *f = fopen(TMPFILE, "r");
        int c, chars = 0, lines = 0, words = 0;
        bool in_word = false;

        while ((c = fgetc(f)) != EOF) {         /* MUST be an int */
            chars++;
            if (c == '\n') lines++;
            if (isspace(c)) in_word = false;
            else if (!in_word) { in_word = true; words++; }
        }
        fclose(f);
        printf("  wc-style count: %d lines, %d words, %d characters\n",
               lines, words, chars);
        puts("  fgetc must return 256 distinct byte values PLUS EOF (-1), which");
        puts("  needs 257 values — more than a char holds. Storing it in a char");
        puts("  makes the byte 0xFF indistinguishable from EOF. On a signed-char");
        puts("  platform that truncates any file containing 0xFF. Real bug.");
    }

    puts("\n=== BUFFERING ===");
    {
        puts("  stdout to a TERMINAL : line-buffered — flushes on every '\\n'");
        puts("  stdout to a PIPE/FILE: BLOCK-buffered (4-8 KB) — nothing appears");
        puts("                         until the buffer fills or the program exits");
        puts("  stderr               : unbuffered — appears immediately, always");
        puts("");
        puts("  That is why `./prog | cat` can show stderr lines out of order:");
        puts("  they are not out of order, stdout is just sitting in a buffer.");
        puts("");
        printf("  Prompt with no newline needs an explicit flush: ");
        fflush(stdout);                        /* force it out NOW */
        puts("[flushed]");
        puts("  fflush(stdout) is defined. fflush(stdin) is UNDEFINED BEHAVIOUR —");
        puts("  it does not discard pending input, whatever tutorials say.");
        puts("  setvbuf(f, NULL, _IONBF, 0) makes a stream unbuffered if you need it.");
    }

    puts("\n=== POSITIONING ===");
    {
        FILE *f = fopen(TMPFILE, "rb");        /* 'b' matters for ftell/fseek */
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        rewind(f);                              /* == fseek(f, 0, SEEK_SET) */
        printf("  file size via fseek/ftell: %ld bytes\n", size);

        fseek(f, 4, SEEK_SET);
        char buf[16] = {0};
        if (fread(buf, 1, 5, f) != 5) puts("  short read");
        printf("  bytes 4..8: \"%s\"\n", buf);
        printf("  position now: %ld\n", ftell(f));
        fclose(f);

        puts("  Caveats: this is only reliable for BINARY streams; for a text");
        puts("  stream ftell's value is not a meaningful byte count on all");
        puts("  platforms. It is also racy — the file can change underneath you.");
        puts("  On POSIX, stat() is the right way to get a file's size.");
        puts("  For files over 2 GB use fseeko/ftello (off_t, not long).");
    }

    puts("\n=== A REAL TASK: WORD FREQUENCY ===");
    {
        FILE *f = fopen(TMPFILE, "r");
        WordCount table[MAX_WORDS];
        int n_words = 0;
        char line[256];

        while (fgets(line, sizeof line, f) != NULL) {
            /* Hand-rolled tokenising: strtok would destroy the buffer and
             * collapse the runs of spaces we deliberately put in the file. */
            const char *p = line;
            while (*p) {
                while (*p && !isalpha((unsigned char)*p)) p++;      /* skip junk */
                if (!*p) break;
                const char *start = p;
                while (*p && isalpha((unsigned char)*p)) p++;

                char word[32];
                size_t len = (size_t)(p - start);
                if (len >= sizeof word) len = sizeof word - 1;
                for (size_t i = 0; i < len; i++)
                    word[i] = (char)tolower((unsigned char)start[i]);
                word[len] = '\0';

                int idx = wc_find_or_add(table, &n_words, word);
                if (idx >= 0) table[idx].count++;
            }
        }
        fclose(f);

        qsort(table, (size_t)n_words, sizeof table[0], wc_cmp);
        printf("  %d distinct words; the most frequent:\n", n_words);
        for (int i = 0; i < n_words && i < 6; i++)
            printf("    %-8s %d\n", table[i].word, table[i].count);
    }

    puts("\n=== WRITING ===");
    {
        FILE *f = fopen(TMPFILE, "a");         /* append: never truncates */
        if (f != NULL) {
            fprintf(f, "\nappended at %s\n", "run time");
            fputs("fputs adds no newline of its own\n", f);
            fputc('X', f);

            /* fclose can FAIL — a buffered write may only hit the disk here.
             * Ignoring its return value can silently lose data. */
            if (fclose(f) != 0) perror("  fclose");
            else puts("  appended and closed cleanly (fclose's return value checked)");
        }
    }

    remove(TMPFILE);
    puts("\n  (temporary file removed)");

    puts("\n=== THE RULES ===");
    puts("  1. Check fopen for NULL, and report errno.");
    puts("  2. Loop on the READ's return value, never on !feof.");
    puts("  3. fgetc/getc results go in an int.");
    puts("  4. Use \"rb\"/\"wb\" for anything that is not text.");
    puts("  5. fclose can fail — check it when the data matters.");
    puts("  6. fgets is bounded by sizeof buf; never use gets (removed in C11).");
    puts("  7. fflush(stdout) after a prompt; fflush(stdin) is undefined.");
    puts("  8. On POSIX, getline() is better than fgets: it handles any line");
    puts("     length by reallocating for you. Not ISO C, but universal.");

    return 0;
}
