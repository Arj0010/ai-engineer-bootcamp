# Module 05 — Memory Management

C hands you the memory and gets out of the way. There is no garbage collector, no
ownership tracking, no bounds checking. Everything in this module is about building the
discipline that replaces those things.

By the end you will have **written your own `malloc`**, which is the point at which the
heap stops being magic.

---

## 1. The memory layout of a running process

```
high addresses
┌─────────────────────────────┐
│   command-line args, env    │
├─────────────────────────────┤
│   STACK                     │  ← automatic variables, return addresses,
│         │                   │    saved registers. Fixed size (8 MB default).
│         ▼ grows down        │    Allocation is one instruction: sub rsp, N
│                             │
│         (unmapped gap)      │
│                             │
│         ▲ grows up          │
│   HEAP                      │  ← malloc/free. Size limited by RAM+swap.
├─────────────────────────────┤    Allocation costs hundreds of cycles.
│   .bss    zero-initialised  │  ← globals/statics with no initialiser.
│           statics           │    Costs zero bytes in the executable file.
├─────────────────────────────┤
│   .data   initialised       │  ← globals/statics with a non-zero initialiser
│           statics           │
├─────────────────────────────┤
│   .rodata read-only         │  ← string literals, const tables
├─────────────────────────────┤
│   .text   machine code      │  ← your compiled functions. Read+execute.
└─────────────────────────────┘
low addresses
```

Run `practice/01_memory_layout.c` to print the real addresses on your machine and see this
picture confirmed.

### Stack vs heap — the decision table

| | Stack | Heap |
|---|---|---|
| Allocation cost | ~1 instruction (move the stack pointer) | ~100s of cycles (search a free list) |
| Deallocation | automatic, at scope exit | you must call `free` |
| Size limit | ~8 MB total (`ulimit -s`) | available RAM |
| Size known at | compile time (mostly) | run time |
| Lifetime | the enclosing block | until you free it |
| Fragmentation | never | yes |
| Failure mode | stack overflow → SIGSEGV | `malloc` returns NULL |

**Default to the stack.** Use the heap when the object is large (>100 KB), when its size
is only known at run time, or when it must outlive the function that created it.

---

## 2. The allocation functions

```c
void *malloc(size_t size);              /* uninitialised — contains garbage */
void *calloc(size_t n, size_t size);    /* zero-filled, and n*size is overflow-checked */
void *realloc(void *p, size_t size);    /* grow or shrink; may MOVE the block */
void  free(void *p);                    /* free(NULL) is a defined no-op */
```

### The idiomatic `malloc` call

```c
int *a = malloc(n * sizeof *a);
if (a == NULL) { /* handle it */ }
```

Three things to notice:

1. **`sizeof *a`, not `sizeof(int)`.** If you later change `a` to `long *`, this line stays
   correct. `sizeof(int)` silently becomes a bug.
2. **No cast.** `void *` converts implicitly in C. The cast is noise, and pre-C99 it could
   hide a missing `#include <stdlib.h>`.
3. **Check for NULL.** Every time. An unchecked `malloc` is a NULL dereference waiting for
   a bad day.

### `calloc` is not just `malloc` + `memset`

`calloc(n, size)` **checks `n * size` for overflow**. This matters:

```c
size_t n = attacker_controlled;
int *a = malloc(n * sizeof *a);   /* n * 4 can WRAP to a small number! */
```
If `n` is `SIZE_MAX/2`, `n * 4` wraps, you allocate 8 bytes, and then write gigabytes into
them. This exact bug has produced many CVEs. `calloc(n, sizeof *a)` returns NULL instead.

`calloc` is also often *faster* for large blocks, because the OS can hand back pages that
are already known-zero without touching them.

### `realloc` has three traps

```c
p = realloc(p, new_size);        /* WRONG — leaks p if realloc fails */
```
On failure `realloc` returns NULL **and leaves the original block allocated**. Assigning
NULL over `p` loses the only pointer to it. Correct:

```c
void *tmp = realloc(p, new_size);
if (tmp == NULL) { /* p is still valid — handle or free it */ }
p = tmp;
```

Trap two: **realloc may move the block.** Every other pointer into the old block becomes
dangling. If you keep interior pointers, store offsets instead.

Trap three: `realloc(p, 0)` is implementation-defined in C17 (and undefined in C23).
Do not use it to free; call `free`.

### Growth strategy

Never grow an array by one:

```c
for (...) arr = realloc(arr, ++n * sizeof *arr);   /* O(n²) — copies every time */
```

**Double the capacity when full.** That makes `n` appends cost O(n) total — amortised O(1)
each. This is what module 09's dynamic array does, and what every `std::vector` does.

---

## 3. Ownership — the discipline that replaces a GC

C has no way to express "who frees this". You have to. The rule that actually works:

> **Every allocation has exactly one owner. The owner frees it. Document the owner in the
> header, next to the function that returns the pointer.**

```c
/* Returns a newly allocated string. CALLER MUST free() it. */
char *format_name(const char *first, const char *last);

/* Returns a pointer INTO the table. Do NOT free. Valid until table_clear(). */
const Entry *table_lookup(const Table *t, const char *key);
```

Naming conventions carry a lot of this weight:

- `X_create` / `X_destroy` — paired, and always both.
- `X_init(X *)` / `X_deinit(X *)` — for caller-provided storage.
- A function taking `const T *` never frees or stores it.

**The cleanup pattern** (module 02's `goto`) is how you guarantee it on error paths.

---

## 4. Custom allocators

`malloc` is general-purpose, which means it is optimal for nothing. Three patterns cover
most cases where it is the bottleneck:

### Arena (bump) allocator

Allocate a big block once; hand out pieces by moving a cursor forward. Free **everything
at once** by resetting the cursor.

- Allocation is a pointer increment — a handful of instructions.
- There is no individual `free`. That is the trade, and it is often exactly right:
  per-request memory in a server, per-frame memory in a game, per-parse memory in a compiler.
- No fragmentation, no metadata per allocation, perfect locality.

### Pool (free-list) allocator

For many objects of **one fixed size**. Carve a block into equal slots and thread a
singly-linked free list through the unused ones (the "next" pointer lives *inside* the free
slot, so it costs nothing extra).

- `alloc` and `free` are both O(1) and are a few instructions.
- Zero fragmentation, because every slot is interchangeable.
- This is how kernels allocate fixed-size objects (Linux calls it the slab allocator).

### Stack allocator

Like an arena, but supports LIFO frees via a saved marker. Good for recursive algorithms
with scratch space.

---

## 5. Writing your own `malloc`

`practice/05_my_malloc.c` implements a real allocator on top of `mmap`:

- A **block header** before each allocation carrying its size and a free flag.
- An **implicit free list** — walk the headers to find a fit.
- **First-fit** search, with **splitting** when the found block is much too big.
- **Coalescing** of adjacent free blocks on release, to fight fragmentation.
- **Alignment** to `max_align_t` so any type can be stored.

That is roughly `malloc` circa 1979, and it is enough to run real programs. Modern
allocators (ptmalloc, jemalloc, tcmalloc) add size-class bins, per-thread caches, and
different strategies for large blocks — but the core idea is exactly this.

Reading the code answers questions people carry for years:
*Where does `free` learn the size?* (From the header, at `p - sizeof(Header)`.)
*Why does freeing not return memory to the OS?* (It usually just marks the block free.)
*Why is `malloc` slow?* (It searches.)

---

## 6. Debugging memory

| Tool | Catches | Cost |
|---|---|---|
| `-fsanitize=address` | overflow, use-after-free, double-free, leaks | 2× time, 3× memory |
| `-fsanitize=undefined` | signed overflow, bad shifts, misalignment, NULL deref | ~20% |
| `valgrind --leak-check=full` | leaks, uninitialised reads, invalid access — no recompile | 20–50× time |
| `-fsanitize=leak` | leaks only | small |
| `mtrace` / `MALLOC_CHECK_=3` | glibc's own heap checks | small |

ASan and valgrind overlap but are not identical: valgrind catches **uninitialised reads**
that ASan misses; ASan catches **stack** overflows and is far faster. Use both.

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_memory_layout.c` | The real addresses of every segment; stack growth direction |
| `practice/02_malloc_basics.c` | All four functions, the idioms, the `realloc` traps, growth strategy |
| `practice/03_dynamic_2d.c` | Three 2D layouts with allocation and cleanup, timed |
| `practice/04_arena_allocator.c` | A complete arena, with alignment and a scratch-marker API |
| `practice/05_my_malloc.c` | **A working `malloc`/`free`/`realloc` on `mmap`**, with heap visualisation |
| `practice/06_pool_allocator.c` | Fixed-size pool with an intrusive free list, benchmarked against `malloc` |
| `practice/07_refcount.c` | Reference counting: shared ownership without a GC |
| `practice/08_ownership_patterns.c` | Create/destroy pairs, borrowed vs owned, cleanup on error paths |

```bash
cd 05-Memory-Management
gcc -std=c17 -Wall -Wextra -g -O2 practice/05_my_malloc.c -o my_malloc && ./my_malloc
valgrind --leak-check=full ./my_malloc      # zero leaks by construction
```

---

## Checklist

- [ ] You write `malloc(n * sizeof *p)`, never `malloc(n * sizeof(int))`.
- [ ] You check every allocation for NULL.
- [ ] You never write `p = realloc(p, n)`.
- [ ] You use `calloc` when the count comes from outside your program.
- [ ] You grow arrays by doubling, not by one.
- [ ] You can say, for every pointer in your program, who frees it.
- [ ] You have read your own `malloc` implementation and know where `free` gets the size.
- [ ] You run valgrind and ASan before believing anything works.
