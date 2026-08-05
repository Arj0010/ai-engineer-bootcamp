# Module 00 — Setup and Toolchain

Before you write a line of C you need to understand what actually happens when you press
"compile". In most languages you can ignore this. In C you cannot, because the errors you
will get come from four different programs and each one fails in its own dialect.

---

## 1. What C actually is

C is a thin, portable layer over what a CPU can do. It has:

- A handful of data types that correspond to machine registers and memory cells.
- Direct access to memory addresses.
- No garbage collector, no bounds checking, no exceptions, no runtime to speak of.

The consequence: **C will let you do anything, including things that are meaningless.**
The language defines a set of legal programs; step outside it and the standard says the
behaviour is *undefined* — the compiler may do anything at all, including working fine
on your machine and corrupting memory on someone else's. Learning C is largely learning
where that boundary is.

---

## 2. The four stages of compilation

`gcc hello.c -o hello` looks like one step. It is four.

```
hello.c
   │
   │  1. PREPROCESSOR   (cpp)      handles #include, #define, #ifdef
   ▼
hello.i          ← still C, but expanded: ~700 lines after one #include <stdio.h>
   │
   │  2. COMPILER      (cc1)       C → assembly for your CPU
   ▼
hello.s          ← x86-64 / ARM assembly text
   │
   │  3. ASSEMBLER     (as)        assembly → machine code
   ▼
hello.o          ← object file: machine code + a table of unresolved symbols
   │
   │  4. LINKER        (ld)        resolve symbols, pull in libc, lay out the binary
   ▼
hello            ← executable
```

Run each stage yourself — this is the single most clarifying exercise in this module:

```bash
gcc -E hello.c -o hello.i    # stop after preprocessing
gcc -S hello.i -o hello.s    # stop after compiling to assembly
gcc -c hello.s -o hello.o    # stop after assembling
gcc    hello.o -o hello      # link
```

`practice/compile_stages.sh` does all four and prints what changed at each step.

**Why this matters for debugging:**

| Error looks like | Which stage | Typical cause |
|---|---|---|
| `fatal error: foo.h: No such file` | preprocessor | wrong include path (`-I`) |
| `error: expected ';' before ...` | compiler | syntax |
| `warning: implicit declaration of 'sqrt'` | compiler | forgot `#include <math.h>` |
| `undefined reference to 'sqrt'` | **linker** | forgot to link the library (`-lm`) |
| `undefined reference to 'main'` | linker | no `main`, or you compiled a library as a program |

"Undefined reference" is *never* a syntax problem. It means the compiler believed you that
a function exists and the linker could not find its body.

---

## 3. Flags you should always use

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -g -O2 prog.c -o prog -lm
```

| Flag | Why |
|---|---|
| `-std=c17` | Pin the language version. Without it you get whatever your compiler defaults to, which changes between machines. |
| `-Wall` | "All" the common warnings. Misleading name — it is not all of them. |
| `-Wextra` | The rest of the useful ones: unused parameters, sign comparison, missing field initialisers. |
| `-Wpedantic` | Complain about anything that is a GNU extension rather than ISO C. Keeps your code portable. |
| `-g` | Emit debug symbols so `gdb` can show you source lines and variable names. Costs nothing at runtime. |
| `-O2` | Optimise. Also enables extra dataflow analysis, so `-O2` finds warnings `-O0` misses. |
| `-lm` | Link the math library. Needed for `sqrt`, `pow`, `sin`. Must come **after** the source files. |

**Treat warnings as errors while learning:** add `-Werror`. It is annoying and it will save
you hours.

### The debugging build

```bash
gcc -std=c17 -Wall -Wextra -g -O0 -fsanitize=address,undefined prog.c -o prog -lm
```

- `-O0` — do not optimise, so single-stepping in the debugger matches the source.
- `-fsanitize=address` — AddressSanitizer. Turns buffer overflows, use-after-free, and
  double-free into an immediate crash with a stack trace instead of silent corruption.
- `-fsanitize=undefined` — UBSan. Catches signed overflow, bad shifts, misaligned access,
  null dereference.

Sanitizers cost ~2× runtime and ~3× memory. Ship without them; develop with them.

---

## 4. The tools around the compiler

| Tool | What it does | Command to know |
|---|---|---|
| `gdb` | Interactive debugger | `gdb ./prog` then `run`, `bt`, `p x`, `break main` |
| `valgrind` | Memory error detector (no recompile needed) | `valgrind --leak-check=full ./prog` |
| `make` | Rebuild only what changed | `make` |
| `objdump` | Disassemble a binary | `objdump -d --demangle prog \| less` |
| `nm` | List symbols in an object file | `nm -C prog.o` |
| `strace` | Trace system calls (Linux) | `strace ./prog` |
| `perf` | CPU profiler (Linux) | `perf stat ./prog` |
| `clang-format` | Auto-format source | `clang-format -i *.c` |

Module 14 covers `gdb` and `valgrind` properly. For now, just know they exist.

---

## 5. Anatomy of the smallest real program

```c
#include <stdio.h>          /* 1 */

int main(void) {            /* 2 */
    printf("Hello, world\n");   /* 3 */
    return 0;               /* 4 */
}
```

1. **`#include <stdio.h>`** — the preprocessor pastes in the *declarations* of `printf`
   and friends. It does not include their code; that comes from libc at link time.
   `<...>` searches system directories; `"..."` searches your project first.

2. **`int main(void)`** — the entry point. Two legal signatures in ISO C:
   `int main(void)` and `int main(int argc, char **argv)`. It returns `int`.
   Writing `void main()` is wrong; some compilers accept it, the standard does not.
   `(void)` means "takes no arguments"; empty `()` means "unspecified arguments"
   which disables argument checking — always write `(void)`.

3. **`printf(...)`** — `\n` is a newline escape. Without it, output may sit in a buffer
   and not appear when you expect. stdout is line-buffered to a terminal and
   *block*-buffered to a pipe or file, which is why `./prog | cat` can reorder output.

4. **`return 0;`** — the exit status. `0` means success by convention (`EXIT_SUCCESS`);
   non-zero means failure (`EXIT_FAILURE`). The shell reads it via `echo $?`.
   `main` is special: falling off the end implicitly returns 0. No other function gets that.

---

## Practice

| File | What it shows |
|---|---|
| `practice/hello.c` | The minimal program, annotated |
| `practice/compile_stages.sh` | Runs all four stages and shows the intermediate output |
| `practice/args.c` | `argc`/`argv`, exit codes, `stderr` vs `stdout` |
| `practice/warnings_demo.c` | Code that compiles clean without `-Wall` and is badly broken |

```bash
gcc -std=c17 -Wall -Wextra -g -O2 practice/hello.c -o hello && ./hello
bash practice/compile_stages.sh
gcc -std=c17 -Wall -Wextra practice/args.c -o args && ./args one two three; echo "exit=$?"
```

For `warnings_demo.c`, compile it twice and compare:

```bash
gcc practice/warnings_demo.c -o wd            # silence
gcc -Wall -Wextra practice/warnings_demo.c -o wd   # four real bugs
```

---

## Checklist before moving on

- [ ] You can name the four compilation stages and what each consumes and produces.
- [ ] You know that "undefined reference" is a linker error, not a syntax error.
- [ ] You have `-Wall -Wextra` in your fingers, not in a note.
- [ ] You have run a program under `-fsanitize=address` at least once.
- [ ] You know why `printf` output can appear "out of order" when piped.
