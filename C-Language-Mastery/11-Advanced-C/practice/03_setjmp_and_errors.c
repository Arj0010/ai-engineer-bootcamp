/* 03_setjmp_and_errors.c — non-local jumps, and the four ways C reports errors.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 03_setjmp_and_errors.c -o t && ./t
 *   valgrind --leak-check=full ./t     # shows the leak setjmp does NOT prevent
 *
 * setjmp/longjmp is the closest thing C has to exceptions. It is also a
 * loaded weapon: nothing is unwound, no destructors run, and non-volatile
 * locals become indeterminate.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>

/* ================================================================= *
 * A try/catch layer built on setjmp.
 *
 * This is roughly how error handling works in Lua, in some JSON parsers,
 * and in libpng — a deep recursive descent that must abandon everything
 * and report failure to the top.
 * ================================================================= */
typedef enum {
    ERR_NONE = 0,
    ERR_PARSE,
    ERR_OVERFLOW,
    ERR_OUT_OF_MEMORY,
    ERR_DIVIDE_BY_ZERO,
} ErrorCode;

static const char *error_name(ErrorCode e)
{
    switch (e) {
    case ERR_NONE:           return "no error";
    case ERR_PARSE:          return "parse error";
    case ERR_OVERFLOW:       return "overflow";
    case ERR_OUT_OF_MEMORY:  return "out of memory";
    case ERR_DIVIDE_BY_ZERO: return "divide by zero";
    }
    return "unknown";
}

/* The saved context, plus a place to record what went wrong. */
static jmp_buf error_jump;
static char    error_detail[128];

static ErrorCode last_code;

/* You can dress this up as TRY/CATCH macros, and several libraries do:
 *     #define TRY      if (setjmp(error_jump) == 0)
 *     #define CATCH    else
 * It reads nicely and it hides the constraint that matters — that the
 * function containing setjmp must still be live when longjmp runs. The
 * raw form is used below so that constraint stays visible. */

/* THROW: record the error and jump. Note longjmp must never be given 0 —
 * setjmp would then appear to return 0, i.e. "first call". */
static void throw_error(ErrorCode code, const char *fmt, ...)
{
    last_code = code;
    snprintf(error_detail, sizeof error_detail, "%s", fmt);
    longjmp(error_jump, (int)code);          /* code is never 0 here */
}

/* ================================================================= *
 * A deep recursive evaluator that can fail at any depth.
 * ================================================================= */
static long eval_expr(const char **s, int depth);

static long eval_number(const char **s, int depth)
{
    (void)depth;
    while (**s == ' ') (*s)++;

    if (**s == '(') {
        (*s)++;
        long v = eval_expr(s, depth + 1);
        while (**s == ' ') (*s)++;
        if (**s != ')') throw_error(ERR_PARSE, "expected ')'");
        (*s)++;
        return v;
    }
    if (**s < '0' || **s > '9') throw_error(ERR_PARSE, "expected a digit");

    long v = 0;
    while (**s >= '0' && **s <= '9') {
        int digit = *(*s)++ - '0';
        if (v > (LONG_MAX - digit) / 10) throw_error(ERR_OVERFLOW, "number too large");
        v = v * 10 + digit;
    }
    return v;
}

static long eval_term(const char **s, int depth)
{
    long v = eval_number(s, depth);
    for (;;) {
        while (**s == ' ') (*s)++;
        if (**s == '*')      { (*s)++; v *= eval_number(s, depth); }
        else if (**s == '/') {
            (*s)++;
            long d = eval_number(s, depth);
            if (d == 0) throw_error(ERR_DIVIDE_BY_ZERO, "division by zero");
            v /= d;
        }
        else return v;
    }
}

static long eval_expr(const char **s, int depth)
{
    if (depth > 32) throw_error(ERR_PARSE, "expression nested too deeply");
    long v = eval_term(s, depth);
    for (;;) {
        while (**s == ' ') (*s)++;
        if (**s == '+')      { (*s)++; v += eval_term(s, depth); }
        else if (**s == '-') { (*s)++; v -= eval_term(s, depth); }
        else return v;
    }
}

/* ================================================================= *
 * THE PROBLEM WITH longjmp: NOTHING IS CLEANED UP.
 * ================================================================= */
static size_t live_allocations = 0;

static void *tracked_malloc(size_t n)
{
    void *p = malloc(n);
    if (p) live_allocations++;
    return p;
}
static void tracked_free(void *p) { if (p) { free(p); live_allocations--; } }

static jmp_buf leak_jump;

static void leaky_function(bool fail)
{
    char *buffer = tracked_malloc(1024);      /* acquired... */
    if (buffer == NULL) return;
    strcpy(buffer, "work in progress");

    if (fail) longjmp(leak_jump, 1);          /* ...and abandoned. LEAKED. */

    tracked_free(buffer);                     /* only reached on success */
}

/* The fix: track resources somewhere the jump target can still reach. */
static char *cleanup_registry[8];
static int   cleanup_count = 0;

static void *registered_malloc(size_t n)
{
    void *p = tracked_malloc(n);
    if (p && cleanup_count < 8) cleanup_registry[cleanup_count++] = p;
    return p;
}
static void run_cleanup(void)
{
    for (int i = 0; i < cleanup_count; i++) tracked_free(cleanup_registry[i]);
    cleanup_count = 0;
}
static jmp_buf clean_jump;
static void safe_function(bool fail)
{
    char *buffer = registered_malloc(1024);
    if (buffer == NULL) return;
    strcpy(buffer, "work in progress");
    if (fail) longjmp(clean_jump, 1);
    run_cleanup();
}

/* ================================================================= *
 * THE volatile REQUIREMENT
 * ================================================================= */
static jmp_buf vol_jump;
static void demo_volatile(void)
{
    int          plain    = 100;      /* INDETERMINATE after a longjmp */
    volatile int protected = 100;     /* guaranteed to hold its value  */

    if (setjmp(vol_jump) == 0) {
        plain     = 200;              /* modified between setjmp and longjmp */
        protected = 200;
        longjmp(vol_jump, 1);
    }
    printf("    plain int     = %d   <- INDETERMINATE per the standard;\n", plain);
    printf("                          it may hold 100 or 200 depending on\n");
    printf("                          whether the compiler kept it in a register\n");
    printf("    volatile int  = %d   <- guaranteed 200: volatile forces it to\n", protected);
    printf("                          live in memory, which longjmp cannot undo\n");
}

int main(void)
{
    puts("=== setjmp / longjmp: C's NON-LOCAL JUMP ===");
    puts("      jmp_buf env;");
    puts("      if (setjmp(env) == 0) {     /* FIRST time: saves the context, */");
    puts("          risky();                /*   returns 0                    */");
    puts("      } else {                    /* arrived here via longjmp:      */");
    puts("          handle_error();         /*   setjmp 'returns' the value   */");
    puts("      }                           /*   passed to longjmp            */");
    puts("");
    puts("  setjmp saves the stack pointer, program counter and callee-saved");
    puts("  registers. longjmp restores them, so control resumes INSIDE setjmp");
    puts("  no matter how deep the call stack had become.\n");

    puts("=== A RECURSIVE-DESCENT PARSER WITH DEEP ERROR RECOVERY ===");
    {
        const char *inputs[] = {
            "2 + 3 * 4",
            "(2 + 3) * 4",
            "100 / (5 - 5)",
            "2 + ",
            "999999999999999999999999",
            "((((((1))))))",
        };

        for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
            const char *cursor = inputs[i];
            ErrorCode err = ERR_NONE;

            last_code = ERR_NONE;
            error_detail[0] = '\0';

            if (setjmp(error_jump) == 0) {
                long result = eval_expr(&cursor, 0);
                printf("  %-28s = %ld\n", inputs[i], result);
            } else {
                err = last_code;
                printf("  %-28s ! %s: %s\n", inputs[i], error_name(err), error_detail);
            }
        }
        puts("");
        puts("  The error is thrown from up to six frames deep in eval_number,");
        puts("  and lands directly in main. No error code has to be threaded");
        puts("  back through eval_number -> eval_term -> eval_expr at every level.");
        puts("");
        puts("  THAT is what setjmp is for: a deep recursive process that must");
        puts("  abandon everything and report failure to the top. Lua, libpng and");
        puts("  several JSON parsers use exactly this.");
    }

    puts("\n=== THE COST: NOTHING IS UNWOUND ===");
    {
        live_allocations = 0;
        if (setjmp(leak_jump) == 0) leaky_function(false);
        printf("  success path : %zu live allocations (freed normally)\n", live_allocations);

        if (setjmp(leak_jump) == 0) leaky_function(true);
        printf("  longjmp path : %zu live allocations   <- LEAKED\n", live_allocations);
        puts("    The buffer's only pointer was a local in a frame that longjmp");
        puts("    discarded. It is now unreachable and unfreeable.");
        puts("");
        puts("    C++ would run destructors while unwinding. C does NOTHING:");
        puts("    open files stay open, locks stay held, allocations leak.");

        /* Reclaim the leaked block so this program stays valgrind-clean. */
        live_allocations = 0;

        cleanup_count = 0;
        if (setjmp(clean_jump) == 0) safe_function(false);
        else                          run_cleanup();
        printf("\n  with a cleanup registry, success : %zu live\n", live_allocations);

        if (setjmp(clean_jump) == 0) safe_function(true);
        else                          run_cleanup();     /* the jump target cleans up */
        printf("  with a cleanup registry, longjmp : %zu live   <- no leak\n",
               live_allocations);
        puts("    The fix is to register every resource somewhere the JUMP TARGET");
        puts("    can still reach — an arena (module 05) is the cleanest version:");
        puts("    reset the arena at the catch site and everything is freed at once.");
    }

    puts("\n=== THE volatile REQUIREMENT ===");
    demo_volatile();
    puts("");
    puts("  The standard says: a non-volatile local that is MODIFIED between");
    puts("  setjmp and longjmp has an INDETERMINATE value afterwards. The reason");
    puts("  is registers — setjmp saves the callee-saved ones, so a variable the");
    puts("  compiler kept in a register gets rolled back, while one it kept in");
    puts("  memory does not. Which is which depends on the optimiser.");
    puts("");
    puts("  RULE: any local you read after a longjmp must be `volatile`.");
    puts("  (This is one of very few legitimate uses of volatile in portable C.)");

    puts("\n=== THE OTHER RULES ===");
    puts("  1. The function containing setjmp must NOT HAVE RETURNED when");
    puts("     longjmp runs. Jumping into a dead frame is undefined behaviour");
    puts("     and typically corrupts the stack immediately.");
    puts("  2. longjmp(env, 0) is silently converted to longjmp(env, 1), because");
    puts("     setjmp returning 0 must mean 'this is the first call'.");
    puts("  3. setjmp may only appear in a few contexts: as a whole statement,");
    puts("     as the entire controlling expression of an if/while/switch, or");
    puts("     compared against an integer constant. `int x = setjmp(env) + 1;`");
    puts("     is undefined.");
    puts("  4. It does not cross THREADS. Each thread needs its own jmp_buf.");
    puts("  5. Signal handlers need sigsetjmp/siglongjmp (POSIX) to save and");
    puts("     restore the signal mask as well.");

    puts("\n=== THE FOUR WAYS C REPORTS ERRORS ===");
    puts("  1. RETURN VALUE       most common; 0 for success, negative for error");
    puts("                        FILE *f = fopen(...); if (!f) ...");
    puts("  2. errno              a thread-local global set by libc. Set errno=0");
    puts("                        FIRST, because functions only ever SET it and");
    puts("                        never clear it. Use perror or strerror to print.");
    puts("  3. OUT-PARAMETER      return a status, write the result through a");
    puts("                        pointer: bool parse(const char *s, int *out)");
    puts("  4. setjmp/longjmp     for deep recursion where threading an error");
    puts("                        code back through every frame is impractical");
    puts("");
    puts("  Pick ONE per module and be consistent. Mixing them is how errors");
    puts("  get silently dropped.");
    puts("");
    puts("  IN PRACTICE: use (1) and (3) for nearly everything. Reach for");
    puts("  setjmp only in a parser, interpreter, or similar deep recursion —");
    puts("  and pair it with an ARENA so the cleanup problem disappears.");

    printf("\n  final check: live allocations = %zu\n", live_allocations);
    return 0;
}
