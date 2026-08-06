/* 02_strings.c — what a C string really is, and the <string.h> toolkit.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 02_strings.c -o t && ./t
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(void)
{
    puts("=== A STRING IS AN ARRAY OF char ENDING IN '\\0' ===");
    {
        char s[] = "hi";
        printf("  char s[] = \"hi\";\n");
        printf("  sizeof s = %zu  <- 3 bytes: 'h', 'i', and the NUL terminator\n", sizeof s);
        printf("  strlen(s)= %zu  <- length EXCLUDES the terminator\n", strlen(s));
        printf("  bytes: ");
        for (size_t i = 0; i < sizeof s; i++)
            printf("[%zu]=%d('%c') ", i, s[i], s[i] ? s[i] : '0');
        puts("\n  The '\\0' is DATA. Every function in <string.h> depends on it");
        puts("  being there. Lose it and strlen walks off into other memory.");
    }

    puts("\n=== ARRAY vs POINTER — the difference that segfaults people ===");
    {
        char        arr[] = "modifiable";   /* a COPY of the literal, on the stack */
        const char *ptr   = "read-only";    /* points INTO .rodata */

        printf("  char arr[]      = \"%s\"   sizeof = %zu (the array)\n", arr, sizeof arr);
        printf("  const char *ptr = \"%s\"   sizeof = %zu (just the pointer)\n",
               ptr, sizeof ptr);

        arr[0] = 'M';                       /* fine */
        printf("  arr[0] = 'M'  -> \"%s\"\n", arr);
        puts("  ptr[0] = 'R'  -> UNDEFINED BEHAVIOUR. The literal lives in a");
        puts("                   read-only page; this usually segfaults.");
        puts("  Declaring it `const char *` makes the compiler stop you. Do that.");
        printf("  addresses: arr is on the stack (%p),\n", (void *)arr);
        printf("             ptr points into .rodata (%p)\n", (const void *)ptr);
    }

    puts("\n=== LENGTH: strlen is O(n) ===");
    {
        const char *s = "the quick brown fox";
        printf("  strlen(\"%s\") = %zu\n", s, strlen(s));
        puts("  It COUNTS to the terminator every single time it is called.");
        puts("  NEVER: for (size_t i = 0; i < strlen(s); i++)   <- O(n^2)");
        puts("  ALWAYS: size_t n = strlen(s); for (size_t i = 0; i < n; i++)");
        puts("  or just: for (const char *p = s; *p; p++)       <- O(n), idiomatic");
    }

    puts("\n=== COPYING ===");
    {
        char dst[32];

        strcpy(dst, "strcpy: no bound at all");
        printf("  strcpy  -> \"%s\"\n", dst);
        puts("            UNSAFE. If the source is longer than dst, it writes");
        puts("            past the end. This is THE buffer overflow.");

        /* strncpy: bounded, but two surprises. */
        char small[8];
        strncpy(small, "abcdefghij", sizeof small);
        /* Surprise 1: it copied exactly 8 bytes and did NOT terminate. */
        small[sizeof small - 1] = '\0';       /* you must do this yourself */
        printf("  strncpy -> \"%s\" (had to terminate manually)\n", small);

        char padded[16];
        strncpy(padded, "abc", sizeof padded);
        /* Surprise 2: when the source is SHORTER, it zero-pads the whole
         * destination. That is O(size), not O(strlen) — a real cost in loops. */
        printf("  strncpy -> \"%s\", and zero-padded all %zu bytes\n",
               padded, sizeof padded);
        puts("            strncpy was built for fixed-width records, not safety.");

        /* snprintf: always terminates, and TELLS you if it truncated.
         * Looping over a table keeps the compiler from constant-folding this
         * into a compile-time -Wformat-truncation warning — which is exactly
         * the situation you are in with real, run-time input. */
        char out[8];
        const char *inputs[] = { "short", "abcdefghij" };
        int need = 0;
        for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
            need = snprintf(out, sizeof out, "%s", inputs[i]);
            printf("  snprintf-> \"%-7s\" returned %2d  %s\n", out, need,
                   (size_t)need >= sizeof out ? "TRUNCATED" : "fitted");
        }
        printf("            %d >= %zu is how you DETECT truncation.\n",
               need, sizeof out);
        puts("            snprintf returns the length it WOULD have written.");
        puts("            This is the safest general-purpose copy in C.");
    }

    puts("\n=== CONCATENATION ===");
    {
        char buf[64] = "start";
        strcat(buf, " + strcat");
        strncat(buf, " + strncat-bounded", sizeof buf - strlen(buf) - 1);
        printf("  \"%s\"\n", buf);
        puts("  Note strncat's n is 'how many bytes to APPEND, plus a NUL' —");
        puts("  different from strncpy's n, which is the total buffer size.");
        puts("  Repeated strcat is also O(n^2): each call rescans the whole");
        puts("  destination to find the end. Track the offset yourself instead");
        puts("  (see 03_safe_strings.c).");
    }

    puts("\n=== COMPARING ===");
    {
        const char *a = "apple", *b = "banana", *c = "apple";
        printf("  strcmp(\"apple\",\"banana\") = %d  (<0: a sorts before b)\n", strcmp(a, b));
        printf("  strcmp(\"banana\",\"apple\") = %d  (>0)\n", strcmp(b, a));
        printf("  strcmp(\"apple\",\"apple\")  = %d  (==0 means EQUAL)\n", strcmp(a, c));
        puts("  Write `strcmp(x,y) == 0` for equality. `!strcmp(x,y)` reads");
        puts("  backwards and is a documented source of inverted logic.");
        printf("  strncmp(\"apple\",\"apricot\",2) = %d  (first 2 chars match)\n",
               strncmp("apple", "apricot", 2));
        puts("  The magnitude is NOT the alphabetical distance and is not portable.");
        puts("  Only the SIGN is meaningful.");
        puts("  a == b compares POINTERS, not contents. It is almost always a bug.");
    }

    puts("\n=== SEARCHING ===");
    {
        const char *text = "the quick brown fox jumps";
        const char *found;

        found = strchr(text, 'q');
        printf("  strchr(s,'q')   -> offset %td: \"%s\"\n", found - text, found);
        found = strrchr(text, 'o');
        printf("  strrchr(s,'o')  -> offset %td: \"%s\"  (LAST occurrence)\n",
               found - text, found);
        found = strstr(text, "brown");
        printf("  strstr(s,\"brown\")-> offset %td: \"%s\"\n", found - text, found);
        printf("  strstr(s,\"zzz\") -> %s\n", strstr(text, "zzz") ? "found" : "NULL");
        printf("  strspn(s,\"the \")= %zu  (length of the initial run of those chars)\n",
               strspn(text, "the "));
        printf("  strcspn(s,\"aeiou\")=%zu (length until the FIRST of those chars)\n",
               strcspn(text, "aeiou"));
        puts("  strcspn(line, \"\\n\") is the idiomatic way to find a newline's index.");
    }

    puts("\n=== TOKENISING with strtok — and why it is dangerous ===");
    {
        /* strtok MODIFIES its input, so it cannot take a literal. */
        char csv[] = "alice,30,engineer,,london";
        printf("  input: \"%s\"\n", csv);
        printf("  tokens: ");
        for (char *tok = strtok(csv, ","); tok != NULL; tok = strtok(NULL, ","))
            printf("[%s] ", tok);
        puts("");
        printf("  input AFTER strtok: \"%s\"  <- commas replaced by NULs!\n", csv);
        puts("  Problems with strtok:");
        puts("    1. It writes '\\0' over the delimiters — the input is destroyed.");
        puts("    2. It keeps state in a hidden GLOBAL, so you cannot tokenise");
        puts("       two strings at once, and it is not thread-safe.");
        puts("    3. It COLLAPSES consecutive delimiters — note the empty field");
        puts("       between 'engineer' and 'london' vanished. Fatal for CSV.");
        puts("  Use strtok_r (POSIX) for reentrancy, or hand-roll with strcspn");
        puts("  (see 04_string_algorithms.c) when empty fields matter.");
    }

    puts("\n=== mem* FUNCTIONS: bytes, not strings ===");
    {
        char a[16], b[16];
        memset(a, 'x', 8); a[8] = '\0';
        printf("  memset(a,'x',8)      -> \"%s\"\n", a);

        int nums[4];
        memset(nums, 0, sizeof nums);
        printf("  memset(nums,0,...)   -> {%d,%d,%d,%d}  (0 works for any type)\n",
               nums[0], nums[1], nums[2], nums[3]);
        memset(nums, 1, sizeof nums);
        printf("  memset(nums,1,...)   -> {%d,...}  <- sets BYTES: 0x01010101\n", nums[0]);
        puts("                          memset only makes sense for 0 or byte fills.");

        memcpy(b, a, 9);
        printf("  memcpy(b,a,9)        -> \"%s\"\n", b);
        puts("  memcpy: the regions MUST NOT OVERLAP. Overlapping is UB even");
        puts("          though it often 'works'.");

        char shift[] = "ABCDEFGH";
        memmove(shift + 2, shift, 6);      /* overlapping: shift right by 2 */
        shift[8] = '\0';
        printf("  memmove overlapping  -> \"%s\"  (memmove handles overlap)\n", shift);

        puts("  memcmp on structs is unreliable: it compares PADDING bytes too,");
        puts("  and padding is uninitialised. Compare fields explicitly.");
    }

    puts("\n=== <ctype.h>: ALWAYS cast to unsigned char ===");
    {
        const char *s = "Hello, World 42!";
        int letters = 0, digits = 0, spaces = 0, punct = 0;
        for (const char *p = s; *p; p++) {
            unsigned char c = (unsigned char)*p;   /* the required cast */
            if      (isalpha(c)) letters++;
            else if (isdigit(c)) digits++;
            else if (isspace(c)) spaces++;
            else                 punct++;
        }
        printf("  \"%s\": %d letters, %d digits, %d spaces, %d punctuation\n",
               s, letters, digits, spaces, punct);

        char upper[32];
        size_t i = 0;
        for (const char *p = s; *p && i < sizeof upper - 1; p++, i++)
            upper[i] = (char)toupper((unsigned char)*p);
        upper[i] = '\0';
        printf("  uppercased: \"%s\"\n", upper);
        puts("  Why the cast: if char is signed, a byte like 0xE9 is -23, and");
        puts("  isalpha(-23) indexes a lookup table out of bounds. Undefined");
        puts("  behaviour, and it really does crash on some libc builds.");
    }

    puts("\n=== strdup: convenient, POSIX-only, and YOU free it ===");
    {
        const char *original = "duplicate me";
        size_t n = strlen(original) + 1;
        char *copy = malloc(n);            /* the portable strdup */
        if (copy != NULL) {
            memcpy(copy, original, n);
            printf("  copied \"%s\" to fresh heap memory at %p\n", copy, (void *)copy);
            free(copy);                    /* forget this and it leaks */
        }
        puts("  strdup() is POSIX, not ISO C (it IS in C23). malloc+memcpy is");
        puts("  portable everywhere. Either way, the caller owns the result.");
    }

    return 0;
}
