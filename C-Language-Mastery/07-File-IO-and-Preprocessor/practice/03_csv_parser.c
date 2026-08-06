/* 03_csv_parser.c — a CSV reader that handles the cases naive ones do not.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_csv_parser.c -o t && ./t
 *
 * "Just split on commas" fails on real CSV. This handles:
 *   - quoted fields containing commas:      "Smith, John",42
 *   - escaped quotes inside quotes:         "say ""hi""",1
 *   - EMPTY fields:                         a,,c   -> three fields
 *   - trailing empty field:                 a,b,   -> three fields
 *   - CRLF line endings from Windows
 *   - lines longer than any fixed buffer
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define CSV_FILE "07_csv_demo.csv"
#define MAX_FIELDS 16

/* A parsed row. Fields point INTO `storage`, which owns the bytes — one
 * allocation for the whole row instead of one per field. */
typedef struct {
    char  *storage;
    char  *fields[MAX_FIELDS];
    size_t n_fields;
} Row;

static void row_free(Row *r) { free(r->storage); r->storage = NULL; r->n_fields = 0; }

/* ---------------------------------------------------------------- *
 * THE PARSER — a small state machine over one line.
 *
 * Writes the unescaped field contents back into `storage` in place, using
 * a read cursor and a write cursor. The write cursor never overtakes the
 * read cursor because unescaping only ever SHRINKS the text.
 * ---------------------------------------------------------------- */
static bool csv_parse_line(const char *line, Row *out)
{
    size_t len = strlen(line);
    out->storage = malloc(len + 1);
    if (out->storage == NULL) return false;
    out->n_fields = 0;

    const char *r = line;                 /* read cursor  */
    char       *w = out->storage;         /* write cursor */

    for (;;) {
        if (out->n_fields >= MAX_FIELDS) { row_free(out); return false; }
        out->fields[out->n_fields++] = w;   /* this field starts here */

        if (*r == '"') {
            /* QUOTED FIELD: commas and newlines are literal; "" is one quote. */
            r++;
            while (*r != '\0') {
                if (*r == '"') {
                    if (r[1] == '"') { *w++ = '"'; r += 2; }   /* escaped quote */
                    else             { r++; break; }           /* closing quote */
                } else {
                    *w++ = *r++;
                }
            }
        } else {
            /* UNQUOTED FIELD: runs to the next comma or end of line. */
            while (*r != '\0' && *r != ',') *w++ = *r++;
        }

        *w++ = '\0';                       /* terminate this field */

        if (*r == ',') { r++; continue; }  /* another field follows... */
        if (*r == '\0') break;             /* ...or that was the last one */
        /* Anything else here is malformed (e.g. text after a closing quote);
         * be permissive and skip to the next comma. */
        while (*r != '\0' && *r != ',') r++;
        if (*r == ',') r++;
    }
    return true;
}

/* Strip a trailing newline, handling both "\n" and Windows "\r\n". */
static void chomp(char *s)
{
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

/* ---------------------------------------------------------------- *
 * Reading a line of ANY length. fgets caps at the buffer size, so a long
 * line arrives in pieces; this joins them, growing as needed.
 * Returns a malloc'd string the caller must free, or NULL at EOF.
 * ---------------------------------------------------------------- */
static char *read_full_line(FILE *f)
{
    size_t cap = 128, len = 0;
    char *buf = malloc(cap);
    if (buf == NULL) return NULL;

    for (;;) {
        if (fgets(buf + len, (int)(cap - len), f) == NULL) {
            if (len == 0) { free(buf); return NULL; }   /* EOF with nothing read */
            break;
        }
        len += strlen(buf + len);

        if (len > 0 && buf[len - 1] == '\n') break;     /* got a whole line */
        if (len + 1 < cap) break;                       /* short read: EOF */

        char *tmp = realloc(buf, cap * 2);              /* need more room */
        if (tmp == NULL) { free(buf); return NULL; }
        buf = tmp; cap *= 2;
    }
    return buf;
}

static void write_sample_csv(void)
{
    FILE *f = fopen(CSV_FILE, "w");
    if (f == NULL) return;
    fputs("id,name,role,city,notes\n", f);
    fputs("1,Alice,Engineer,London,\n",  f);                 /* trailing empty */
    fputs("2,\"Smith, John\",Manager,Paris,likes commas\n", f); /* comma in quotes */
    fputs("3,Carol,,Berlin,no role\n", f);                   /* empty middle */
    fputs("4,\"O\"\"Brien\",Analyst,Dublin,\"said \"\"hello\"\"\"\n", f);  /* escaped quotes */
    fputs("5,,,,\n", f);                                     /* all empty */
    fputs("6,Frank,Designer,Tokyo,\"multi word, with comma\"\r\n", f); /* CRLF */
    fclose(f);
}

int main(void)
{
    write_sample_csv();

    puts("=== WHY \"SPLIT ON COMMAS\" IS NOT ENOUGH ===");
    puts("  Real CSV has four cases that break the naive approach:");
    puts("    \"Smith, John\",42      a comma INSIDE a quoted field");
    puts("    \"say \"\"hi\"\"\",1        an escaped quote inside quotes");
    puts("    a,,c                  an EMPTY field (strtok silently eats it)");
    puts("    a,b,                  a trailing empty field");
    puts("  Plus CRLF line endings and lines longer than your buffer.\n");

    puts("=== PARSING THE SAMPLE FILE ===");
    {
        FILE *f = fopen(CSV_FILE, "r");
        if (f == NULL) { perror("fopen"); return 1; }

        char *line;
        int line_no = 0;
        while ((line = read_full_line(f)) != NULL) {
            chomp(line);
            line_no++;

            Row row = {0};
            if (!csv_parse_line(line, &row)) {
                printf("  line %d: PARSE FAILED\n", line_no);
                free(line);
                continue;
            }

            printf("  line %d (%zu fields): ", line_no, row.n_fields);
            for (size_t i = 0; i < row.n_fields; i++) {
                if (row.fields[i][0] == '\0') printf("[<empty>] ");
                else                          printf("[%s] ", row.fields[i]);
            }
            puts("");

            row_free(&row);
            free(line);
        }
        fclose(f);
    }

    puts("\n=== WHAT THE PARSER GOT RIGHT ===");
    puts("  line 3: \"Smith, John\" stayed as ONE field — the comma was quoted");
    puts("  line 4: the empty role field is preserved, not collapsed");
    puts("  line 5: O\"\"Brien became O\"Brien; \"\"hello\"\" became \"hello\"");
    puts("  line 6: five empty fields, all counted");
    puts("  line 7: the \\r\\n was stripped by chomp()");

    puts("\n=== WHY NOT strtok ===");
    {
        char sample[] = "a,,c,";
        printf("  input: \"a,,c,\"  (4 fields: a, empty, c, empty)\n");
        printf("  strtok gives: ");
        for (char *t = strtok(sample, ","); t; t = strtok(NULL, ",")) printf("[%s] ", t);
        puts("  <- only 2!");
        puts("  strtok treats RUNS of delimiters as one, so every empty field");
        puts("  disappears and the column alignment shifts. For CSV that is not");
        puts("  an inconvenience, it is silent data corruption.");
        puts("  It also destroys the input buffer and keeps state in a global,");
        puts("  so it cannot be used on two strings at once or from two threads.");
    }

    puts("\n=== THE TECHNIQUES WORTH TAKING AWAY ===");
    puts("  1. STATE MACHINE. 'in a quoted field' vs 'not' is a state, and");
    puts("     handling it as one keeps the code linear and total.");
    puts("  2. READ CURSOR + WRITE CURSOR. Unescaping only shrinks text, so the");
    puts("     write cursor never overtakes the read cursor and you can decode");
    puts("     in place with no second buffer.");
    puts("  3. ONE ALLOCATION PER ROW. The fields point INTO a single buffer,");
    puts("     so freeing a row is one free(), not one per field.");
    puts("  4. GROW THE LINE BUFFER. Never assume a maximum line length; a");
    puts("     fixed buffer means a long line silently splits into two records.");
    puts("  5. HANDLE CRLF. Files cross platforms; \\r is not whitespace to");
    puts("     every function you might use.");
    puts("");
    puts("  For production work, use a tested library. RFC 4180 also allows");
    puts("  NEWLINES inside quoted fields, which means the reader cannot work");
    puts("  line by line at all — it has to consume the whole file as a stream.");

    remove(CSV_FILE);
    puts("\n  (temporary file removed)");
    return 0;
}
