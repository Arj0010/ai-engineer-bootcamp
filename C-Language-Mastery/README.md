# C Language Mastery — From First Principles to Machine Learning

A complete, self-contained path for learning C from absolute zero to the level where you can
write allocators, data structures, systems code, and machine learning engines **by hand**.

Every module has two halves:

| Half | What it is |
|---|---|
| **Theory** (`README.md` in each module) | The mental model. Why the language does what it does, what the machine is doing underneath, and the traps. |
| **Practice** (`practice/`) | Real, compilable, runnable C programs. Every file has a `main()` unless it is part of a multi-file project with its own `Makefile`. |

Most modules also ship an `exercises.md` with problems and worked solutions.

---

## The Roadmap

### Tier 0 — Getting the machine to talk back
| Module | Topic |
|---|---|
| [00 — Setup and Toolchain](00-Setup-and-Toolchain/) | Compiler, the four compilation stages, flags you should always use |

### Tier 1 — The Language Core
| Module | Topic |
|---|---|
| [01 — Fundamentals](01-Fundamentals/) | Types, sizes, integer representation, operators, `printf`/`scanf`, promotion rules |
| [02 — Control Flow and Functions](02-Control-Flow-and-Functions/) | Branching, loops, recursion, scope, storage classes, linkage |
| [03 — Arrays and Strings](03-Arrays-and-Strings/) | Arrays, decay, C strings, `<string.h>`, safe string handling |

### Tier 2 — The Part Everyone Gets Wrong
| Module | Topic |
|---|---|
| [04 — Pointers](04-Pointers/) | Address arithmetic, arrays vs pointers, `const` placement, function pointers, multi-level indirection |
| [05 — Memory Management](05-Memory-Management/) | Stack vs heap, `malloc`/`free`, alignment, leaks, arena and pool allocators, **writing your own `malloc`**, reference counting |
| [06 — Structs, Unions, Enums](06-Structs-Unions-Enums/) | Padding and alignment, bitfields, tagged unions, opaque types, flexible array members |

### Tier 3 — Programs, Not Just Files
| Module | Topic |
|---|---|
| [07 — File I/O and the Preprocessor](07-File-IO-and-Preprocessor/) | Streams, text vs binary, CSV parsing, macros, X-macros, include guards |
| [08 — Modular Programming and Build Systems](08-Modular-Programming-and-Build-Systems/) | Translation units, headers, linkage, static and shared libraries, `make` |

### Tier 4 — Computer Science in C
| Module | Topic |
|---|---|
| [09 — Data Structures](09-Data-Structures/) | Dynamic array, linked lists, stack, queue, ring buffer, hash table, BST, AVL, heap, trie, graph, union-find, LRU cache |
| [10 — Algorithms](10-Algorithms/) | Complexity, sorting, searching, divide & conquer, DP, greedy, backtracking, graph algorithms, string algorithms, bit tricks |

### Tier 5 — Advanced C
| Module | Topic |
|---|---|
| [11 — Advanced C](11-Advanced-C/) | Bit manipulation, `_Generic`, varargs, `setjmp`, generic containers via `void*`, UB, `volatile`/`restrict`, atomics |
| [12 — Systems Programming](12-Systems-Programming/) | Syscalls, processes, pipes, signals, pthreads, mutex/condvar, thread pool, `mmap`, TCP sockets |
| [13 — Performance and Optimization](13-Performance-and-Optimization/) | Cache hierarchy, locality, tiling, branch prediction, SIMD intrinsics, profiling |
| [14 — Debugging and Testing](14-Debugging-and-Testing/) | `gdb`, `valgrind`, ASan/UBSan, a minimal unit test framework, deliberate bug hunts |

### Tier 6 — Numerical and Machine Learning
| Module | Topic |
|---|---|
| [15 — Numerical Computing](15-Numerical-Computing/) | IEEE-754, floating point traps, a matrix library, Gaussian elimination, RNGs |
| [16 — Machine Learning in C](16-Machine-Learning-in-C/) | Linear regression → logistic regression → k-NN/k-means → decision trees → **a neural network from scratch** → **a reverse-mode autodiff engine** → convolutions and an inference loop |

### Tier 7 — Build Real Things
| Module | Topic |
|---|---|
| [17 — Projects](17-Projects/) | A shell, a JSON parser, a stack-based virtual machine, a mini ML framework |
| [Reference](Reference/) | Cheat sheet, standard library map, pitfalls, interview questions, glossary |

---

## How to use this

**Do not read it like a book.** Read the theory for a module, then type the practice programs
out yourself (do not copy-paste — the muscle memory is the point), compile them, break them
deliberately, and fix them.

A realistic pace:

- **Weeks 1–2** — Modules 00–03. Get fluent enough that syntax stops costing you thought.
- **Weeks 3–5** — Modules 04–06. This is the hump. Pointers and memory are the whole language.
- **Weeks 6–7** — Modules 07–08. You can now build multi-file programs.
- **Weeks 8–11** — Modules 09–10. Implement every data structure without looking.
- **Weeks 12–15** — Modules 11–14. This is where you become dangerous.
- **Weeks 16–20** — Modules 15–17. Numerics and ML, then ship a project.

---

## Building and running everything

```bash
cd C-Language-Mastery

# Build every single-file program in every module into build/
make

# Build and run a module's programs
make M=04-Pointers

# Compile + run one file quickly
make run FILE=05-Memory-Management/practice/03_arena_allocator.c

# Build with AddressSanitizer (catches use-after-free, overflow, leaks)
make asan

# Verify every program in the repo still compiles
./check.sh

# Remove all build output
make clean
```

Multi-file projects (module 08, parts of 16, and all of 17) have their own `Makefile`;
`cd` into the directory and run `make`.

### The flags used everywhere

```
-std=c17 -Wall -Wextra -Wpedantic -g -O2
```

`-Wall -Wextra` is not optional. In C, most warnings are bugs that have not happened yet.
For debugging sessions, swap `-O2` for `-O0 -fsanitize=address,undefined`.

---

## Prerequisites

Nothing but a terminal and a compiler. Verify yours:

```bash
gcc --version    # or clang --version
make --version
gdb --version
valgrind --version
```

On Debian/Ubuntu: `sudo apt install build-essential gdb valgrind`.
On macOS: `xcode-select --install` (use `lldb` and `leaks` in place of `gdb`/`valgrind`).

---

## A note on the C standard

Everything here targets **C17** (`-std=c17`), which is C11 with defect fixes and is what
almost every current compiler defaults to. Where something is C99-only, C11-only, or a
POSIX/GNU extension rather than ISO C, the theory notes say so explicitly, because that
distinction matters the moment you compile on a different platform.
