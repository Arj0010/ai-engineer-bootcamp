/* stack.c — the IMPLEMENTATION. Everything in here is invisible to callers.
 *
 * Because the struct definition lives in this file and not in stack.h, we can
 * change the representation freely — swap the array for a linked list, add
 * statistics, change the growth policy — and no caller needs recompiling,
 * let alone editing.
 */
#include "stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>     /* SIZE_MAX */

/* THE DEFINITION. Callers see only `typedef struct Stack Stack;` and so can
 * never reach these fields. This is the whole mechanism. */
struct Stack {
    int    *data;
    size_t  size;         /* how many are in use   */
    size_t  capacity;     /* how many fit          */
    bool    growable;     /* created with capacity 0 -> grow on demand */

    /* Nothing stops us adding fields. No caller can notice. */
    size_t  max_size_seen;
    size_t  push_count;
};

/* Internal helpers get `static` — invisible even to the linker. */
static bool stack_grow(Stack *s)
{
    size_t cap = (s->capacity == 0) ? 8 : s->capacity * 2;
    if (cap > SIZE_MAX / sizeof *s->data) return false;

    int *tmp = realloc(s->data, cap * sizeof *tmp);
    if (tmp == NULL) return false;              /* s->data still valid */
    s->data = tmp;
    s->capacity = cap;
    return true;
}

const char *stack_strerror(StackError e)
{
    switch (e) {
    case STACK_OK:     return "ok";
    case STACK_EMPTY:  return "stack is empty";
    case STACK_FULL:   return "stack is full";
    case STACK_NOMEM:  return "out of memory";
    case STACK_BADARG: return "invalid argument";
    }
    return "unknown error";
}

Stack *stack_create(size_t capacity)
{
    Stack *s = calloc(1, sizeof *s);        /* calloc: pointers start NULL, so
                                             * the failure path below is safe */
    if (s == NULL) return NULL;

    s->growable = (capacity == 0);
    if (capacity > 0) {
        s->data = malloc(capacity * sizeof *s->data);
        if (s->data == NULL) { free(s); return NULL; }
        s->capacity = capacity;
    }
    return s;
}

void stack_destroy(Stack *s)
{
    if (s == NULL) return;                  /* mirror free()'s contract */
    free(s->data);
    free(s);
}

StackError stack_push(Stack *s, int value)
{
    if (s == NULL) return STACK_BADARG;

    if (s->size == s->capacity) {
        if (!s->growable)      return STACK_FULL;
        if (!stack_grow(s))    return STACK_NOMEM;
    }
    s->data[s->size++] = value;

    s->push_count++;
    if (s->size > s->max_size_seen) s->max_size_seen = s->size;
    return STACK_OK;
}

StackError stack_pop(Stack *s, int *out_value)
{
    if (s == NULL)   return STACK_BADARG;
    if (s->size == 0) return STACK_EMPTY;
    s->size--;
    if (out_value != NULL) *out_value = s->data[s->size];   /* NULL = discard */
    return STACK_OK;
}

StackError stack_peek(const Stack *s, int *out_value)
{
    if (s == NULL || out_value == NULL) return STACK_BADARG;
    if (s->size == 0) return STACK_EMPTY;
    *out_value = s->data[s->size - 1];
    return STACK_OK;
}

size_t stack_size    (const Stack *s) { return s ? s->size     : 0; }
size_t stack_capacity(const Stack *s) { return s ? s->capacity : 0; }
bool   stack_is_empty(const Stack *s) { return s == NULL || s->size == 0; }

void stack_clear(Stack *s) { if (s != NULL) s->size = 0; }
