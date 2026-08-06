# Module 02 — Control Flow and Functions

Branching, looping, and the unit of reuse. Also: the four storage classes, which decide
*where in memory* a variable lives and *how long it lives* — a topic usually taught badly
and then needed constantly.

---

## 1. Selection

### `if` / `else`

C has no boolean type until you `#include <stdbool.h>`, and even then `bool` is just an
integer that holds 0 or 1. **Any scalar expression is a condition**: zero is false,
everything else is true. A pointer is true when non-NULL.

```c
if (n)          /* true when n != 0     */
if (p)          /* true when p != NULL  */
if (!strcmp(a, b))   /* true when the strings are EQUAL — strcmp returns 0 */
```

That last one reads backwards and is a classic source of inverted logic. Prefer
`if (strcmp(a, b) == 0)`.

**Always brace your blocks.** The dangling-`else` and the "goto fail" bug both come from
unbraced bodies:

```c
if (x)
    a();
    b();        /* NOT part of the if. Runs unconditionally. */
```

### `switch`

```c
switch (expr) {          /* expr must be an INTEGER type (or enum) */
case 1:                  /* labels must be compile-time constants  */
case 2:
    do_something();
    break;               /* without break, control FALLS THROUGH */
case 3: {
    int local = 0;       /* a declaration needs its own block      */
    (void)local;
    break;
}
default:
    handle_other();
    break;
}
```

- **Fallthrough is the default and is a bug 95% of the time.** GCC's
  `-Wimplicit-fallthrough` (in `-Wextra`) catches it. When you *want* fallthrough, say so:
  `__attribute__((fallthrough));` or a `/* fall through */` comment.
- You cannot switch on a `float`, a `char *`, or a string.
- `default` can go anywhere, but put it last.
- A jump table is generated when the cases are dense, which makes `switch` genuinely
  faster than a chain of `if`s for many cases.

---

## 2. Iteration

```c
while (cond)     { }            /* test, then body                      */
do   { } while (cond);          /* body at least once, then test        */
for (init; cond; step) { }      /* all three parts are optional          */
for (;;) { }                    /* infinite loop, idiomatic              */
```

`for (int i = 0; ...)` — declaring the counter in the loop header is C99 and later.
Do it; it keeps the scope tight.

### `break`, `continue`, and breaking out of nested loops

`break` exits the innermost loop or `switch`. `continue` jumps to the next iteration
(in a `for`, that means the step expression *does* run).

C has no labelled break. The three legitimate options for exiting nested loops:

```c
/* 1. a flag */
bool found = false;
for (int i = 0; i < n && !found; i++)
    for (int j = 0; j < m; j++)
        if (a[i][j] == target) { found = true; break; }

/* 2. extract the loop into a function and return */
static int find(int a[][M], int n, int target) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < M; j++)
            if (a[i][j] == target) return i;
    return -1;
}

/* 3. goto — genuinely the clearest option here */
for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++)
        if (a[i][j] == target) goto found;
found: ;
```

### `goto` is not forbidden

The one place `goto` is *idiomatic* in C is cleanup on error paths, because C has no
destructors and no `finally`:

```c
int process(void)
{
    FILE *f = NULL;  char *buf = NULL;  int rc = -1;

    f = fopen("data", "rb");
    if (!f)                    goto out;
    buf = malloc(SIZE);
    if (!buf)                  goto out;
    if (fread(buf, 1, SIZE, f) != SIZE) goto out;

    rc = 0;                          /* success */
out:
    free(buf);                       /* free(NULL) is a no-op — safe */
    if (f) fclose(f);
    return rc;
}
```
This pattern is used throughout the Linux kernel. The alternative — nested `if`s or
repeated cleanup at every return — is worse. Rules: only jump **forward**, only jump
**out** of a scope (never into one), and use one exit label per resource level.

---

## 3. Functions

```c
return_type name(parameter_list) { body }
```

### Declaration vs definition

A **declaration** (prototype) tells the compiler the signature so it can type-check calls.
A **definition** provides the body. Headers carry declarations; `.c` files carry definitions.

```c
double area(double r);          /* declaration — note the semicolon */
double area(double r) { ... }   /* definition */
```

Always write `void` for an empty parameter list. `int f()` means "unspecified arguments"
and disables checking; `int f(void)` means "no arguments".

### Everything is pass-by-value

C has no pass-by-reference. **Every argument is copied.** To let a function modify the
caller's variable you pass its *address* — and then you are still passing a pointer by
value, you have just chosen to copy something that points at the original.

```c
void broken(int x)  { x = 42; }         /* modifies a copy; caller unaffected */
void works (int *x) { *x = 42; }        /* modifies what x points at */

int n = 0;
broken(n);   /* n is still 0 */
works(&n);   /* n is now 42 */
```

Arrays are the apparent exception: an array argument *decays* to a pointer to its first
element, so the function receives an address and can modify the caller's data. It is still
pass-by-value — the *pointer* was copied. This is module 03/04 territory.

### Recursion

Every recursive function needs a **base case** and a **step that provably approaches it**.
Each call consumes a stack frame; the default stack is ~8 MB on Linux, so a few hundred
thousand frames will `SIGSEGV` (stack overflow).

```c
long factorial(int n) {
    if (n <= 1) return 1;             /* base case */
    return n * factorial(n - 1);      /* strictly smaller argument */
}
```

**Tail recursion** — where the recursive call is the very last thing — can be converted
by the compiler into a loop at `-O2`, using constant stack. C does not *guarantee* this,
so never rely on it for correctness; if depth is unbounded, write the loop yourself.

Naive recursion can be exponentially wasteful: `fib(n)` calling itself twice is O(φⁿ).
`practice/02_recursion.c` shows the same function memoised and iterative, with timings.

---

## 4. Scope, lifetime, and linkage — the three separate questions

People conflate these constantly. They are independent.

| Question | Answer determined by | Possible values |
|---|---|---|
| **Scope** — where the *name* is visible | where you declare it | block, file, function, prototype |
| **Lifetime** — how long the *object* exists | storage class | automatic, static, allocated, thread |
| **Linkage** — whether the name refers to the same object in other translation units | `static` / `extern` / default | none, internal, external |

### The storage class specifiers

| Keyword | Lifetime | Linkage | Where it lives |
|---|---|---|---|
| *(none, inside a block)* | automatic — created at entry, destroyed at exit | none | stack |
| `static` inside a block | **whole program** | none | data/bss segment |
| `static` at file scope | whole program | **internal** — invisible to other `.c` files | data/bss |
| *(none, at file scope)* | whole program | external — visible everywhere | data/bss |
| `extern` | whole program | external — "defined elsewhere" | data/bss |
| `register` | automatic | none | hint only; ignored by modern compilers |
| `_Thread_local` (C11) | per-thread | as declared | TLS |

The two meanings of `static` are unrelated and both matter:

```c
/* file scope: "private to this .c file" — the closest C has to encapsulation */
static int counter = 0;
static void helper(void) { }   /* not linkable from another file */

/* block scope: "one instance, persists across calls" */
int next_id(void) {
    static int id = 0;         /* initialised ONCE, at program start */
    return ++id;               /* returns 1, 2, 3, ... */
}
```

`static` at file scope should be your default for anything not in a header. It shrinks the
public surface, lets the compiler inline and optimise more aggressively, and prevents
name collisions at link time.

### Initialisation rules — worth memorising

- **Automatic** (local, non-static) variables are **uninitialised**. They contain garbage.
  Reading them is UB.
- **Static and global** variables are **zero-initialised** before `main` runs. Pointers get
  NULL, floats get 0.0. This is guaranteed.
- Static/global initialisers must be **constant expressions** — you cannot write
  `static int x = rand();`.

---

## 5. `inline` and function-like macros

```c
static inline int max_i(int a, int b) { return a > b ? a : b; }
```

`static inline` in a header is the safe, portable way to get an inlinable helper.
(Plain `inline` without `static` or `extern` has surprising one-definition rules.)

Prefer it over a macro whenever the types are fixed, because a macro:

```c
#define MAX(a, b) ((a) > (b) ? (a) : (b))
MAX(i++, j)     /* i++ evaluated TWICE */
```

evaluates its arguments more than once. Module 07 covers macros properly, and module 11
covers `_Generic` for when you genuinely need type-independence.

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_control_flow.c` | if/switch/loops, fallthrough, breaking out of nested loops, `goto` cleanup |
| `practice/02_recursion.c` | Factorial, Fibonacci naive vs memoised vs iterative (timed), Ackermann, Towers of Hanoi, tail recursion |
| `practice/03_scope_and_storage.c` | All four storage classes, `static` in both meanings, initialisation guarantees, where each variable lives in memory |
| `practice/04_function_design.c` | Pass-by-value vs pointer, multiple return values, error-return conventions, variadic sum |

---

## Checklist

- [ ] You brace every `if` body, even one-liners.
- [ ] You know `switch` falls through by default and `-Wextra` will tell you.
- [ ] You can explain the two meanings of `static` without hesitating.
- [ ] You know locals are garbage and globals are zero, and why.
- [ ] You can write the `goto out:` cleanup pattern from memory.
- [ ] You understand that C passes everything by value, including pointers.
