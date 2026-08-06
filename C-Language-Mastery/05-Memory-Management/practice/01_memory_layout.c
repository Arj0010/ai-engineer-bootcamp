/* 01_memory_layout.c — where every kind of variable actually lives.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 01_memory_layout.c -o t && ./t
 *
 * Compare with:  size ./t        (segment sizes in the executable)
 *                cat /proc/self/maps  (the live map, on Linux)
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* .rodata — read-only. Writing here is UB and usually SIGSEGV. */
static const char  RODATA_STRING[] = "in .rodata";
static const int   RODATA_TABLE[4] = {1, 2, 3, 4};

/* .data — initialised to something non-zero. These bytes are stored IN the
 * executable file, so a big initialised array makes your binary big. */
static int   data_int   = 12345;
static char  data_array[64] = "initialised";

/* .bss — no initialiser, or an all-zero one. Guaranteed zeroed before main.
 * Costs ZERO bytes in the file; the loader just maps zeroed pages. */
static int   bss_int;
static char  bss_array[4096];

static void recurse(int depth, const int *first_frame);

int main(void)
{
    /* stack — automatic variables */
    int    stack_int    = 1;
    char   stack_array[256];
    double stack_double = 2.0;

    /* heap — malloc */
    void *heap_small = malloc(16);
    void *heap_large = malloc(1024 * 1024);      /* 1 MB — often mmap'd separately */

    puts("=== PROCESS MEMORY LAYOUT (actual addresses this run) ===\n");

    printf("  %-28s %18p   %s\n", ".text (code)  main",
           (void *)(uintptr_t)main, "read + execute");
    printf("  %-28s %18p   %s\n", ".rodata  string literal",
           (const void *)RODATA_STRING, "READ-ONLY");
    printf("  %-28s %18p\n", ".rodata  const table", (const void *)RODATA_TABLE);
    printf("  %-28s %18p   %s\n", ".data    data_int",
           (void *)&data_int, "stored in the binary");
    printf("  %-28s %18p\n", ".data    data_array", (void *)data_array);
    printf("  %-28s %18p   %s\n", ".bss     bss_int",
           (void *)&bss_int, "zeroed by the loader");
    printf("  %-28s %18p   %s\n", ".bss     bss_array[4096]",
           (void *)bss_array, "costs 0 bytes in the file");
    printf("  %-28s %18p   %s\n", "HEAP     malloc(16)",
           heap_small, "grows UP");
    printf("  %-28s %18p   %s\n", "HEAP     malloc(1 MB)",
           heap_large, "large blocks may be mmap'd elsewhere");
    printf("  %-28s %18p   %s\n", "STACK    stack_int",
           (void *)&stack_int, "grows DOWN");
    printf("  %-28s %18p\n", "STACK    stack_array[256]", (void *)stack_array);
    printf("  %-28s %18p\n", "STACK    stack_double", (void *)&stack_double);

    puts("\n=== WHAT THE ADDRESSES TELL YOU ===");
    printf("  stack is above the heap by roughly %.1f GB\n",
           ((double)(uintptr_t)&stack_int - (double)(uintptr_t)heap_small) / 1e9);
    puts("  Code and constants sit lowest, then initialised data, then the heap");
    puts("  growing upward, then a huge unmapped gap, then the stack growing down.");
    puts("  The gap is what lets both grow without colliding. Exhaust it and you");
    puts("  get either a stack overflow or a failed malloc.");
    puts("  (Addresses differ every run because of ASLR — that is deliberate,");
    puts("   it makes memory-corruption exploits much harder.)");

    puts("\n=== VERIFY THE ZERO GUARANTEE ===");
    printf("  bss_int      = %d      <- GUARANTEED 0, never written by us\n", bss_int);
    printf("  bss_array[0] = %d      <- likewise, all 4096 bytes\n", bss_array[0]);
    printf("  data_int     = %d  <- initialiser stored in the executable\n", data_int);
    puts("  Automatic variables get NO such guarantee. They hold whatever the");
    puts("  previous stack frame left behind.");

    puts("\n=== WHICH DIRECTION DOES THE STACK GROW? ===");
    recurse(0, &stack_int);

    puts("\n=== STACK vs HEAP: choosing ===");
    puts("                    STACK                     HEAP");
    puts("  allocate          1 instruction (sub rsp)   ~100s of cycles");
    puts("  free              automatic at scope exit   you must call free()");
    puts("  max size          ~8 MB total (ulimit -s)   available RAM");
    puts("  size known        compile time              run time");
    puts("  lifetime          the enclosing block       until free()");
    puts("  fragmentation     never                     yes");
    puts("  failure           SIGSEGV, no warning       malloc returns NULL");
    puts("");
    puts("  DEFAULT TO THE STACK. Reach for the heap when:");
    puts("    - the object is large (rule of thumb: over ~100 KB)");
    puts("    - the size is only known at run time");
    puts("    - the object must outlive the function that created it");

    puts("\n=== HOW BIG IS THE STACK? ===");
    puts("  Linux/macOS default: 8 MB. Check with `ulimit -s`.");
    puts("  Threads get much less — often 512 KB to 2 MB (see pthread_attr_setstacksize).");
    puts("  Blow it and you get SIGSEGV with no allocation error, because there");
    puts("  is no allocator involved: the CPU just faults on an unmapped page.");
    puts("  Causes: unbounded recursion, or a huge local array:");
    puts("      void f(void) { int big[4 * 1000 * 1000]; }   /* 16 MB — instant death */");

    puts("\n=== SEGMENT SIZES OF THIS BINARY ===");
    puts("  Run:  size ./t");
    puts("     text = machine code + .rodata");
    puts("     data = initialised statics (stored in the file)");
    puts("     bss  = zeroed statics (NOT stored — just a size to allocate)");
    puts("  That is why bss_array[4096] above adds 4 KB to memory but ~0 to the file.");

    free(heap_small);
    free(heap_large);
    return 0;
}

/* Compare frame addresses across recursion depth to see which way the stack
 * grows. `volatile` stops the optimiser from eliding the frames. */
static void recurse(int depth, const int *first_frame)
{
    volatile int frame_marker = depth;

    if (depth < 3) {
        printf("  depth %d: local at %p", depth, (void *)&frame_marker);
        if (depth > 0)
            printf("  (%+td bytes from main's frame)",
                   (const char *)&frame_marker - (const char *)first_frame);
        puts("");
        recurse(depth + 1, first_frame);
    } else {
        printf("  depth %d: local at %p\n", depth, (void *)&frame_marker);
        puts("  Addresses DECREASE with depth -> the stack grows toward LOW");
        puts("  addresses on x86-64 and ARM64. Each call subtracts from rsp.");
    }
}
