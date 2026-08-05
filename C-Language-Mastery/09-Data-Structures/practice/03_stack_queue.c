/* 03_stack_queue.c — stack, queue, ring buffer, deque.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 03_stack_queue.c -o t && ./t
 *
 * These are not really data structures — they are ACCESS DISCIPLINES imposed
 * on an array or a list. The restriction is the point: it makes the code that
 * uses them easier to reason about.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ================================================================= *
 * STACK (LIFO) — an array plus a top index. Everything is O(1).
 * ================================================================= */
typedef struct { int *data; size_t top, cap; } Stack;

static bool stack_init(Stack *s, size_t cap)
{
    s->data = malloc(cap * sizeof *s->data);
    if (s->data == NULL) return false;
    s->top = 0; s->cap = cap;
    return true;
}
static void stack_free(Stack *s) { free(s->data); s->data = NULL; s->top = s->cap = 0; }
static bool stack_push(Stack *s, int v)
{
    if (s->top == s->cap) {
        size_t cap = s->cap * 2;
        int *tmp = realloc(s->data, cap * sizeof *tmp);
        if (tmp == NULL) return false;
        s->data = tmp; s->cap = cap;
    }
    s->data[s->top++] = v;
    return true;
}
static bool stack_pop (Stack *s, int *out) { if (!s->top) return false; if (out) *out = s->data[--s->top]; return true; }
static bool stack_peek(const Stack *s, int *out) { if (!s->top) return false; if (out) *out = s->data[s->top - 1]; return true; }
static bool stack_empty(const Stack *s) { return s->top == 0; }

/* ================================================================= *
 * RING (CIRCULAR) BUFFER — a FIXED-SIZE queue with NO allocation after
 * setup. Head and tail wrap around with modulo.
 *
 * This is the workhorse of embedded and real-time systems: bounded memory,
 * O(1) operations, and it is the natural structure for a producer/consumer
 * handoff (audio buffers, UART receive queues, network packet rings).
 * ================================================================= */
typedef struct {
    int    *data;
    size_t  cap;
    size_t  head;    /* index to READ from  */
    size_t  tail;    /* index to WRITE to   */
    size_t  count;   /* how many are stored */
} Ring;

/* Why store `count` rather than deriving it? With only head and tail, the
 * FULL and EMPTY states are indistinguishable — both have head == tail.
 * The alternatives are: keep a count (done here), waste one slot, or use
 * free-running indices and mask them. */
static bool ring_init(Ring *r, size_t cap)
{
    r->data = malloc(cap * sizeof *r->data);
    if (r->data == NULL) return false;
    r->cap = cap; r->head = r->tail = r->count = 0;
    return true;
}
static void ring_free(Ring *r) { free(r->data); r->data = NULL; r->cap = r->count = 0; }
static bool ring_full (const Ring *r) { return r->count == r->cap; }
static bool ring_empty(const Ring *r) { return r->count == 0; }

static bool ring_push(Ring *r, int v)
{
    if (ring_full(r)) return false;            /* refuse; see overwrite below */
    r->data[r->tail] = v;
    r->tail = (r->tail + 1) % r->cap;          /* WRAP */
    r->count++;
    return true;
}
/* The other policy: overwrite the oldest. Right for logs and audio, where
 * fresh data matters more than complete data. */
static void ring_push_overwrite(Ring *r, int v)
{
    if (ring_full(r)) r->head = (r->head + 1) % r->cap;   /* drop the oldest */
    else              r->count++;
    r->data[r->tail] = v;
    r->tail = (r->tail + 1) % r->cap;
}
static bool ring_pop(Ring *r, int *out)
{
    if (ring_empty(r)) return false;
    if (out) *out = r->data[r->head];
    r->head = (r->head + 1) % r->cap;
    r->count--;
    return true;
}
static void ring_print(const Ring *r, const char *label)
{
    printf("  %-22s [", label);
    for (size_t i = 0; i < r->count; i++)
        printf("%d%s", r->data[(r->head + i) % r->cap], i + 1 < r->count ? " " : "");
    printf("]  head=%zu tail=%zu count=%zu/%zu\n", r->head, r->tail, r->count, r->cap);
}

/* ================================================================= *
 * QUEUE (FIFO) from a linked list — grows without bound.
 * ================================================================= */
typedef struct QNode { int v; struct QNode *next; } QNode;
typedef struct { QNode *head, *tail; size_t len; } Queue;

static void queue_init(Queue *q) { q->head = q->tail = NULL; q->len = 0; }
static void queue_free(Queue *q)
{ QNode *n = q->head; while (n) { QNode *x = n->next; free(n); n = x; } queue_init(q); }
static bool queue_enqueue(Queue *q, int v)
{
    QNode *n = malloc(sizeof *n);
    if (n == NULL) return false;
    n->v = v; n->next = NULL;
    if (q->tail) q->tail->next = n; else q->head = n;   /* the tail pointer is */
    q->tail = n;                                        /* what makes this O(1) */
    q->len++;
    return true;
}
static bool queue_dequeue(Queue *q, int *out)
{
    if (q->head == NULL) return false;
    QNode *n = q->head;
    if (out) *out = n->v;
    q->head = n->next;
    if (q->head == NULL) q->tail = NULL;
    free(n);
    q->len--;
    return true;
}

/* ================================================================= *
 * A QUEUE FROM TWO STACKS — the classic interview problem, and a genuine
 * lesson in AMORTISED analysis.
 *
 * `in` receives pushes. When `out` is empty, everything is poured from `in`
 * into `out`, which REVERSES the order and turns LIFO into FIFO.
 *
 * A single dequeue can cost O(n), but each element is moved between the two
 * stacks exactly ONCE in its lifetime, so n operations cost O(n) total —
 * amortised O(1).
 * ================================================================= */
typedef struct { Stack in, out; } TwoStackQueue;

static bool tsq_init(TwoStackQueue *q) { return stack_init(&q->in, 8) && stack_init(&q->out, 8); }
static void tsq_free(TwoStackQueue *q) { stack_free(&q->in); stack_free(&q->out); }
static bool tsq_enqueue(TwoStackQueue *q, int v) { return stack_push(&q->in, v); }
static bool tsq_dequeue(TwoStackQueue *q, int *out)
{
    if (stack_empty(&q->out)) {                 /* only refill when empty */
        int v = 0;
        while (stack_pop(&q->in, &v))           /* pour: this REVERSES the order */
            if (!stack_push(&q->out, v)) return false;
    }
    return stack_pop(&q->out, out);
}

/* ================================================================= *
 * DEQUE — a double-ended queue over a ring buffer. Push and pop at BOTH
 * ends, all O(1).
 * ================================================================= */
typedef Ring Deque;
static bool deque_push_back(Deque *d, int v) { return ring_push(d, v); }
static bool deque_push_front(Deque *d, int v)
{
    if (ring_full(d)) return false;
    d->head = (d->head + d->cap - 1) % d->cap;   /* step BACK, with wrap */
    d->data[d->head] = v;
    d->count++;
    return true;
}
static bool deque_pop_front(Deque *d, int *out) { return ring_pop(d, out); }
static bool deque_pop_back(Deque *d, int *out)
{
    if (ring_empty(d)) return false;
    d->tail = (d->tail + d->cap - 1) % d->cap;
    if (out) *out = d->data[d->tail];
    d->count--;
    return true;
}

int main(void)
{
    puts("=== STACK (LIFO) ===");
    {
        Stack s; stack_init(&s, 4);
        printf("  pushing 1..6: ");
        for (int i = 1; i <= 6; i++) { stack_push(&s, i); printf("%d ", i); }
        printf("(capacity grew to %zu)\n", s.cap);

        int v = 0;
        stack_peek(&s, &v);
        printf("  peek -> %d (not removed)\n", v);
        printf("  popping: ");
        while (stack_pop(&s, &v)) printf("%d ", v);
        puts(" <- reverse order, by definition");
        printf("  pop on empty -> %s\n", stack_pop(&s, &v) ? "value" : "false");
        stack_free(&s);

        puts("\n  Where stacks appear whether you write one or not:");
        puts("    - the CALL STACK: every function call pushes a frame");
        puts("    - expression evaluation and bracket matching");
        puts("    - undo/redo history");
        puts("    - DFS, and any recursive algorithm made iterative");
        puts("    - backtracking (module 10)");
    }

    puts("\n=== QUEUE (FIFO) ===");
    {
        Queue q; queue_init(&q);
        printf("  enqueue 1..5: ");
        for (int i = 1; i <= 5; i++) { queue_enqueue(&q, i); printf("%d ", i); }
        printf("(len %zu)\n", q.len);

        int v = 0;
        printf("  dequeue: ");
        while (queue_dequeue(&q, &v)) printf("%d ", v);
        puts(" <- SAME order, by definition");
        queue_free(&q);

        puts("  The tail pointer is what makes enqueue O(1). Without one you");
        puts("  would walk the whole list to find the end, making it O(n).");
        puts("  Queues appear in: BFS, task/work queues, print spoolers,");
        puts("  producer-consumer handoffs, and OS scheduler run queues.");
    }

    puts("\n=== RING BUFFER (fixed size, no allocation after setup) ===");
    {
        Ring r; ring_init(&r, 5);
        for (int i = 1; i <= 5; i++) ring_push(&r, i);
        ring_print(&r, "filled with 1..5");
        printf("  full? %s   push(6) -> %s\n",
               ring_full(&r) ? "yes" : "no", ring_push(&r, 6) ? "ok" : "REFUSED");

        int v = 0;
        ring_pop(&r, &v); ring_pop(&r, &v);
        ring_print(&r, "after 2 pops");
        printf("      ^ head moved to %zu; slots 0 and 1 are now free for reuse\n", r.head);

        ring_push(&r, 6); ring_push(&r, 7);
        ring_print(&r, "push 6,7 (WRAPPED)");
        printf("      ^ tail wrapped around to %zu. The data is not contiguous in\n", r.tail);
        puts("        the array, but it is contiguous LOGICALLY.");

        puts("\n  overwrite policy (for logs and audio, where fresh beats complete):");
        Ring o; ring_init(&o, 4);
        for (int i = 1; i <= 7; i++) {
            ring_push_overwrite(&o, i);
            char label[32]; snprintf(label, sizeof label, "after overwrite-push %d", i);
            ring_print(&o, label);
        }
        puts("      ^ the oldest entries were dropped to make room");
        ring_free(&r); ring_free(&o);

        puts("\n  WHY count IS STORED SEPARATELY:");
        puts("    With only head and tail, FULL and EMPTY both look like");
        puts("    head == tail. Three ways out: store a count (done here),");
        puts("    waste one slot so full means tail+1 == head, or use");
        puts("    free-running indices and mask them (best for lock-free rings).");
        puts("");
        puts("  A ring buffer is THE structure for embedded and real-time work:");
        puts("  bounded memory, no allocation, O(1) everything, and with a");
        puts("  single producer and single consumer it needs NO LOCK at all.");
    }

    puts("\n=== A QUEUE FROM TWO STACKS ===");
    {
        TwoStackQueue q; tsq_init(&q);
        printf("  enqueue 1..5 (all land on the `in` stack)\n");
        for (int i = 1; i <= 5; i++) tsq_enqueue(&q, i);
        printf("    in=%zu out=%zu\n", q.in.top, q.out.top);

        int v = 0;
        tsq_dequeue(&q, &v);
        printf("  first dequeue -> %d   (poured all 5 across: an O(n) step)\n", v);
        printf("    in=%zu out=%zu\n", q.in.top, q.out.top);

        printf("  further dequeues: ");
        while (tsq_dequeue(&q, &v)) printf("%d ", v);
        puts(" <- each O(1), no pouring needed");

        tsq_enqueue(&q, 10); tsq_enqueue(&q, 20);
        tsq_dequeue(&q, &v);
        printf("  enqueue 10,20 then dequeue -> %d (correct FIFO order)\n", v);
        tsq_free(&q);

        puts("\n  AMORTISED ANALYSIS: one dequeue can cost O(n), yet n operations");
        puts("  cost O(n) TOTAL. Each element is pushed to `in` once, popped");
        puts("  once, pushed to `out` once, popped once — four operations in its");
        puts("  entire lifetime, no matter how many dequeues happen.");
        puts("  Worst case per operation and amortised cost are different");
        puts("  questions. For a real-time system the worst case is what matters;");
        puts("  for throughput, the amortised cost is.");
    }

    puts("\n=== DEQUE (both ends, all O(1)) ===");
    {
        Deque d; ring_init(&d, 6);
        deque_push_back(&d, 3); deque_push_back(&d, 4);
        deque_push_front(&d, 2); deque_push_front(&d, 1);
        ring_print(&d, "push 3,4 back; 2,1 front");

        int v = 0;
        deque_pop_front(&d, &v); printf("  pop_front -> %d\n", v);
        deque_pop_back(&d, &v);  printf("  pop_back  -> %d\n", v);
        ring_print(&d, "after both pops");
        ring_free(&d);

        puts("  push_front steps the head BACKWARD with wraparound:");
        puts("      head = (head + cap - 1) % cap");
        puts("  The `+ cap` avoids a negative modulo — remember that C's % takes");
        puts("  the sign of the dividend, so (0 - 1) % 6 is -1, not 5.");
        puts("  Deques power: sliding-window algorithms, undo/redo with a cap,");
        puts("  and work-stealing schedulers.");
    }

    puts("\n=== CHOOSING ===");
    puts("  STACK        LIFO. Recursion made explicit, backtracking, parsing.");
    puts("  QUEUE        FIFO. BFS, task queues, fair scheduling.");
    puts("  RING BUFFER  FIFO with a HARD SIZE CAP and no allocation. Embedded,");
    puts("               audio, networking, logging.");
    puts("  DEQUE        both ends. Sliding windows, work stealing.");
    puts("");
    puts("  Back them with an ARRAY unless you need unbounded growth with no");
    puts("  large contiguous allocation. The array version is faster in every");
    puts("  case that fits in memory.");

    return 0;
}
