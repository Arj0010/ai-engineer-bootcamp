/* 01_pointer_basics.c — a pointer is a variable holding an address. That is all.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_pointer_basics.c -o t && ./t
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* The two ways to give a function access to the caller's data. */
static void cannot_modify(int x)  { x = 99; (void)x; }
static void can_modify(int *x)    { *x = 99; }

/* An out-parameter: return a status, write the results through pointers. */
static int minmax(const int *a, size_t n, int *out_min, int *out_max)
{
    if (a == NULL || n == 0) return -1;
    int lo = a[0], hi = a[0];
    for (size_t i = 1; i < n; i++) {
        if (a[i] < lo) lo = a[i];
        if (a[i] > hi) hi = a[i];
    }
    if (out_min) *out_min = lo;       /* NULL means "caller does not want it" */
    if (out_max) *out_max = hi;
    return 0;
}

int main(void)
{
    puts("=== MEMORY IS A NUMBERED ARRAY OF BYTES ===");
    {
        int x = 42;
        int *p = &x;                  /* & takes the address */

        printf("  int x = 42;\n");
        printf("    x        = %d          the VALUE\n", x);
        printf("    &x       = %p    the ADDRESS (where the 4 bytes live)\n", (void *)&x);
        printf("    sizeof x = %zu bytes\n", sizeof x);

        printf("  int *p = &x;\n");
        printf("    p        = %p    p HOLDS that address\n", (void *)p);
        printf("    *p       = %d          dereference: read the int there\n", *p);
        printf("    &p       = %p    p is itself a variable, with its own address\n",
               (void *)&p);
        printf("    sizeof p = %zu bytes       (every data pointer is this size here)\n",
               sizeof p);

        *p = 99;                      /* write through the pointer */
        printf("  after *p = 99:  x = %d   <- we changed x without naming it\n", x);
    }

    puts("\n=== THE * SYMBOL MEANS TWO DIFFERENT THINGS ===");
    puts("    int *p;      in a DECLARATION: 'p is a pointer to int'");
    puts("    *p = 5;      in an EXPRESSION: 'the thing p points at'");
    puts("  They are unrelated uses of the same character. This is why");
    puts("      int* a, b;    declares a POINTER and an INT — the * binds to `a`.");
    puts("  Write `int *a, *b;` or one declaration per line.");

    puts("\n=== & AND * ARE INVERSES ===");
    {
        int x = 7;
        int *p = &x;
        printf("  x        = %d\n", x);
        printf("  *&x      = %d   <- address-of then dereference gets you back\n", *&x);
        printf("  &*p == p ? %s   <- dereference then address-of is also a no-op\n",
               (&*p == p) ? "yes" : "no");
    }

    puts("\n=== POINTERS TO DIFFERENT TYPES ===");
    {
        char   c = 'A';
        int    i = 1000;
        double d = 3.14159;

        char   *pc = &c;
        int    *pi = &i;
        double *pd = &d;

        printf("  %-10s value %-12s addr %p  points at %zu byte(s)\n",
               "char *",   "'A'", (void *)pc, sizeof *pc);
        printf("  %-10s value %-12d addr %p  points at %zu byte(s)\n",
               "int *",    i,     (void *)pi, sizeof *pi);
        printf("  %-10s value %-12g addr %p  points at %zu byte(s)\n",
               "double *", d,     (void *)pd, sizeof *pd);
        puts("  Every pointer is the same SIZE (8 bytes here). The TYPE tells the");
        puts("  compiler how many bytes to read and how to interpret them.");
        puts("  That is the entire difference between char* and double*.");
    }

    puts("\n=== LOOKING AT AN OBJECT BYTE BY BYTE ===");
    {
        /* unsigned char * is the ONE pointer type allowed to alias anything.
         * That exemption is what makes byte inspection and memcpy legal. */
        int32_t v = 0x41424344;                 /* 'A''B''C''D' */
        const unsigned char *bytes = (const unsigned char *)&v;
        printf("  int32_t v = 0x%08X occupies bytes: ", v);
        for (size_t i = 0; i < sizeof v; i++) printf("%02X ", bytes[i]);
        printf("\n  as characters: ");
        for (size_t i = 0; i < sizeof v; i++) printf("'%c' ", bytes[i]);
        printf("\n  -> %s-endian machine\n", bytes[0] == 0x44 ? "little" : "big");
    }

    puts("\n=== NULL ===");
    {
        int *p = NULL;
        printf("  int *p = NULL;   p = %p\n", (void *)p);
        printf("  if (p)     -> %s\n", p ? "true" : "false");
        printf("  if (p != NULL) -> %s   (identical; the second reads better)\n",
               p != NULL ? "true" : "false");
        puts("  *p is UNDEFINED BEHAVIOUR. On Linux page 0 is unmapped so you get");
        puts("  SIGSEGV, which is the GOOD outcome — it stops immediately. On some");
        puts("  embedded targets address 0 is real memory and you corrupt it silently.");
        puts("");
        puts("  ALWAYS initialise pointers:");
        puts("      int *bad;        /* garbage address — writing through it is chaos */");
        puts("      int *good = NULL;/* a known-bad value you can TEST for */");
    }

    puts("\n=== WHY POINTERS EXIST: MODIFYING THE CALLER'S DATA ===");
    {
        int n = 1;
        cannot_modify(n);
        printf("  cannot_modify(n) : n = %d   (the function got a COPY)\n", n);
        can_modify(&n);
        printf("  can_modify(&n)   : n = %d   (the function got the ADDRESS)\n", n);
        puts("  C is always pass-by-value. Passing &n copies the ADDRESS, and an");
        puts("  address is enough to reach the original.");
    }

    puts("\n=== SWAP: the canonical example ===");
    {
        int a = 1, b = 2;
        printf("  before: a=%d b=%d\n", a, b);
        int t = a; a = b; b = t;                /* inline for clarity */
        printf("  after : a=%d b=%d\n", a, b);
        puts("  As a function it MUST take pointers:");
        puts("      void swap(int *x, int *y) { int t = *x; *x = *y; *y = t; }");
        puts("      swap(&a, &b);");
        puts("  Taking ints by value would swap two copies and change nothing.");
    }

    puts("\n=== OUT-PARAMETERS: returning more than one value ===");
    {
        int data[] = {5, -3, 12, 8, 0, -7, 21};
        size_t n = sizeof data / sizeof data[0];
        int lo, hi;

        if (minmax(data, n, &lo, &hi) == 0)
            printf("  minmax -> min=%d max=%d\n", lo, hi);
        if (minmax(data, n, NULL, &hi) == 0)
            printf("  max only (passed NULL for min) -> %d\n", hi);
        if (minmax(NULL, 0, &lo, &hi) != 0)
            puts("  minmax(NULL, 0) correctly refused");
        puts("  This is how the standard library works: strtol, fread, sscanf all");
        puts("  return a STATUS and write RESULTS through pointers.");
    }

    puts("\n=== HOW TO READ A DECLARATION: right to left, from the name ===");
    puts("    int *p;          p is a pointer to int");
    puts("    int **pp;        pp is a pointer to a pointer to int");
    puts("    int *a[10];      a is an array of 10 pointers to int");
    puts("    int (*a)[10];    a is a pointer to an array of 10 ints");
    puts("    int (*f)(int);   f is a pointer to a function (int) -> int");
    puts("    int *f(int);     f is a function (int) -> pointer to int");
    puts("  [] and () bind tighter than *, so the parentheses change everything.");
    puts("  When it gets ugly, introduce a typedef. Always.");

    return 0;
}
