/* 03_scope_and_storage.c — scope, lifetime, and linkage are THREE separate things.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 03_scope_and_storage.c -o t && ./t
 *
 * The address printouts show you the actual memory layout: globals and statics
 * cluster together low in memory, locals sit high on the stack, and heap
 * allocations land in between.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* ================= FILE SCOPE ================= */

/* External linkage: visible to any other .c file that declares it `extern`.
 * Zero-initialised before main runs — this is GUARANTEED, unlike locals. */
int global_visible_everywhere = 42;

/* Internal linkage: this name exists only in this translation unit. Another
 * .c file may define its own `static int file_private` with no conflict.
 * THIS SHOULD BE YOUR DEFAULT for anything not declared in a header. */
static int file_private = 7;

/* Uninitialised globals go in .bss (zero-filled at load time, costs no
 * bytes in the executable). Initialised ones go in .data. */
static int bss_variable;              /* guaranteed 0 */
static int data_variable = 12345;

/* A string LITERAL lives in read-only memory (.rodata). Writing through this
 * pointer is undefined behaviour and typically segfaults. */
static const char *literal = "I live in .rodata";

/* An ARRAY initialised from a literal is a modifiable copy in .data. */
static char writable_copy[] = "I am a modifiable array";

static void show_addresses(void);

/* ================= static-inside-a-function ================= */

/* The OTHER meaning of `static`: one instance for the whole program,
 * initialised exactly once, retains its value between calls.
 * Lifetime: whole program.  Scope: this function only.  Linkage: none. */
static int next_id(void)
{
    static int id = 0;            /* initialised once, at program start */
    return ++id;
}

/* A cheap way to run something only the first time a function is called. */
static void expensive_setup_once(void)
{
    static bool initialised = false;
    if (initialised) { puts("    (setup already done, skipping)"); return; }
    puts("    running one-time setup...");
    initialised = true;
}

/* Contrast: an AUTOMATIC local is recreated on every call, so this
 * counter never gets past 1. */
static int broken_counter(void)
{
    int id = 0;                   /* new object every call */
    return ++id;
}

/* THE CLASSIC BUG: returning the address of an automatic variable.
 * The object is destroyed when the function returns; the pointer dangles.
 * GCC warns about the obvious case, which is why this one is built
 * indirectly — but the returned pointer is still invalid. */
static const char *safe_returns_static(void)
{
    static char buf[] = "static buffer — outlives the call, safe to return";
    return buf;                   /* fine: static lifetime */
}
static const char *safe_returns_literal(void)
{
    return "string literal — .rodata, lives forever, safe to return";
}

/* ================= shadowing ================= */
static int shadow_demo(void)
{
    int x = 1;                    /* function-block scope */
    printf("    outer block: x = %d\n", x);
    {
        int x = 2;                /* SHADOWS the outer x. Legal, and confusing. */
        printf("    inner block: x = %d  <- a different object entirely\n", x);
        {
            int x = 3;
            printf("    innermost : x = %d\n", x);
        }
    }
    printf("    back outside: x = %d  <- the outer one was never touched\n", x);
    return x;                     /* -Wshadow warns about all of this */
}

int main(void)
{
    puts("=== THE THREE INDEPENDENT QUESTIONS ===");
    puts("  SCOPE    — where the NAME is visible      (block / file)");
    puts("  LIFETIME — how long the OBJECT exists     (automatic / static / allocated)");
    puts("  LINKAGE  — is it the same object in other .c files? (none / internal / external)");
    puts("  These are decided separately. `static` changes two of them, differently,");
    puts("  depending on where you write it.\n");

    puts("=== `static` INSIDE a function: persists across calls ===");
    printf("  next_id()       : %d %d %d %d\n", next_id(), next_id(), next_id(), next_id());
    puts("  (argument evaluation order is unspecified, so those may print in any");
    puts("   order — but they are four DIFFERENT ids, which is the point.)");
    printf("  broken_counter(): %d %d %d %d  <- automatic local, resets every call\n",
           broken_counter(), broken_counter(), broken_counter(), broken_counter());

    puts("\n=== one-time initialisation with a static flag ===");
    expensive_setup_once();
    expensive_setup_once();
    expensive_setup_once();

    puts("\n=== INITIALISATION GUARANTEES ===");
    {
        int uninitialised_local;                 /* GARBAGE. Reading it is UB. */
        printf("  static/global int  : %d  <- GUARANTEED zero before main runs\n",
               bss_variable);
        printf("  static with value  : %d\n", data_variable);
        puts("  automatic local    : UNDEFINED — whatever was on the stack.");
        puts("                       Reading it is undefined behaviour; the value");
        puts("                       often looks plausible, which makes it worse.");
        /* Deliberately not printing it: reading it really is UB. */
        (void)uninitialised_local;
        puts("  RULE: initialise every local at the point of declaration.");
    }

    puts("\n=== SHADOWING (compile with -Wshadow to be warned) ===");
    shadow_demo();

    puts("\n=== SAFE vs UNSAFE returns ===");
    printf("  %s\n", safe_returns_static());
    printf("  %s\n", safe_returns_literal());
    puts("  UNSAFE: returning &local — the frame is gone, the pointer dangles.");
    puts("      char *bad(void) { char buf[32]; return buf; }   /* never do this */");
    puts("  The three correct options: return a static, malloc it (caller frees),");
    puts("  or take a caller-provided buffer as a parameter (best).");

    puts("\n=== STRING LITERALS ARE READ-ONLY ===");
    printf("  const char *p = \"...\";  -> \"%s\"\n", literal);
    printf("  char arr[]    = \"...\";  -> \"%s\"\n", writable_copy);
    writable_copy[0] = 'i';                  /* fine: it is a modifiable array */
    printf("  after arr[0] = 'i'      -> \"%s\"\n", writable_copy);
    puts("  literal[0] = 'x';  <- would be UNDEFINED BEHAVIOUR (usually SIGSEGV).");
    puts("  Always declare literal pointers as `const char *`.");

    show_addresses();

    puts("\n=== SUMMARY TABLE ===");
    puts("  where declared        lifetime    linkage    memory");
    puts("  --------------------  ----------  ---------  ---------------");
    puts("  local (no keyword)    automatic   none       stack");
    puts("  local + static        program     none       .data / .bss");
    puts("  file scope + static   program     internal   .data / .bss");
    puts("  file scope, plain     program     external   .data / .bss");
    puts("  extern declaration    program     external   defined elsewhere");
    puts("  malloc'd              until free  n/a        heap");

    return 0;
}

static void show_addresses(void)
{
    int    automatic_local = 0;
    static int static_local = 0;
    void  *heap = malloc(16);

    puts("\n=== WHERE THINGS ACTUALLY LIVE (addresses on this run) ===");
    printf("  .rodata  literal          %p\n", (const void *)literal);
    printf("  .data    data_variable    %p\n", (void *)&data_variable);
    printf("  .bss     bss_variable     %p\n", (void *)&bss_variable);
    printf("  .data    global           %p\n", (void *)&global_visible_everywhere);
    printf("  .data    file_private      %p  (value %d, internal linkage)\n",
           (void *)&file_private, file_private);
    printf("  .bss     static local     %p\n", (void *)&static_local);
    printf("  heap     malloc(16)       %p\n", heap);
    printf("  stack    automatic local  %p\n", (void *)&automatic_local);
    puts("  Low addresses: code and static data. High addresses: the stack,");
    puts("  which grows DOWNWARD. The heap grows upward from just above .bss.");
    puts("  (Exact values differ every run because of ASLR.)");

    free(heap);
}
