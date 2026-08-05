/* stack.h — a PUBLIC interface with a completely HIDDEN representation.
 *
 * This is C's answer to encapsulation. Callers get a `Stack *` and a set of
 * functions. They cannot see the fields, cannot declare a Stack on their own
 * stack, and cannot depend on its size — so you can change the implementation
 * entirely without recompiling a single caller.
 */
#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdbool.h>

/* An INCOMPLETE type: declared here, DEFINED only in stack.c.
 * The compiler knows `struct Stack` exists and that pointers to it are 8 bytes.
 * It knows nothing else — which is exactly the point. */
typedef struct Stack Stack;

/* Error codes. Returning a status and writing results through pointers is the
 * standard C convention (strtol, fread, and sscanf all work this way). */
typedef enum {
    STACK_OK = 0,
    STACK_EMPTY,
    STACK_FULL,
    STACK_NOMEM,
    STACK_BADARG,
} StackError;

/* Human-readable form of an error code. */
const char *stack_strerror(StackError e);

/* --- lifetime --------------------------------------------------------- *
 * stack_create allocates. The CALLER MUST call stack_destroy exactly once.
 * Returns NULL on allocation failure.
 * `capacity` of 0 means "grow on demand".
 */
Stack *stack_create(size_t capacity);
void   stack_destroy(Stack *s);          /* stack_destroy(NULL) is a no-op */

/* --- operations ------------------------------------------------------- */
StackError stack_push(Stack *s, int value);
StackError stack_pop (Stack *s, int *out_value);   /* out_value may be NULL */
StackError stack_peek(const Stack *s, int *out_value);

/* --- queries ---------------------------------------------------------- */
size_t stack_size    (const Stack *s);
size_t stack_capacity(const Stack *s);
bool   stack_is_empty(const Stack *s);

/* Clears the contents but keeps the allocated capacity. */
void stack_clear(Stack *s);

#endif /* STACK_H */
