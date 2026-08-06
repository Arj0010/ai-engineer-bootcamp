/* 04_string_algorithms.c — implement the standard string routines yourself.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 04_string_algorithms.c -o t && ./t
 *
 * Writing these by hand is the fastest way to internalise pointer walking
 * and NUL termination. Each one is checked against the libc version.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>

/* ============ reimplementations of <string.h> ============ */

static size_t my_strlen(const char *s)
{
    const char *p = s;
    while (*p) p++;              /* stop at the NUL */
    return (size_t)(p - s);      /* pointer difference = element count */
}

static char *my_strcpy(char *dst, const char *src)
{
    char *out = dst;
    while ((*dst++ = *src++) != '\0')   /* copies, then tests, INCLUDING the NUL */
        ;
    return out;
}

static int my_strcmp(const char *a, const char *b)
{
    /* Compare as UNSIGNED char: the standard requires it, and it makes the
     * ordering consistent for bytes above 127. */
    while (*a && (*a == *b)) { a++; b++; }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static char *my_strchr(const char *s, int c)
{
    for (; ; s++) {
        if (*s == (char)c) return (char *)s;
        if (*s == '\0')    return NULL;      /* checked AFTER, so strchr(s,0) works */
    }
}

static char *my_strstr(const char *hay, const char *needle)
{
    if (*needle == '\0') return (char *)hay;         /* empty needle matches at 0 */
    for (; *hay; hay++) {
        const char *h = hay, *n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (*n == '\0') return (char *)hay;          /* consumed the whole needle */
    }
    return NULL;
}

static void *my_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char       *d = dst;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

/* memmove must handle overlap. The trick: if the regions overlap and dst is
 * AFTER src, copy backwards so you read each byte before overwriting it. */
static void *my_memmove(void *dst, const void *src, size_t n)
{
    unsigned char       *d = dst;
    const unsigned char *s = src;
    if (d < s) { while (n--) *d++ = *s++; }
    else       { d += n; s += n; while (n--) *--d = *--s; }
    return dst;
}

/* ============ classic string algorithms ============ */

static void reverse_in_place(char *s)
{
    size_t n = strlen(s);
    for (size_t i = 0, j = n ? n - 1 : 0; i < j; i++, j--) {
        char t = s[i]; s[i] = s[j]; s[j] = t;
    }
}

static bool is_palindrome(const char *s)
{
    size_t i = 0, j = strlen(s);
    if (j == 0) return true;
    j--;
    while (i < j) {
        while (i < j && !isalnum((unsigned char)s[i])) i++;   /* skip punctuation */
        while (i < j && !isalnum((unsigned char)s[j])) j--;
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)s[j]))
            return false;
        i++; j--;
    }
    return true;
}

static size_t count_words(const char *s)
{
    size_t words = 0;
    bool in_word = false;
    for (; *s; s++) {
        if (isspace((unsigned char)*s)) in_word = false;
        else if (!in_word)              { in_word = true; words++; }
    }
    return words;
}

/* Reverse the ORDER of words, keeping each word intact.
 * The classic trick: reverse the whole string, then reverse each word. */
static void reverse_words(char *s)
{
    reverse_in_place(s);
    char *word = s;
    for (char *p = s; ; p++) {
        if (*p == ' ' || *p == '\0') {
            char save = *p;
            *p = '\0';
            reverse_in_place(word);
            *p = save;
            if (save == '\0') break;
            word = p + 1;
        }
    }
}

/* Collapse runs of whitespace, trim the ends. In place, O(n). */
static void squeeze_spaces(char *s)
{
    char *w = s;                    /* write cursor */
    const char *r = s;              /* read cursor  */
    while (isspace((unsigned char)*r)) r++;          /* skip leading */
    while (*r) {
        if (!isspace((unsigned char)*r)) { *w++ = *r++; }
        else { *w++ = ' '; while (isspace((unsigned char)*r)) r++; }
    }
    if (w > s && w[-1] == ' ') w--;                  /* drop the trailing one */
    *w = '\0';
}

/* ============ number <-> string, by hand ============ */

static int my_atoi(const char *s)
{
    while (isspace((unsigned char)*s)) s++;
    int sign = 1;
    if (*s == '-' || *s == '+') sign = (*s++ == '-') ? -1 : 1;
    long long value = 0;
    while (isdigit((unsigned char)*s)) {
        value = value * 10 + (*s++ - '0');           /* '7' - '0' == 7 */
        if (value > (long long)INT_MAX + 1) break;   /* saturate rather than wrap */
    }
    value *= sign;
    if (value > INT_MAX) return INT_MAX;
    if (value < INT_MIN) return INT_MIN;
    return (int)value;
}

/* itoa is not in ISO C. Here is a correct one, including the INT_MIN case
 * that trips up most implementations (you cannot negate INT_MIN). */
static void my_itoa(int value, char *buf, size_t bufsize)
{
    if (bufsize == 0) return;
    /* Build digits into a temp, backwards, using an unsigned magnitude so
     * INT_MIN works. -INT_MIN is undefined behaviour; this avoids it. */
    char tmp[16];
    size_t n = 0;
    unsigned magnitude = (value < 0) ? (unsigned)(-(long long)value) : (unsigned)value;
    do { tmp[n++] = (char)('0' + magnitude % 10); magnitude /= 10; } while (magnitude);
    if (value < 0 && n < sizeof tmp) tmp[n++] = '-';

    size_t out = 0;
    while (n > 0 && out < bufsize - 1) buf[out++] = tmp[--n];   /* reverse it */
    buf[out] = '\0';
}

/* Convert to any base 2..36 — the mechanism behind %x, %o, and strtol. */
static void to_base(unsigned long value, int base, char *buf, size_t bufsize)
{
    static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    char tmp[65];
    size_t n = 0;
    if (base < 2 || base > 36 || bufsize == 0) { if (bufsize) buf[0] = '\0'; return; }
    do { tmp[n++] = digits[value % (unsigned long)base]; value /= (unsigned long)base; }
    while (value && n < sizeof tmp);
    size_t out = 0;
    while (n > 0 && out < bufsize - 1) buf[out++] = tmp[--n];
    buf[out] = '\0';
}

#define CHECK(label, mine, theirs) \
    printf("  %-34s mine=%-18s libc=%-18s %s\n", label, mine, theirs, \
           strcmp(mine, theirs) == 0 ? "OK" : "MISMATCH")

int main(void)
{
    puts("=== REIMPLEMENTING <string.h> ===");
    {
        const char *s = "the quick brown fox";
        printf("  my_strlen  : %zu   (libc: %zu)  %s\n",
               my_strlen(s), strlen(s), my_strlen(s) == strlen(s) ? "OK" : "MISMATCH");

        char a[32], b[32];
        my_strcpy(a, s); strcpy(b, s);
        CHECK("my_strcpy", a, b);

        printf("  my_strcmp  : (\"abc\",\"abd\") sign %d   libc sign %d  %s\n",
               (my_strcmp("abc","abd") < 0) ? -1 : 1,
               (strcmp   ("abc","abd") < 0) ? -1 : 1,
               ((my_strcmp("abc","abd") < 0) == (strcmp("abc","abd") < 0)) ? "OK" : "MISMATCH");

        printf("  my_strchr  : offset %td   libc offset %td  %s\n",
               my_strchr(s, 'q') - s, strchr(s, 'q') - s,
               my_strchr(s,'q') == strchr(s,'q') ? "OK" : "MISMATCH");

        printf("  my_strstr  : offset %td   libc offset %td  %s\n",
               my_strstr(s, "brown") - s, strstr(s, "brown") - s,
               my_strstr(s,"brown") == strstr(s,"brown") ? "OK" : "MISMATCH");
        printf("  my_strstr  : missing needle -> %s\n",
               my_strstr(s, "zebra") == NULL ? "NULL (OK)" : "wrong");

        char c1[16] = "..............", c2[16] = "..............";
        my_memcpy(c1, "abcdef", 7); memcpy(c2, "abcdef", 7);
        CHECK("my_memcpy", c1, c2);

        /* the overlapping case memcpy is not allowed to handle */
        char o1[16] = "ABCDEFGH", o2[16] = "ABCDEFGH";
        my_memmove(o1 + 2, o1, 6); o1[8] = '\0';
        memmove   (o2 + 2, o2, 6); o2[8] = '\0';
        CHECK("my_memmove (overlapping)", o1, o2);
    }

    puts("\n=== CLASSIC ALGORITHMS ===");
    {
        char buf[64];

        strcpy(buf, "hello world");
        reverse_in_place(buf);
        printf("  reverse(\"hello world\")     = \"%s\"\n", buf);

        const char *tests[] = { "racecar", "A man, a plan, a canal: Panama",
                                "hello", "", "a" };
        for (size_t i = 0; i < sizeof tests / sizeof tests[0]; i++)
            printf("  is_palindrome(\"%.28s\")%*s = %s\n", tests[i],
                   (int)(28 - (strlen(tests[i]) > 28 ? 28 : strlen(tests[i]))), "",
                   is_palindrome(tests[i]) ? "yes" : "no");

        printf("  count_words(\"  the quick  brown fox \") = %zu\n",
               count_words("  the quick  brown fox "));

        strcpy(buf, "the quick brown fox");
        reverse_words(buf);
        printf("  reverse_words              = \"%s\"\n", buf);

        strcpy(buf, "   too    many     spaces   here  ");
        squeeze_spaces(buf);
        printf("  squeeze_spaces             = \"%s\"\n", buf);
    }

    puts("\n=== NUMBER CONVERSION BY HAND ===");
    {
        const char *nums[] = { "42", "  -137", "+7", "2147483647", "-2147483648",
                               "99999999999", "12abc", "abc" };
        for (size_t i = 0; i < sizeof nums / sizeof nums[0]; i++)
            printf("  my_atoi(\"%-12s\") = %12d   (libc atoi: %d)\n",
                   nums[i], my_atoi(nums[i]), atoi(nums[i]));
        puts("  Note both return 0 for \"abc\" — indistinguishable from a real 0.");
        puts("  That is why real code uses strtol, which reports where it stopped.");

        char buf[32];
        int vals[] = { 0, 7, -7, 12345, INT_MAX, INT_MIN };
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++) {
            my_itoa(vals[i], buf, sizeof buf);
            char ref[32];
            snprintf(ref, sizeof ref, "%d", vals[i]);
            printf("  my_itoa(%-12d) = %-12s  %s\n", vals[i], buf,
                   strcmp(buf, ref) == 0 ? "OK" : "MISMATCH");
        }
        puts("  INT_MIN is the case that breaks naive implementations: you cannot");
        puts("  compute -INT_MIN, so build the magnitude as an unsigned value.");

        puts("\n  arbitrary bases (this is how printf's %x and %o work):");
        for (int base = 2; base <= 36; base += 6) {
            to_base(48879, base, buf, sizeof buf);
            printf("    48879 in base %-2d = %s\n", base, buf);
        }
    }

    puts("\n=== THE PATTERNS WORTH KEEPING ===");
    puts("  while (*p) p++;                walk to the NUL");
    puts("  while ((*d++ = *s++)) ;        copy including the terminator");
    puts("  p - s                          pointer difference = element count");
    puts("  '7' - '0' == 7                 ASCII digits are contiguous");
    puts("  build digits backwards, then reverse — every itoa works this way");
    puts("  two pointers moving inward     palindromes, reversal, partitioning");
    puts("  read cursor + write cursor     in-place filtering, O(n), no allocation");

    return 0;
}
