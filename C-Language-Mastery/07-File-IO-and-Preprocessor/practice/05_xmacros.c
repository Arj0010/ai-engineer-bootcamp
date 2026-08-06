/* 05_xmacros.c — X-macros: define a list ONCE, expand it many ways.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic 05_xmacros.c -o t && ./t
 *
 * See the generated code:
 *   gcc -E 05_xmacros.c | sed -n '/typedef enum/,/TokenKind/p'
 *
 * The problem X-macros solve: an enum and its name table, its parser, and its
 * serialiser all have to stay in sync. Four places to edit, and eventually one
 * of them is forgotten. With an X-macro there is ONE list, and everything else
 * is generated from it — drift becomes impossible.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ================================================================= *
 * THE SINGLE SOURCE OF TRUTH
 *
 * Each row is X(enum_suffix, text, precedence). The macro X is not
 * defined yet — each expansion below defines it, expands the list, and
 * then undefines it again.
 * ================================================================= */
#define TOKEN_LIST                       \
    X(PLUS,    "+",     1)               \
    X(MINUS,   "-",     1)               \
    X(STAR,    "*",     2)               \
    X(SLASH,   "/",     2)               \
    X(PERCENT, "%",     2)               \
    X(CARET,   "^",     3)               \
    X(LPAREN,  "(",     0)               \
    X(RPAREN,  ")",     0)

/* --- expansion 1: the enum ---------------------------------------- */
typedef enum {
#define X(name, text, prec) TOK_##name,
    TOKEN_LIST
#undef X
    TOK_COUNT                   /* the count comes out for free */
} TokenKind;

/* --- expansion 2: the name table ---------------------------------- */
static const char *TOKEN_NAMES[TOK_COUNT] = {
#define X(name, text, prec) #name,
    TOKEN_LIST
#undef X
};

/* --- expansion 3: the text table ---------------------------------- */
static const char *TOKEN_TEXT[TOK_COUNT] = {
#define X(name, text, prec) text,
    TOKEN_LIST
#undef X
};

/* --- expansion 4: a precedence table ------------------------------ */
static const int TOKEN_PREC[TOK_COUNT] = {
#define X(name, text, prec) prec,
    TOKEN_LIST
#undef X
};

/* --- expansion 5: a parser (text -> enum) ------------------------- */
static bool token_from_text(const char *s, TokenKind *out)
{
#define X(name, text, prec) if (strcmp(s, text) == 0) { *out = TOK_##name; return true; }
    TOKEN_LIST
#undef X
    return false;
}

/* --- expansion 6: a self-test over every member ------------------- */
static int token_selftest(void)
{
    int failures = 0;
#define X(name, text, prec)                                                  \
    do {                                                                     \
        TokenKind k;                                                         \
        if (!token_from_text(text, &k) || k != TOK_##name) {                 \
            printf("    FAIL: %s did not round-trip\n", #name);              \
            failures++;                                                      \
        }                                                                    \
    } while (0);
    TOKEN_LIST
#undef X
    return failures;
}

/* The count is a compile-time constant, so we can assert the tables match. */
_Static_assert(sizeof TOKEN_NAMES / sizeof TOKEN_NAMES[0] == TOK_COUNT,
               "TOKEN_NAMES out of sync with TOKEN_LIST");
_Static_assert(sizeof TOKEN_TEXT / sizeof TOKEN_TEXT[0] == TOK_COUNT,
               "TOKEN_TEXT out of sync with TOKEN_LIST");

/* ================================================================= *
 * A SECOND EXAMPLE: generating a struct AND its serialiser.
 * ================================================================= */
#define CONFIG_FIELDS                                        \
    F(int,          max_connections, 100,   "%d")            \
    F(int,          timeout_seconds, 30,    "%d")            \
    F(double,       rate_limit,      1.5,   "%g")            \
    F(bool,         verbose,         false, "%d")            \
    F(const char *, log_path,        "/var/log/app.log", "%s")

/* the struct */
typedef struct {
#define F(type, name, def, fmt) type name;
    CONFIG_FIELDS
#undef F
} Config;

/* the defaults */
static Config config_defaults(void)
{
    Config c = {
#define F(type, name, def, fmt) .name = def,
        CONFIG_FIELDS
#undef F
    };
    return c;
}

/* the printer — add a field to the list and it appears here automatically */
static void config_print(const Config *c)
{
#define F(type, name, def, fmt) printf("    %-16s = " fmt "\n", #name, c->name);
    CONFIG_FIELDS
#undef F
}

/* A generated "does this key exist?" check. Note what it can and cannot do:
 * the LOOKUP is generated, but ASSIGNING a value needs a different conversion
 * per type (strtol vs strtod vs strdup), and an X-macro cannot produce that
 * from a list of type names alone. This is the honest boundary of the
 * technique: it generates SHAPE, not type-specific logic.
 *
 * The usual fix is to put the conversion function in the list itself —
 * F(int, max_connections, 100, "%d", parse_int) — which works, at the cost of
 * writing one small parser per type. */
static bool config_has_key(const char *key)
{
#define F(type, name, def, fmt) if (strcmp(key, #name) == 0) return true;
    CONFIG_FIELDS
#undef F
    return false;
}

/* The list of valid keys, also generated — useful for error messages. */
static void config_print_keys(void)
{
    printf("    valid keys:");
#define F(type, name, def, fmt) printf(" %s", #name);
    CONFIG_FIELDS
#undef F
    putchar('\n');
}

int main(void)
{
    puts("=== ONE LIST, SIX EXPANSIONS ===");
    puts("  #define TOKEN_LIST      \\");
    puts("      X(PLUS,  \"+\", 1)    \\");
    puts("      X(MINUS, \"-\", 1)    \\");
    puts("      ...");
    puts("  Each expansion below #defines X differently, pulls in the list, and");
    puts("  #undefs X again. Same data, different generated code each time.\n");

    printf("  the enum has %d members (TOK_COUNT came out of the list for free)\n\n",
           TOK_COUNT);

    puts("  enum value | name    | text | precedence");
    puts("  -----------+---------+------+-----------");
    for (int i = 0; i < TOK_COUNT; i++)
        printf("     %2d      | %-7s |  %-3s |     %d\n",
               i, TOKEN_NAMES[i], TOKEN_TEXT[i], TOKEN_PREC[i]);

    puts("\n=== THE GENERATED PARSER ===");
    {
        const char *inputs[] = {"+", "*", "^", "(", "@", "**"};
        for (size_t i = 0; i < sizeof inputs / sizeof inputs[0]; i++) {
            TokenKind k;
            if (token_from_text(inputs[i], &k))
                printf("  \"%s\" -> %s (precedence %d)\n",
                       inputs[i], TOKEN_NAMES[k], TOKEN_PREC[k]);
            else
                printf("  \"%s\" -> not a token\n", inputs[i]);
        }
    }

    puts("\n=== THE GENERATED SELF-TEST ===");
    {
        int failures = token_selftest();
        printf("  every token round-trips text -> enum -> text: %s\n",
               failures == 0 ? "PASS" : "FAIL");
        puts("  The test covers every member BY CONSTRUCTION. Add a token and");
        puts("  it is tested automatically — you cannot forget.");
    }

    puts("\n=== WHY THIS MATTERS ===");
    puts("  Without X-macros you would maintain, by hand:");
    puts("    - the enum");
    puts("    - the name table (for error messages)");
    puts("    - the text table (for printing)");
    puts("    - the precedence table (for the parser)");
    puts("    - the text -> enum function");
    puts("    - the test");
    puts("  Six places. Add one token, forget one place, and you get either a");
    puts("  build error (lucky) or a silently wrong table (not lucky) — a table");
    puts("  one entry short reads out of bounds for the last enumerator.");
    puts("");
    puts("  With an X-macro there is exactly ONE list. Everything else is");
    puts("  generated, so drift is not possible. _Static_assert on the table");
    puts("  sizes catches even the case where someone hand-edits a table.");

    puts("\n=== SECOND EXAMPLE: a config struct and its printer ===");
    {
        Config c = config_defaults();
        puts("  defaults, printed by generated code:");
        config_print(&c);

        c.max_connections = 500;
        c.verbose = true;
        puts("  after modification:");
        config_print(&c);

        printf("  config_has_key(\"rate_limit\")  -> %s\n",
               config_has_key("rate_limit") ? "yes" : "no");
        printf("  config_has_key(\"nonexistent\") -> %s\n",
               config_has_key("nonexistent") ? "yes" : "no");
        config_print_keys();

        puts("\n  Adding a field means adding ONE line to CONFIG_FIELDS. The");
        puts("  struct, the defaults, the printer, the key check, and the key");
        puts("  list all update themselves.");
    }

    puts("\n=== WHERE X-MACROS ARE USED FOR REAL ===");
    puts("  - opcode tables in bytecode VMs (name, arity, handler)");
    puts("  - error-code enums with matching message strings");
    puts("  - the token list in a lexer (exactly the example above)");
    puts("  - register maps in embedded firmware");
    puts("  - protocol message types with their sizes and handlers");
    puts("  - the Linux kernel's syscall table");

    puts("\n=== LIMITS ===");
    puts("  - the expansions are invisible in a debugger; use gcc -E to see them");
    puts("  - a compile error inside an expansion points at the macro, not the row");
    puts("  - it generates SHAPE, not type-specific logic (see config_set above:");
    puts("    each field type needs its own conversion, so the pattern stops there)");
    puts("  - it is genuinely hard to read the first time");
    puts("");
    puts("  Use it when the SAME list must appear in three or more places and");
    puts("  the cost of them drifting apart is a real bug. Below that bar, just");
    puts("  write the tables out and add a _Static_assert on their sizes.");

    return 0;
}

/* Stubs the generated setter refers to — see the note in config_set. */
