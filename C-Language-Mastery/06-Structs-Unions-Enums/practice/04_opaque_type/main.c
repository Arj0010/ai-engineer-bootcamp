/* main.c — the CLIENT. It includes only stack.h and knows nothing else.
 *
 *   make && ./stackdemo
 */
#include "stack.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    puts("=== OPAQUE TYPES: ENCAPSULATION IN C ===\n");

    puts("  This file includes only stack.h, which says:");
    puts("      typedef struct Stack Stack;");
    puts("  ...and nothing more. `struct Stack` is an INCOMPLETE type here.");
    puts("");
    puts("  Things this file physically CANNOT do:");
    puts("      Stack s;                 /* error: storage size unknown  */");
    puts("      s->data[0] = 5;          /* error: no member named data  */");
    puts("      sizeof(Stack)            /* error: incomplete type       */");
    puts("  It can only hold a Stack* and call the published functions.\n");

    /* ---------------- fixed capacity ---------------- */
    puts("=== A FIXED-CAPACITY STACK ===");
    {
        Stack *s = stack_create(4);
        if (s == NULL) { perror("stack_create"); return 1; }

        for (int i = 1; i <= 6; i++) {
            StackError e = stack_push(s, i * 10);
            printf("  push(%2d) -> %-16s size=%zu/%zu\n",
                   i * 10, stack_strerror(e), stack_size(s), stack_capacity(s));
        }

        int top;
        if (stack_peek(s, &top) == STACK_OK)
            printf("  peek     -> %d (not removed, size still %zu)\n", top, stack_size(s));

        puts("  popping everything:");
        int value;
        while (stack_pop(s, &value) == STACK_OK)
            printf("    pop -> %d\n", value);
        printf("  pop on empty -> %s\n", stack_strerror(stack_pop(s, &value)));

        stack_destroy(s);
    }

    /* ---------------- growable ---------------- */
    puts("\n=== A GROWABLE STACK (capacity 0) ===");
    {
        Stack *s = stack_create(0);
        if (s == NULL) return 1;

        size_t last_cap = 0;
        for (int i = 0; i < 20; i++) {
            stack_push(s, i);
            if (stack_capacity(s) != last_cap) {
                printf("  after %2zu pushes, capacity grew to %zu\n",
                       stack_size(s), stack_capacity(s));
                last_cap = stack_capacity(s);
            }
        }
        printf("  final: size=%zu capacity=%zu\n", stack_size(s), stack_capacity(s));
        puts("  The doubling policy lives entirely inside stack.c. This file has");
        puts("  no idea it exists, and would not need changing if it changed.");
        stack_destroy(s);
    }

    /* ---------------- a real use ---------------- */
    puts("\n=== A REAL USE: BRACKET MATCHING ===");
    {
        const char *tests[] = {
            "((a + b) * [c - d])", "({[]})", "(()", "([)]", "", "no brackets here"
        };

        for (size_t t = 0; t < sizeof tests / sizeof tests[0]; t++) {
            Stack *s = stack_create(0);
            bool balanced = true;

            for (const char *p = tests[t]; *p && balanced; p++) {
                if (*p == '(' || *p == '[' || *p == '{') {
                    stack_push(s, *p);
                } else if (*p == ')' || *p == ']' || *p == '}') {
                    int open;
                    if (stack_pop(s, &open) != STACK_OK) { balanced = false; break; }
                    char want = (*p == ')') ? '(' : (*p == ']') ? '[' : '{';
                    if (open != want) balanced = false;
                }
            }
            if (!stack_is_empty(s)) balanced = false;    /* unclosed openers */

            printf("  %-24s %s\n", tests[t], balanced ? "balanced" : "NOT balanced");
            stack_destroy(s);
        }
    }

    puts("\n=== WHAT OPAQUE TYPES BUY YOU ===");
    puts("  + Callers cannot touch, corrupt, or depend on the representation.");
    puts("  + You can change fields, add fields, or swap the whole data structure");
    puts("    for another one, and callers do not even recompile.");
    puts("  + The header stays tiny — no implementation #includes leak into it,");
    puts("    which keeps build times down across a large project.");
    puts("  + Invariants are enforceable, because every mutation goes through");
    puts("    a function you wrote.");
    puts("");
    puts("  - The object MUST be heap-allocated: callers do not know its size,");
    puts("    so they cannot put one on the stack or embed it in another struct.");
    puts("  - Every access is a function call (though -O2 + LTO can inline them).");
    puts("");
    puts("  THE HYBRID: publish the struct but document the fields as private,");
    puts("  or expose a size constant so callers can allocate storage while the");
    puts("  layout stays hidden. That is how pthread_mutex_t works.");
    puts("");
    puts("  You have already used opaque types today: FILE * is one. You have");
    puts("  never looked inside a FILE, and that is why the same code works on");
    puts("  glibc, musl, and Windows.");

    return 0;
}
