# Module 04 — Pointers

This is the module the whole language turns on. If pointers click, C is easy. If they do
not, everything after this feels like memorisation.

The core idea is embarrassingly simple: **memory is a big array of bytes, each byte has a
number (its address), and a pointer is a variable holding one of those numbers.** The type
of the pointer tells the compiler how many bytes to read at that address and how to
interpret them.

---

## 1. The three operators

```c
int  x = 42;
int *p = &x;        /* & = "address of"   -> p now holds x's address */
int  y = *p;        /* * = "dereference"  -> read the int at that address */
*p = 99;            /* write through the pointer -> x is now 99 */
```

The `*` in a declaration and the `*` in an expression are different things that share a
symbol:

- `int *p;` — declares `p` as "a pointer to int".
- `*p` — in an expression, "the thing p points at".

Read declarations right-to-left from the name:

```c
int *p;             /* p is a pointer to int */
int **pp;           /* pp is a pointer to a pointer to int */
int *a[10];         /* a is an array of 10 pointers to int */
int (*a)[10];       /* a is a pointer to an array of 10 ints */
int (*f)(int);      /* f is a pointer to a function taking int, returning int */
int *f(int);        /* f is a function taking int, returning a pointer to int */
```

The `[]` and `()` bind tighter than `*`, which is why the parentheses change everything.
When it gets hairy: `cdecl` or a `typedef`.

---

## 2. NULL

`NULL` is a null pointer constant — a pointer value guaranteed not to be the address of
any object. It compares equal to no valid pointer.

```c
int *p = NULL;
if (p != NULL) { ... }      /* or just if (p) */
*p;                         /* UNDEFINED BEHAVIOUR — usually SIGSEGV */
```

- **Dereferencing NULL is undefined behaviour**, not "a crash". On most desktop OSes page
  zero is unmapped so you get a segfault, which is the good outcome. On some embedded
  systems address 0 is real memory and you silently corrupt it.
- Always initialise pointers. `int *p;` then `*p = 1;` writes to a garbage address.
- After `free(p)`, set `p = NULL`. Then a later use crashes loudly instead of corrupting
  memory quietly, and a double `free(NULL)` is a defined no-op.

---

## 3. Pointer arithmetic

**Pointer arithmetic is in units of the pointed-to type, not bytes.**

```c
int a[5] = {10, 20, 30, 40, 50};
int *p = a;
p + 1        /* address + 4 bytes (sizeof(int)) */
*(p + 2)     /* == a[2] == 30 */
p++          /* advance one int */
```

| Operation | Legal? | Result |
|---|---|---|
| `p + n`, `p - n` | yes, within the array (and one past the end) | pointer |
| `p - q` | yes, if both point into the **same** array | `ptrdiff_t`: element count |
| `p + q` | **no** | meaningless |
| `p < q`, `p == q` | only within the same array | comparison |
| `*(a + n)` | identical to `a[n]` | element |

The rules are strict for a reason: the compiler uses them to optimise. Computing a pointer
more than one past the end of an array is undefined behaviour *even if you never
dereference it*.

```c
int a[5];
int *end = a + 5;    /* legal: one-past-the-end is a valid ADDRESS */
*end;                /* UB: it is not a valid OBJECT */
int *bad = a + 6;    /* UB just to compute */
```

The one-past-the-end guarantee is what makes the standard loop idiom legal:

```c
for (int *p = a; p != a + 5; p++) { ... }
```

---

## 4. `const` and pointers — read right to left

```c
const int *p;          /* pointer to const int:  *p is read-only, p can move   */
int const *p;          /* identical to the above */
int * const p;         /* const pointer to int:  p cannot move, *p is writable */
const int * const p;   /* neither can change */
```

Mnemonic: **`const` applies to whatever is immediately to its left; if there is nothing to
its left, it applies to what is on its right.**

`const` on a parameter is a contract with your caller:

```c
size_t count(const int *data, size_t n);   /* "I will not modify your array" */
```

It is checked by the compiler, it documents intent, and it lets the optimiser assume the
data does not change. Use it on every input parameter you do not write to.

Note that `const` is shallow: `const struct S *p` means you cannot write `p->field`, but
if `field` is itself a pointer, you *can* write through it.

---

## 5. `void *` — the generic pointer

```c
void *p;            /* points at "something"; size and interpretation unknown */
```

- Any object pointer converts to `void *` and back without a cast, losslessly.
- **You cannot dereference it** or do arithmetic on it (GCC allows `void*` arithmetic as
  an extension, treating it as `char*` — `-Wpedantic` will complain; do not rely on it).
- This is how `malloc`, `memcpy`, and `qsort` are type-agnostic.

```c
void *malloc(size_t size);
int *a = malloc(n * sizeof *a);      /* no cast needed in C */
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));
```

**Do not cast the return of `malloc` in C.** It is unnecessary, and before C99 it could
hide a missing `#include <stdlib.h>`. (In C++ the cast is required — but that is C++.)

Note: converting between object pointers and *function* pointers is not guaranteed by ISO
C, which is why `dlsym` returns `void *` and every portable program winces about it.

---

## 6. Function pointers

```c
int add(int a, int b) { return a + b; }

int (*op)(int, int) = add;      /* &add and add are equivalent */
int r = op(2, 3);               /* (*op)(2, 3) also works */
```

A `typedef` makes them readable, and you should always use one:

```c
typedef int (*BinaryOp)(int, int);
BinaryOp op = add;
```

Function pointers are how you get polymorphism, callbacks, and dispatch tables in C:

```c
/* dispatch table — replaces a long if/else chain, and is O(1) */
static const struct { char sym; BinaryOp fn; } ops[] = {
    {'+', add}, {'-', sub}, {'*', mul}
};

/* callback — how qsort sorts anything */
qsort(arr, n, sizeof arr[0], compare_ints);

/* a vtable — how you build "objects" in C */
typedef struct {
    void (*draw)(void *self);
    void (*free)(void *self);
} ShapeVTable;
```

This is the mechanism behind the Linux kernel's `file_operations`, every plugin system in
C, and the "objects in C" pattern in module 06.

---

## 7. Multi-level pointers

`T **` shows up in exactly three situations, and it is worth naming them so you recognise
which one you are in:

1. **An out-parameter that is itself a pointer** — the callee needs to modify the caller's
   pointer.
   ```c
   int alloc_buffer(char **out, size_t n) { *out = malloc(n); return *out ? 0 : -1; }
   char *buf; alloc_buffer(&buf, 100);
   ```
2. **An array of pointers** — `char *argv[]` is `char **argv`. Each element points at a
   separate string, which may be anywhere in memory.
3. **A jagged 2D structure** — `int **matrix`, where `matrix[i]` is a separately allocated
   row. This is *not* the same layout as `int m[3][4]` (which is one contiguous block),
   and mixing them up is a common crash. Module 05 builds both.

---

## 8. The failure modes

| Bug | What it is | How to catch it |
|---|---|---|
| **Dangling pointer** | Points at memory that has been freed or whose scope ended | ASan, valgrind; set to NULL after free |
| **Wild pointer** | Never initialised; holds garbage | Initialise at declaration |
| **Use-after-free** | Dereferencing a freed pointer | ASan (`heap-use-after-free`) |
| **Double free** | `free`ing the same block twice | ASan; NULL after free |
| **Returning `&local`** | The frame is gone | `-Wreturn-local-addr` |
| **Off-by-one** | `<=` instead of `<` on the bound | ASan, careful loops |
| **Aliasing violation** | Accessing an object through an incompatible pointer type | `-Wstrict-aliasing`, use `memcpy` |

**Strict aliasing** deserves a note because it surprises experienced people. The compiler
assumes that pointers of different types never point at the same object, and optimises on
that basis:

```c
float f = 1.0f;
int i = *(int *)&f;          /* UB — type punning through a cast */
int i; memcpy(&i, &f, sizeof i);   /* correct, and compiles to the same thing */
```

`char *` and `unsigned char *` may alias anything — that exemption is what makes `memcpy`
and byte-inspection legal. Unions are also a legal punning mechanism in C (unlike C++).

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_pointer_basics.c` | `&`, `*`, NULL, what an address looks like, pointers to different types |
| `practice/02_pointer_arithmetic.c` | Scaling by type size, `p - q`, one-past-the-end, array traversal idioms |
| `practice/03_const_and_void.c` | `const` placement, `void *`, generic swap/find/foreach, `qsort`, type punning |
| `practice/04_function_pointers.c` | Callbacks, `qsort`, dispatch tables, a vtable, a state machine |
| `practice/05_multilevel_pointers.c` | `T **` in all three roles; contiguous vs jagged vs hybrid 2D; `argv` |
| `practice/broken/06_pointer_bugs.c` | Every failure mode above, each triggerable by a flag under ASan |

The last file lives under `broken/` because it deliberately contains bugs, so the
repo-wide build skips it. Build it by hand with the sanitizers on — reading the ASan
report is the skill it teaches:

```bash
cd 04-Pointers/practice/broken
gcc -std=c17 -Wall -Wextra -g -O0 -fsanitize=address,undefined 06_pointer_bugs.c -o bugs
./bugs            # explains all nine bugs, triggers none
./bugs list       # the flags
./bugs uaf        # heap-use-after-free, with both stack traces
./bugs leak       # LeakSanitizer report at exit
valgrind --leak-check=full ./bugs leak
```

---

## Checklist

- [ ] You can read `int (*f)(int)` and `int *f(int)` aloud correctly.
- [ ] You know `p + 1` advances by `sizeof(*p)` bytes, not 1.
- [ ] You know why `const int *p` and `int * const p` are different.
- [ ] You never cast the result of `malloc`.
- [ ] You set pointers to NULL after freeing them.
- [ ] You can explain why `int **` is not interchangeable with `int [3][4]`.
- [ ] You use `memcpy`, not a pointer cast, for type punning.
