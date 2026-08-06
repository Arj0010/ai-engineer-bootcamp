/* 02_binary_io.c — binary files, and why fwrite(&struct) is not a file format.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_binary_io.c -o t && ./t
 *
 * Two approaches are built side by side:
 *   (a) dump the struct  — fast, trivial, and unreadable on any other machine
 *   (b) serialise fields — explicit widths, explicit byte order, PORTABLE
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define RAW_FILE  "07_binary_raw.bin"
#define PORT_FILE "07_binary_portable.bin"

/* Deliberately laid out so it HAS padding, like most real structs. */
typedef struct {
    char     flag;         /* 1 byte  + 3 padding */
    int32_t  id;           /* 4 bytes */
    double   score;        /* 8 bytes */
    char     name[12];     /* 12 bytes + 4 tail padding */
} Record;

/* ================================================================= *
 * (a) THE NON-PORTABLE WAY — dump the struct's bytes
 * ================================================================= */
static bool dump_records(const char *path, const Record *recs, size_t n)
{
    FILE *f = fopen(path, "wb");                  /* 'b' matters on Windows */
    if (f == NULL) { perror("fopen"); return false; }

    /* fwrite returns the number of ITEMS written, not bytes. */
    size_t written = fwrite(recs, sizeof *recs, n, f);
    bool ok = (written == n) && (fclose(f) == 0);
    if (!ok) perror("write");
    return ok;
}
static bool load_records(const char *path, Record *recs, size_t n)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) { perror("fopen"); return false; }
    size_t got = fread(recs, sizeof *recs, n, f);
    fclose(f);
    return got == n;
}

/* ================================================================= *
 * (b) THE PORTABLE WAY — explicit widths, explicit byte order
 *
 * Little-endian is chosen here and written down. Any decision works as
 * long as it is DOCUMENTED and both sides agree. These helpers do the
 * conversion arithmetically, so they are correct on a big-endian machine
 * too — no #ifdef needed.
 * ================================================================= */
static void put_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v      ); p[1] = (uint8_t)(v >>  8);
    p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
static uint32_t get_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]        | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void put_u64_le(uint8_t *p, uint64_t v)
{
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (8 * i));
}
static uint64_t get_u64_le(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= (uint64_t)p[i] << (8 * i);
    return v;
}

/* A double's 8 bytes are moved with memcpy — a pointer cast would violate
 * strict aliasing. This assumes IEEE-754, which every machine you will meet
 * uses; a truly paranoid format would write the value as text or as a
 * mantissa/exponent pair. */
#define WIRE_RECORD_SIZE (1 + 4 + 8 + 12)      /* 25 bytes, NO padding */

static void serialise(const Record *r, uint8_t out[WIRE_RECORD_SIZE])
{
    size_t off = 0;
    out[off++] = (uint8_t)r->flag;
    put_u32_le(out + off, (uint32_t)r->id);      off += 4;
    uint64_t bits;
    memcpy(&bits, &r->score, sizeof bits);
    put_u64_le(out + off, bits);                 off += 8;
    memcpy(out + off, r->name, 12);              off += 12;
    (void)off;
}
static void deserialise(const uint8_t in[WIRE_RECORD_SIZE], Record *r)
{
    size_t off = 0;
    r->flag = (char)in[off++];
    r->id   = (int32_t)get_u32_le(in + off);     off += 4;
    uint64_t bits = get_u64_le(in + off);        off += 8;
    memcpy(&r->score, &bits, sizeof r->score);
    memcpy(r->name, in + off, 12);               off += 12;
    r->name[11] = '\0';                          /* never trust external data */
    (void)off;
}

/* A file header with a magic number and a version — every real format has one. */
typedef struct { char magic[4]; uint32_t version, count; } Header;

static bool write_portable(const char *path, const Record *recs, uint32_t n)
{
    FILE *f = fopen(path, "wb");
    if (f == NULL) return false;

    uint8_t hdr[12];
    memcpy(hdr, "RECS", 4);
    put_u32_le(hdr + 4, 1u);         /* version */
    put_u32_le(hdr + 8, n);          /* count   */
    if (fwrite(hdr, 1, sizeof hdr, f) != sizeof hdr) { fclose(f); return false; }

    for (uint32_t i = 0; i < n; i++) {
        uint8_t buf[WIRE_RECORD_SIZE];
        serialise(&recs[i], buf);
        if (fwrite(buf, 1, sizeof buf, f) != sizeof buf) { fclose(f); return false; }
    }
    return fclose(f) == 0;
}

static bool read_portable(const char *path, Record *recs, uint32_t max, uint32_t *out_n)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) return false;

    uint8_t hdr[12];
    if (fread(hdr, 1, sizeof hdr, f) != sizeof hdr) { fclose(f); return false; }

    /* VALIDATE. External data is hostile until proven otherwise. */
    if (memcmp(hdr, "RECS", 4) != 0)      { fclose(f); fputs("  bad magic\n",   stderr); return false; }
    if (get_u32_le(hdr + 4) != 1u)        { fclose(f); fputs("  bad version\n", stderr); return false; }

    uint32_t n = get_u32_le(hdr + 8);
    if (n > max) { fclose(f); fputs("  too many records for the buffer\n", stderr); return false; }

    for (uint32_t i = 0; i < n; i++) {
        uint8_t buf[WIRE_RECORD_SIZE];
        if (fread(buf, 1, sizeof buf, f) != sizeof buf) { fclose(f); return false; }
        deserialise(buf, &recs[i]);
    }
    *out_n = n;
    fclose(f);
    return true;
}

static void hexdump(const char *path, size_t max_bytes)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) return;
    unsigned char buf[16];
    size_t got, offset = 0;
    while (offset < max_bytes && (got = fread(buf, 1, sizeof buf, f)) > 0) {
        printf("    %04zx  ", offset);
        for (size_t i = 0; i < 16; i++)
            if (i < got) printf("%02x ", buf[i]); else printf("   ");
        printf(" |");
        for (size_t i = 0; i < got; i++)
            putchar((buf[i] >= 32 && buf[i] < 127) ? buf[i] : '.');
        puts("|");
        offset += got;
    }
    fclose(f);
}

int main(void)
{
    Record records[] = {
        {'A', 1001, 95.5,  "alice"},
        {'B', 1002, 87.25, "bob"},
        {'C', 1003, 78.0,  "carol"},
    };
    const size_t N = sizeof records / sizeof records[0];

    puts("=== THE STRUCT ===");
    printf("  sizeof(Record) = %zu bytes, but only %d bytes are real data\n",
           sizeof(Record), 1 + 4 + 8 + 12);
    printf("  offsets: flag=%zu id=%zu score=%zu name=%zu\n",
           offsetof(Record, flag), offsetof(Record, id),
           offsetof(Record, score), offsetof(Record, name));
    printf("  -> %zu bytes of PADDING per record\n", sizeof(Record) - 25);

    puts("\n=== (a) DUMPING THE STRUCT: fwrite(&rec, sizeof rec, n, f) ===");
    {
        if (!dump_records(RAW_FILE, records, N)) return 1;
        printf("  wrote %zu records = %zu bytes\n", N, N * sizeof(Record));

        Record loaded[3] = {0};
        if (load_records(RAW_FILE, loaded, N)) {
            puts("  read back:");
            for (size_t i = 0; i < N; i++)
                printf("    %c %d %.2f %s\n",
                       loaded[i].flag, loaded[i].id, loaded[i].score, loaded[i].name);
            printf("  round-trip on THIS machine: %s\n",
                   memcmp(records, loaded, sizeof records) == 0 ? "identical" : "DIFFERENT");
        }

        puts("\n  first 32 bytes on disk:");
        hexdump(RAW_FILE, 32);
        puts("    Note the 00 bytes after the flag: that is PADDING, written to");
        puts("    the file. It carries no information and its contents are not");
        puts("    even guaranteed to be reproducible.");
    }

    puts("\n=== WHY THAT IS NOT A FILE FORMAT ===");
    puts("  fwrite(&struct) bakes THREE machine-specific things into the file:");
    puts("");
    puts("  1. PADDING. The compiler chooses it. A different compiler, a");
    puts("     different -O level, or a #pragma pack changes the layout and");
    puts("     the file becomes unreadable.");
    puts("  2. ENDIANNESS. x86 and ARM are little-endian; network byte order");
    puts("     and some embedded targets are big-endian. The same 4 bytes read");
    printf("     as 1001 here would read as %u on a big-endian machine.\n",
           (unsigned)((1001u >> 24) | ((1001u >> 8) & 0xFF00u) |
                      ((1001u << 8) & 0xFF0000u) | (1001u << 24)));
    puts("  3. TYPE SIZES. `long` is 8 bytes on Linux and 4 on Windows.");
    puts("     `int` is 2 bytes on some embedded targets.");
    puts("");
    puts("  And if the struct contains a POINTER, you have written an ADDRESS");
    puts("  from this process to disk. It is meaningless anywhere else, and it");
    puts("  is a genuine information leak.");
    puts("");
    puts("  fwrite(&struct) is fine for a cache or scratch file that the SAME");
    puts("  binary rereads. It is not fine for anything anyone else will read.");

    puts("\n=== (b) EXPLICIT SERIALISATION ===");
    {
        if (!write_portable(PORT_FILE, records, (uint32_t)N)) { perror("write"); return 1; }
        printf("  wrote a 12-byte header + %zu records of %d bytes = %zu bytes\n",
               N, WIRE_RECORD_SIZE, 12 + N * WIRE_RECORD_SIZE);

        Record loaded[8] = {0};
        uint32_t got = 0;
        if (read_portable(PORT_FILE, loaded, 8, &got)) {
            printf("  read back %u records:\n", got);
            for (uint32_t i = 0; i < got; i++)
                printf("    %c %d %.2f %s\n",
                       loaded[i].flag, loaded[i].id, loaded[i].score, loaded[i].name);

            bool same = true;
            for (size_t i = 0; i < N; i++)
                if (records[i].id != loaded[i].id ||
                    records[i].score != loaded[i].score ||
                    strcmp(records[i].name, loaded[i].name) != 0) same = false;
            printf("  field-by-field comparison: %s\n", same ? "identical" : "DIFFERENT");
            puts("  (field by field, NOT memcmp — memcmp would compare padding)");
        }

        puts("\n  the whole file:");
        hexdump(PORT_FILE, 96);
        puts("    Every byte is meaningful. No padding. Byte order is defined by");
        puts("    the code, not by the CPU. This file is readable by any program");
        puts("    on any machine that knows the format.");
    }

    puts("\n=== WHAT EVERY REAL BINARY FORMAT HAS ===");
    puts("  MAGIC NUMBER  \"RECS\", 0x89PNG, \\x7fELF — proves it is your format");
    puts("                and catches a truncated or wrong file immediately");
    puts("  VERSION       so a future reader can handle old files");
    puts("  COUNT/LENGTH  so the reader knows when to stop, and can size buffers");
    puts("  FIXED WIDTHS  int32_t, not int; uint64_t, not long");
    puts("  DEFINED ORDER little- or big-endian, written down");
    puts("  VALIDATION    check the magic, the version, and EVERY length before");
    puts("                you allocate or index with it");
    puts("");
    puts("  That last point is the security one. `count` came from a file you");
    puts("  did not write. malloc(count * sizeof(Record)) with an attacker-chosen");
    puts("  count is an integer overflow into a heap overflow. This is the single");
    puts("  most common class of file-parser CVE.");

    puts("\n=== fread AND fwrite RETURN ITEM COUNTS, NOT BYTES ===");
    puts("  size_t n = fread(buf, sizeof(Record), 10, f);");
    puts("    n == 10  -> got all ten");
    puts("    n <  10  -> short read: check feof() vs ferror() to learn why");
    puts("  A short read is NOT an error by itself. Always check which it was.");

    remove(RAW_FILE);
    remove(PORT_FILE);
    puts("\n  (temporary files removed)");
    return 0;
}
