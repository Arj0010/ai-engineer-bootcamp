/* 04_threads.c — pthreads: races, mutexes, condition variables, a thread pool.
 *
 *   gcc -std=c17 -Wall -Wextra -Wpedantic -O2 04_threads.c -o t -lpthread && ./t
 *
 * Find the deliberate data race with ThreadSanitizer:
 *   gcc -std=c17 -g -fsanitize=thread 04_threads.c -o t_tsan -lpthread && ./t_tsan
 *
 * Threads share EVERYTHING except their stack and registers. That makes
 * communication free and correctness hard.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdalign.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define N_THREADS   8
#define INCREMENTS  200000

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* ================================================================= *
 * 1. THE DATA RACE
 *
 * `counter++` is THREE machine operations: load, add, store. Two threads
 * can both load the same value, both add one, and both store — so two
 * increments produce one. This is UNDEFINED BEHAVIOUR, not merely a wrong
 * count: the compiler is entitled to assume it does not happen.
 * ================================================================= */
/* NOTE THE `volatile`. Without it, at -O2 the compiler hoists the whole
 * loop into a register and emits `counter += 200000` — one store, no race,
 * and the demo silently proves nothing. (That the optimiser is ALLOWED to do
 * this to racy code is itself the point: the race is undefined behaviour, so
 * anything may happen, including it appearing to work.)
 *
 * `volatile` forces a real LOAD-ADD-STORE every iteration, which makes the
 * race visible. It does NOT fix anything: volatile gives no atomicity and no
 * ordering. This is the clearest possible demonstration that VOLATILE IS NOT
 * A THREADING PRIMITIVE. */
static volatile long racy_counter = 0;

static void *racy_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < INCREMENTS; i++) racy_counter++;   /* THE RACE */
    return NULL;
}

/* ================================================================= *
 * 2. MUTEX — correct, and the cost of correctness
 * ================================================================= */
static long           mutex_counter = 0;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

static void *mutex_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < INCREMENTS; i++) {
        pthread_mutex_lock(&counter_lock);
        mutex_counter++;                         /* the CRITICAL SECTION */
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

/* Batching: do the work locally, take the lock ONCE. Same answer, and the
 * lock is contended 200000x less. Shrinking critical sections is the single
 * most effective concurrency optimisation. */
static long           batched_counter = 0;
static pthread_mutex_t batch_lock = PTHREAD_MUTEX_INITIALIZER;

static void *batched_worker(void *arg)
{
    (void)arg;
    long local = 0;
    for (int i = 0; i < INCREMENTS; i++) local++;    /* no sharing at all */
    pthread_mutex_lock(&batch_lock);
    batched_counter += local;
    pthread_mutex_unlock(&batch_lock);
    return NULL;
}

/* ================================================================= *
 * 3. ATOMICS — lock-free, and the right tool for a simple counter
 * ================================================================= */
static atomic_long atomic_counter = 0;

static void *atomic_worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < INCREMENTS; i++)
        atomic_fetch_add(&atomic_counter, 1);    /* ONE indivisible instruction */
    return NULL;
}

/* ================================================================= *
 * 4. FALSE SHARING — the bug you cannot see in the source
 *
 * These counters are separate variables with no synchronisation between
 * them, so there is no race. But if they share a 64-byte CACHE LINE, every
 * write by one core invalidates the line in every other core's cache. The
 * hardware serialises them anyway, and throughput collapses.
 * ================================================================= */
/* volatile again, for the same reason: otherwise the compiler keeps the
 * counter in a register and never touches the cache line at all. */
typedef struct { volatile long value; }             PackedCounter;   /* 8 bytes */
typedef struct { alignas(64) volatile long value; }  PaddedCounter;   /* 64 bytes */

static PackedCounter packed[N_THREADS];
static PaddedCounter padded[N_THREADS];

static void *packed_worker(void *arg)
{
    long id = (long)arg;
    for (int i = 0; i < INCREMENTS * 50; i++) packed[id].value++;
    return NULL;
}
static void *padded_worker(void *arg)
{
    long id = (long)arg;
    for (int i = 0; i < INCREMENTS * 50; i++) padded[id].value++;
    return NULL;
}

/* ================================================================= *
 * 5. CONDITION VARIABLE — waiting without spinning
 *
 * A producer/consumer queue. The consumer must WAIT for work rather than
 * poll, and pthread_cond_wait atomically releases the mutex and sleeps.
 * ================================================================= */
#define QUEUE_CAP 8

typedef struct {
    int             items[QUEUE_CAP];
    int             head, tail, count;
    bool            closed;
    pthread_mutex_t lock;
    pthread_cond_t  not_empty;      /* consumers wait here */
    pthread_cond_t  not_full;       /* producers wait here */
} BoundedQueue;

static void bq_init(BoundedQueue *q)
{
    q->head = q->tail = q->count = 0;
    q->closed = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}
static void bq_destroy(BoundedQueue *q)
{
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
}

static void bq_push(BoundedQueue *q, int value)
{
    pthread_mutex_lock(&q->lock);

    /* WHILE, NEVER IF. Two reasons:
     *   1. SPURIOUS WAKEUPS are permitted by POSIX — cond_wait may return
     *      without any signal at all.
     *   2. Between the signal and this thread actually acquiring the mutex,
     *      ANOTHER thread may have taken the slot. The condition must be
     *      re-checked, not assumed. */
    while (q->count == QUEUE_CAP && !q->closed)
        pthread_cond_wait(&q->not_full, &q->lock);

    if (!q->closed) {
        q->items[q->tail] = value;
        q->tail = (q->tail + 1) % QUEUE_CAP;
        q->count++;
        pthread_cond_signal(&q->not_empty);      /* wake ONE consumer */
    }
    pthread_mutex_unlock(&q->lock);
}

static bool bq_pop(BoundedQueue *q, int *out)
{
    pthread_mutex_lock(&q->lock);
    while (q->count == 0 && !q->closed)
        pthread_cond_wait(&q->not_empty, &q->lock);

    if (q->count == 0) {                          /* closed and drained */
        pthread_mutex_unlock(&q->lock);
        return false;
    }
    *out = q->items[q->head];
    q->head = (q->head + 1) % QUEUE_CAP;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return true;
}

static void bq_close(BoundedQueue *q)
{
    pthread_mutex_lock(&q->lock);
    q->closed = true;
    pthread_cond_broadcast(&q->not_empty);        /* wake EVERY waiter */
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

/* ================================================================= *
 * 6. A THREAD POOL over the queue
 * ================================================================= */
typedef struct {
    BoundedQueue *queue;
    atomic_long  *sum;
    int           id;
    long          items_handled;
} PoolWorker;

static void *pool_worker(void *arg)
{
    PoolWorker *w = arg;
    int job;
    while (bq_pop(w->queue, &job)) {
        /* "process" the job: sum of divisors, just to burn some cycles */
        long work = 0;
        for (int d = 1; d <= job; d++) if (job % d == 0) work += d;
        atomic_fetch_add(w->sum, work);
        w->items_handled++;
    }
    return NULL;
}

/* Parallel sum: split the array, one thread per chunk, combine at the end. */
typedef struct { const double *data; size_t from, to; double partial; } SumJob;

static void *sum_chunk(void *arg)
{
    SumJob *j = arg;
    double s = 0.0;
    for (size_t i = j->from; i < j->to; i++) s += j->data[i];
    j->partial = s;                    /* each thread writes its OWN slot: no race */
    return NULL;
}

int main(void)
{
    pthread_t threads[N_THREADS];

    puts("=== 1. THE DATA RACE ===");
    {
        double t0 = now_seconds();
        for (long i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, racy_worker, (void *)i);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double elapsed = now_seconds() - t0;

        long expected = (long)N_THREADS * INCREMENTS;
        printf("  %d threads x %d increments\n", N_THREADS, INCREMENTS);
        printf("    expected : %ld\n", expected);
        printf("    got      : %ld   (%.1f%% lost)\n", racy_counter,
               100.0 * (double)(expected - racy_counter) / (double)expected);
        printf("    time     : %.4f s\n", elapsed);
        puts("");
        puts("  `counter++` is THREE operations: LOAD, ADD, STORE. Two threads");
        puts("  can both load 5, both compute 6, and both store 6 — two");
        puts("  increments produce one.");
        puts("");
        puts("  This is UNDEFINED BEHAVIOUR, not 'sometimes a wrong count'. The");
        puts("  compiler may assume no data race exists and optimise on that");
        puts("  basis — which is why the loss varies wildly with -O level.");
        puts("");
        puts("  NOTE: racy_counter is declared `volatile` here ONLY so the race");
        puts("  is visible. Without it, -O2 turns the loop into a single");
        puts("  `counter += 200000` and the demo appears to work perfectly.");
        puts("  volatile forces the load-add-store — and fixes NOTHING, because");
        puts("  it provides no atomicity and no ordering. If you take one thing");
        puts("  from this file: VOLATILE IS NOT A THREADING PRIMITIVE.");
        puts("  Run under -fsanitize=thread to see the race reported precisely.");
    }

    puts("\n=== 2. MUTEX ===");
    {
        double t0 = now_seconds();
        for (int i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, mutex_worker, NULL);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double t_mutex = now_seconds() - t0;

        t0 = now_seconds();
        for (int i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, atomic_worker, NULL);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double t_atomic = now_seconds() - t0;

        t0 = now_seconds();
        for (int i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, batched_worker, NULL);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double t_batched = now_seconds() - t0;

        long expected = (long)N_THREADS * INCREMENTS;
        printf("  expected %ld in every case:\n", expected);
        printf("    mutex   : %8ld  %.4f s\n", mutex_counter, t_mutex);
        printf("    atomic  : %8ld  %.4f s  (%.1fx faster than the mutex)\n",
               atomic_load(&atomic_counter), t_atomic, t_mutex / t_atomic);
        printf("    batched : %8ld  %.4f s  (%.0fx faster — the lock is taken\n",
               batched_counter, t_batched, t_mutex / t_batched);
        puts("                                     ONCE per thread, not 200000x)");
        puts("");
        puts("  All three are CORRECT. They differ only in how much they");
        puts("  contend.");
        puts("");
        puts("  - a MUTEX is general: it protects any critical section, of any");
        puts("    size. It costs a syscall when contended (futex on Linux).");
        puts("  - an ATOMIC is one indivisible instruction (LOCK XADD on x86).");
        puts("    Far cheaper, but it only works for single-word operations.");
        puts("  - BATCHING avoids sharing altogether. Each thread works on");
        puts("    private data and combines once at the end.");
        puts("");
        puts("  THE LESSON: the fastest synchronisation is the one you do not");
        puts("  do. Shrink critical sections, or eliminate the sharing.");
    }

    puts("\n=== 3. FALSE SHARING ===");
    {
        memset(packed, 0, sizeof packed);
        memset(padded, 0, sizeof padded);

        double t0 = now_seconds();
        for (long i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, packed_worker, (void *)i);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double t_packed = now_seconds() - t0;

        t0 = now_seconds();
        for (long i = 0; i < N_THREADS; i++) pthread_create(&threads[i], NULL, padded_worker, (void *)i);
        for (int i = 0; i < N_THREADS; i++) pthread_join(threads[i], NULL);
        double t_padded = now_seconds() - t0;

        printf("  %d threads, each incrementing ITS OWN counter %d times:\n",
               N_THREADS, INCREMENTS * 50);
        printf("    packed (8 bytes apart)  : %.4f s\n", t_packed);
        printf("    padded (64 bytes apart) : %.4f s   (%.2fx)\n",
               t_padded, t_packed / t_padded);
        printf("    sizeof(PackedCounter) = %zu, sizeof(PaddedCounter) = %zu\n",
               sizeof(PackedCounter), sizeof(PaddedCounter));
        puts("");
        puts("  There is NO DATA RACE here — every thread touches a different");
        puts("  variable. But 8 packed counters fit in ONE 64-byte cache line,");
        puts("  and cache coherence works at LINE granularity. Every write by");
        puts("  one core invalidates that line in all the others, so the cores");
        puts("  ping-pong the line between them.");
        puts("");
        puts("  alignas(64) puts each counter on its own line and the problem");
        puts("  vanishes. This is FALSE SHARING: a correctness-neutral bug that");
        puts("  is completely invisible in the source.");
        puts("");
        puts("  HOW BIG THE EFFECT IS DEPENDS ENTIRELY ON THE HARDWARE. On a");
        puts("  bare-metal multi-socket machine with many real cores it is");
        puts("  routinely 5-10x. Inside a container or VM with few cores that");
        puts("  are time-sliced, threads rarely run at the same instant, so the");
        puts("  cache line is not actually contended and the gap shrinks to a");
        puts("  few percent — which may well be what you just measured above.");
        puts("");
        puts("  If your ratio is close to 1.0, that is a fact about your machine,");
        puts("  not a refutation. Run it on a bigger box and it reappears.");
        puts("  The engineering rule stands regardless: pad per-thread data to");
        puts("  a cache line. It costs a few bytes and removes a whole class of");
        puts("  invisible scaling failure.");
    }

    puts("\n=== 4. CONDITION VARIABLE + THREAD POOL ===");
    {
        BoundedQueue q;
        bq_init(&q);
        atomic_long total = 0;

        PoolWorker workers[4];
        pthread_t  pool[4];
        for (int i = 0; i < 4; i++) {
            workers[i] = (PoolWorker){ .queue = &q, .sum = &total, .id = i, .items_handled = 0 };
            pthread_create(&pool[i], NULL, pool_worker, &workers[i]);
        }

        const int N_JOBS = 2000;
        for (int i = 1; i <= N_JOBS; i++) bq_push(&q, i);
        bq_close(&q);                              /* tell the workers to finish */
        for (int i = 0; i < 4; i++) pthread_join(pool[i], NULL);

        long handled = 0;
        printf("  4 workers drained a %d-slot queue of %d jobs:\n", QUEUE_CAP, N_JOBS);
        for (int i = 0; i < 4; i++) {
            printf("    worker %d handled %ld jobs\n", i, workers[i].items_handled);
            handled += workers[i].items_handled;
        }
        printf("    total handled: %ld (expected %d) %s\n",
               handled, N_JOBS, handled == N_JOBS ? "OK" : "*** LOST JOBS ***");
        printf("    combined result: %ld\n", atomic_load(&total));
        bq_destroy(&q);

        puts("");
        puts("  The work is NOT split evenly, and that is the point: a thread");
        puts("  pool self-balances. A fast worker simply takes more jobs.");
        puts("");
        puts("  WHY pthread_cond_wait MUST BE IN A while LOOP:");
        puts("      while (queue_is_empty) pthread_cond_wait(&cv, &lock);");
        puts("      ^^^^^ not `if`");
        puts("    1. SPURIOUS WAKEUPS are explicitly permitted by POSIX.");
        puts("    2. Between the signal and this thread reacquiring the mutex,");
        puts("       another thread may have consumed the item.");
        puts("  Using `if` gives you a consumer that proceeds with an empty");
        puts("  queue — a bug that appears once a week under load and never");
        puts("  reproduces in testing.");
        puts("");
        puts("  cond_wait ATOMICALLY releases the mutex and sleeps, then");
        puts("  reacquires it before returning. That atomicity is what closes");
        puts("  the window where a signal could be missed.");
        puts("");
        puts("  signal() wakes ONE waiter; broadcast() wakes ALL. Use broadcast");
        puts("  when the state change may satisfy several waiters — as bq_close");
        puts("  does, because every worker must see the shutdown.");
    }

    puts("\n=== 5. PARALLEL SUM: THE EMBARRASSINGLY PARALLEL CASE ===");
    {
        const size_t N = 20000000;
        double *data = malloc(N * sizeof *data);
        for (size_t i = 0; i < N; i++) data[i] = 1.0;

        double t0 = now_seconds();
        double serial = 0.0;
        for (size_t i = 0; i < N; i++) serial += data[i];
        double t_serial = now_seconds() - t0;

        SumJob jobs[N_THREADS];
        t0 = now_seconds();
        for (int i = 0; i < N_THREADS; i++) {
            jobs[i] = (SumJob){ data, i * N / N_THREADS, (size_t)(i + 1) * N / N_THREADS, 0.0 };
            pthread_create(&threads[i], NULL, sum_chunk, &jobs[i]);
        }
        double parallel = 0.0;
        for (int i = 0; i < N_THREADS; i++) {
            pthread_join(threads[i], NULL);
            parallel += jobs[i].partial;           /* combine AFTER joining */
        }
        double t_parallel = now_seconds() - t0;

        printf("  summing %zu doubles:\n", N);
        printf("    serial   : %.4f s -> %.0f\n", t_serial, serial);
        printf("    %d threads: %.4f s -> %.0f  (%.2fx speedup)\n",
               N_THREADS, t_parallel, parallel, t_serial / t_parallel);
        printf("    results match: %s\n", serial == parallel ? "yes" : "no (floating point ordering)");
        free(data);

        puts("");
        puts("  NO LOCKS AT ALL. Each thread writes only its own `partial`");
        puts("  field, and the combining happens after pthread_join — which is");
        puts("  itself a synchronisation point.");
        puts("");
        puts("  The speedup is well under Nx because this is MEMORY BANDWIDTH");
        puts("  bound, not CPU bound. Eight cores cannot read memory eight times");
        puts("  faster. Amdahl's law plus the memory wall: parallelism helps");
        puts("  compute-heavy work far more than it helps streaming work.");
    }

    puts("\n=== THE RULES ===");
    puts("  1. A DATA RACE IS UNDEFINED BEHAVIOUR. Not 'occasionally wrong'.");
    puts("  2. Protect shared MUTABLE state, or do not share it.");
    puts("  3. Prefer NOT SHARING: give each thread its own data and combine");
    puts("     at the end. It is faster and it cannot be got wrong.");
    puts("  4. Keep critical sections SHORT. Never do I/O holding a lock.");
    puts("  5. cond_wait goes in a WHILE loop. Always.");
    puts("  6. Lock in a consistent ORDER everywhere, or you will deadlock.");
    puts("  7. Atomics for counters and flags; mutexes for anything larger.");
    puts("  8. `volatile` is NOT a threading primitive. It gives no atomicity");
    puts("     and no ordering. Use <stdatomic.h>.");
    puts("  9. Watch for FALSE SHARING on per-thread data — pad to 64 bytes.");
    puts(" 10. Develop under -fsanitize=thread. It finds races that testing");
    puts("     will not, because it checks the HAPPENS-BEFORE relation rather");
    puts("     than waiting for an unlucky interleaving.");

    return 0;
}
