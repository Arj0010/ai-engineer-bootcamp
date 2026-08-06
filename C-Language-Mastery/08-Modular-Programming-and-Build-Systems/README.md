# Module 08 — Modular Programming and Build Systems

How to build a program out of more than one file, and how the linker turns those files
into an executable. Once this clicks, "undefined reference" stops being mysterious.

---

## 1. Translation units

A **translation unit** is one `.c` file after the preprocessor has run — so it includes
every header it pulled in. The compiler processes one at a time, in complete isolation,
and emits one `.o` object file. **The linker** then resolves the cross-references.

```
math_utils.c  ──cc──▶  math_utils.o  ┐
main.c        ──cc──▶  main.o        ├──ld──▶  program
                       libc.so       ┘
```

Each `.o` contains machine code plus a **symbol table**: symbols it *defines* and symbols
it *needs*. `nm` shows you both:

```bash
nm main.o
                 U compute_stats     # U = Undefined: needed from elsewhere
0000000000000000 T main              # T = in the Text section: defined here
```

If no object file defines a symbol something needs, you get
`undefined reference to 'compute_stats'` — a **linker** error, always. It is never a syntax
problem.

---

## 2. Headers: declarations, not definitions

**The rule: headers declare, `.c` files define.**

A header may contain:
- function **declarations** (prototypes)
- `struct`/`union`/`enum`/`typedef` definitions
- `#define`s
- `extern` variable **declarations**
- `static inline` function definitions

A header must **not** contain:
- function definitions (without `static inline`)
- variable definitions

Put a definition in a header, include it from two `.c` files, and you get
`multiple definition of ...` at link time.

### Include guards

```c
#ifndef MATH_UTILS_H
#define MATH_UTILS_H
...
#endif
```

Without these, a header included twice (directly and transitively) redefines its types and
the compile fails. `#pragma once` is shorter and universally supported, but not ISO C.

### `#include "..."` vs `<...>`

`"local.h"` searches your project directory first, then the system paths.
`<stdio.h>` searches only the system paths (plus anything you add with `-I`).
Use quotes for your own headers, angle brackets for system and third-party ones.

### What to include where

Include in the **header** only what the header's own declarations need — a `struct` used by
value, a `size_t`. Include everything else in the **`.c` file**. A header that pulls in ten
others makes every file that touches it slow to compile.

**Forward declarations** help: if a header only needs `struct Foo *`, declare
`struct Foo;` rather than including `foo.h`.

---

## 3. Linkage — the rules that decide what the linker sees

| Declaration | Linkage | Meaning |
|---|---|---|
| `int x;` at file scope | external | one definition, visible to every `.c` file |
| `static int x;` at file scope | **internal** | private to this translation unit |
| `extern int x;` | external | "defined elsewhere" — a declaration, not a definition |
| `void f(void) {}` | external | callable from other files |
| `static void f(void) {}` | **internal** | private |

**Make everything `static` unless it is declared in a header.** It shrinks your public
surface, prevents name collisions at link time, and lets the compiler inline and discard
freely.

### Sharing a global correctly

```c
/* config.h */
extern int g_verbose;          /* DECLARATION — no storage */

/* config.c */
int g_verbose = 0;             /* DEFINITION — exactly one, in exactly one .c */
```

Getting this backwards (definition in the header) gives `multiple definition` as soon as
two files include it.

---

## 4. `make`

`make` rebuilds only what changed, by comparing file timestamps against a dependency graph.

```makefile
target: prerequisites
	recipe          # MUST be indented with a TAB, not spaces
```

A practical Makefile:

```makefile
CC      := gcc
CFLAGS  := -std=c17 -Wall -Wextra -Wpedantic -g -O2
LDLIBS  := -lm

SRCS    := $(wildcard *.c)
OBJS    := $(SRCS:.c=.o)
DEPS    := $(OBJS:.o=.d)
TARGET  := program

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# -MMD -MP makes the compiler generate the header dependencies for us
%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
```

Automatic variables: `$@` the target, `$<` the first prerequisite, `$^` all prerequisites.

**Automatic header dependencies (`-MMD -MP`) are the single most important thing here.**
Without them, editing a header does not trigger a rebuild of the files that include it, and
you get baffling inconsistencies — a struct whose layout differs between two `.o` files.

---

## 5. Libraries

### Static (`.a`)

```bash
gcc -c -o mathlib.o mathlib.c
ar rcs libmathlib.a mathlib.o
gcc main.c -L. -lmathlib -o prog
```

An archive of `.o` files. The linker copies the members you actually use **into** your
executable. Result: a bigger binary, no runtime dependency, and the library version is
frozen at build time.

### Shared (`.so` / `.dylib` / `.dll`)

```bash
gcc -fPIC -c -o mathlib.o mathlib.c        # position-independent code
gcc -shared -o libmathlib.so mathlib.o
gcc main.c -L. -lmathlib -o prog
LD_LIBRARY_PATH=. ./prog                    # found at RUN time
```

Loaded at run time. Smaller binaries, one copy shared between processes, and the library
can be patched without relinking — but the program will not start if it is missing or the
ABI changed.

**Link order matters for static libraries.** `gcc -lm main.c` fails where
`gcc main.c -lm` succeeds: the linker processes arguments left to right and only pulls in
archive members that resolve symbols it has *already* seen.

---

## 6. Project layout

```
project/
├── Makefile
├── include/            # PUBLIC headers — the API
│   └── mathlib.h
├── src/                # implementation + private headers
│   ├── mathlib.c
│   └── internal.h
├── tests/
│   └── test_mathlib.c
└── build/              # generated; never committed
```

Compile with `-Iinclude`, so `#include "mathlib.h"` works from anywhere.

---

## Practice

`practice/mathlib/` is a complete small project:

```
mathlib/
├── Makefile            # incremental build, auto header deps, static + shared libs
├── include/mathlib.h   # the public API
├── src/stats.c         # implementation
├── src/vector.c        # implementation
├── src/internal.h      # private header — not part of the API
├── src/main.c          # the demo program
└── tests/test_mathlib.c
```

```bash
cd practice/mathlib
make            # build the program
make test       # build and run the tests
make static     # build libmathlib.a and link against it
make shared     # build libmathlib.so and link against it
make symbols    # show what nm reports about internal vs external linkage
make clean
```

Then: `touch include/mathlib.h && make` and watch **only** the affected files rebuild.

---

## Checklist

- [ ] You know "undefined reference" is a linker error and what causes it.
- [ ] Your headers contain declarations, never definitions.
- [ ] Every header has an include guard.
- [ ] Everything not in a header is `static`.
- [ ] Shared globals are `extern` in the header, defined in exactly one `.c`.
- [ ] Your Makefile uses `-MMD -MP` for automatic header dependencies.
- [ ] You put `-l` flags *after* the source files.
