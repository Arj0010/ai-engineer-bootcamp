/* 06_pointer_bugs.c — a catalogue of every way pointers go wrong.
 *
 * Each bug is guarded behind a command-line flag so the program is safe to
 * run by default. Trigger them ONE AT A TIME under a sanitizer and read the
 * report — that report is the skill this file is teaching.
 *
 *   gcc -std=c17 -Wall -Wextra -g -O0 -fsanitize=address,undefined \
 *       06_pointer_bugs.c -o bugs
 *   ./bugs                # safe: explains every bug, triggers none
 *   ./bugs list           # show the flags
 *   ./bugs uaf            # use-after-free      -> ASan: heap-use-after-free
 *   ./bugs doublefree     # double free         -> ASan: attempting double-free
 *   ./bugs overflow       # heap overflow       -> ASan: heap-buffer-overflow
 *   ./bugs stackoverflow  # stack overflow      -> ASan: stack-buffer-overflow
 *   ./bugs dangling       # returning &local    -> ASan: stack-use-after-return
 *   ./bugs uninit         # uninitialised read  -> MSan / valgrind
 *   ./bugs nullderef      # NULL dereference    -> SIGSEGV
 *   ./bugs leak           # memory leak         -> ASan LeakSanitizer at exit
 *   ./bugs offbyone       # classic <= bound    -> ASan: heap-buffer-overflow
 *   ./bugs aliasing       # strict aliasing     -> UBSan / wrong answer at -O2
 *
 * Also try:  valgrind --leak-check=full ./bugs leak
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- */
static void bug_use_after_free(void)
{
    int *p = malloc(4 * sizeof *p);
    if (!p) return;
    p[0] = 42;
    free(p);
    /* p still holds the old address, but the block belongs to the allocator
     * again. Reading it may return old data, new data, or allocator metadata. */
    printf("  reading freed memory: %d\n", p[0]);      /* BUG */
}

static void bug_double_free(void)
{
    int *p = malloc(4 * sizeof *p);
    if (!p) return;
    free(p);
    free(p);   /* BUG: corrupts the allocator's free list. Historically this
                * was exploitable into arbitrary code execution. */
}

static void bug_heap_overflow(void)
{
    int *p = malloc(4 * sizeof *p);      /* room for exactly 4 ints */
    if (!p) return;
    for (int i = 0; i <= 4; i++) p[i] = i;   /* BUG: <= writes p[4], the 5th */
    printf("  wrote 5 ints into a 4-int block\n");
    free(p);
}

static void bug_stack_overflow(void)
{
    char buf[8];
    const char *src = "far too long for eight bytes";
    strcpy(buf, src);            /* BUG: writes past buf into the frame */
    printf("  %s\n", buf);
}

/* Returning the address of an automatic variable. The frame is reclaimed the
 * moment the function returns, so the pointer refers to memory that the next
 * call will reuse. GCC warns (-Wreturn-local-addr); this goes through a
 * pointer variable so the demo still builds. */
static int *bug_return_local(void)
{
    int local = 12345;
    int *p = &local;
    return p;                    /* BUG: dangling the instant we return */
}
static void clobber_the_stack(void) { volatile int junk[64]; for (int i = 0; i < 64; i++) junk[i] = -1; }

static void bug_uninitialised(void)
{
    int  *p = malloc(4 * sizeof *p);     /* malloc does NOT zero memory */
    if (!p) return;
    printf("  malloc'd but unwritten: %d %d (garbage)\n", p[0], p[1]);  /* BUG */
    free(p);

    int stack_garbage;
    printf("  uninitialised local: %d (garbage)\n", stack_garbage);      /* BUG */
}

static void bug_null_deref(void)
{
    int *p = NULL;
    printf("  about to dereference NULL...\n");
    fflush(stdout);
    printf("  %d\n", *p);        /* BUG: SIGSEGV on any hosted OS */
}

static void bug_leak(void)
{
    for (int i = 0; i < 3; i++) {
        char *p = malloc(1024);
        if (!p) return;
        snprintf(p, 1024, "block %d", i);
        /* BUG: no free. The pointer goes out of scope and the memory is
         * unreachable — 3 KB lost for the rest of the process's life. */
    }
    printf("  leaked 3 KB (LeakSanitizer reports it at exit)\n");
}

static void bug_off_by_one(void)
{
    size_t n = 10;
    int *a = malloc(n * sizeof *a);
    if (!a) return;
    /* Valid indices are 0..n-1. `<=` visits n, one past the end. */
    for (size_t i = 0; i <= n; i++) a[i] = (int)i;    /* BUG */
    printf("  off-by-one write to a[%zu]\n", n);
    free(a);
}

static void bug_strict_aliasing(void)
{
    /* Accessing a float through an int* violates the aliasing rules. The
     * compiler assumes the two pointers cannot refer to the same object and
     * may reorder or cache the loads. At -O2 this prints the wrong value. */
    float f = 1.5f;
    int  *ip = (int *)&f;                            /* BUG */
    printf("  float 1.5 read through int*: %d\n", *ip);
    printf("  the CORRECT way (memcpy):    ");
    int bits;
    memcpy(&bits, &f, sizeof bits);
    printf("%d\n", bits);
    puts("  Both may print the same thing today. The first is still undefined,");
    puts("  and the optimiser is entitled to break it in any future build.");
}

/* ---------------------------------------------------------------- */
static void explain(void)
{
    puts("=== THE POINTER FAILURE MODES ===\n");

    puts("1. USE AFTER FREE");
    puts("   free(p) returns the block to the allocator; p still holds the address.");
    puts("   Reading gives stale or reused data; writing corrupts another object.");
    puts("   FIX: p = NULL immediately after free. Better: free_and_null(&p).");
    puts("   CATCH: ASan 'heap-use-after-free' with both stack traces.\n");

    puts("2. DOUBLE FREE");
    puts("   Corrupts the allocator's own bookkeeping. Historically escalated");
    puts("   into arbitrary code execution.");
    puts("   FIX: NULL after free — free(NULL) is a defined no-op.");
    puts("   CATCH: ASan 'attempting double-free', glibc abort.\n");

    puts("3. BUFFER OVERFLOW (heap or stack)");
    puts("   Writing past the end of an allocation. On the stack it can reach the");
    puts("   saved return address, which is the classic exploit primitive.");
    puts("   FIX: carry the size with every buffer; use snprintf; check bounds.");
    puts("   CATCH: ASan 'heap-buffer-overflow' / 'stack-buffer-overflow'.\n");

    puts("4. DANGLING POINTER TO A LOCAL");
    puts("   Returning &local. The frame is reclaimed on return; the next call");
    puts("   overwrites it. The value often survives just long enough to fool you.");
    puts("   FIX: return by value, malloc it, or take a caller-supplied buffer.");
    puts("   CATCH: -Wreturn-local-addr; ASan 'stack-use-after-return'");
    puts("          (needs ASAN_OPTIONS=detect_stack_use_after_return=1).\n");

    puts("5. UNINITIALISED READ");
    puts("   malloc does NOT zero memory; neither do automatic variables.");
    puts("   FIX: calloc, or memset, or initialise at declaration.");
    puts("   CATCH: valgrind 'uninitialised value', MemorySanitizer, -Wmaybe-uninitialized.\n");

    puts("6. NULL DEREFERENCE");
    puts("   Almost always an unchecked malloc or an unchecked lookup.");
    puts("   FIX: check every allocation and every 'find' that can miss.");
    puts("   CATCH: SIGSEGV; UBSan reports it with a source line.\n");

    puts("7. MEMORY LEAK");
    puts("   The last pointer to a block goes out of scope. Not a crash — a slow");
    puts("   death. Fatal in anything long-running.");
    puts("   FIX: one owner per allocation, documented; goto-cleanup on error paths.");
    puts("   CATCH: ASan LeakSanitizer at exit; valgrind --leak-check=full.\n");

    puts("8. OFF BY ONE");
    puts("   `<=` where `<` was meant. Valid indices are 0..n-1, always.");
    puts("   CATCH: ASan. It is one byte, and ASan finds it instantly.\n");

    puts("9. STRICT ALIASING VIOLATION");
    puts("   Reading an object through a pointer of an incompatible type.");
    puts("   FIX: memcpy (compiles to the same instruction) or a union.");
    puts("   CATCH: -Wstrict-aliasing, or a wrong answer that only appears at -O2.");
    puts("   NOTE: char* and unsigned char* may alias anything — that exemption");
    puts("         is what makes byte inspection and memcpy legal.\n");

    puts("=== THE DEFENSIVE HABITS ===");
    puts("  1. Initialise every pointer, to NULL if nothing better.");
    puts("  2. Check every malloc/calloc/realloc for NULL.");
    puts("  3. Set pointers to NULL immediately after freeing.");
    puts("  4. One owner per allocation. Write down who frees it, in the header.");
    puts("  5. Pass sizes alongside pointers, always.");
    puts("  6. Develop with -fsanitize=address,undefined. Always.");
    puts("  7. Run valgrind before shipping.");
    puts("  8. Use the goto-cleanup pattern so every path frees everything.\n");

    puts("Run './bugs list' for the flags that trigger each one.");
}

int main(int argc, char **argv)
{
    if (argc < 2) { explain(); return 0; }

    const char *which = argv[1];

    if (strcmp(which, "list") == 0) {
        puts("  uaf doublefree overflow stackoverflow dangling");
        puts("  uninit nullderef leak offbyone aliasing");
        puts("\n  build first:");
        puts("    gcc -std=c17 -Wall -Wextra -g -O0 -fsanitize=address,undefined \\");
        puts("        06_pointer_bugs.c -o bugs");
        return 0;
    }

    printf("*** triggering: %s ***\n", which);

    if      (strcmp(which, "uaf")          == 0) bug_use_after_free();
    else if (strcmp(which, "doublefree")   == 0) bug_double_free();
    else if (strcmp(which, "overflow")     == 0) bug_heap_overflow();
    else if (strcmp(which, "stackoverflow")== 0) bug_stack_overflow();
    else if (strcmp(which, "uninit")       == 0) bug_uninitialised();
    else if (strcmp(which, "nullderef")    == 0) bug_null_deref();
    else if (strcmp(which, "leak")         == 0) bug_leak();
    else if (strcmp(which, "offbyone")     == 0) bug_off_by_one();
    else if (strcmp(which, "aliasing")     == 0) bug_strict_aliasing();
    else if (strcmp(which, "dangling")     == 0) {
        int *p = bug_return_local();
        clobber_the_stack();                 /* reuse the frame p points into */
        printf("  value through a dangling pointer: %d (was 12345)\n", *p);
    }
    else { printf("unknown: %s (try 'list')\n", which); return 1; }

    puts("*** done — if nothing was reported, rebuild with -fsanitize=address ***");
    return 0;
}
