# Module 07 — File I/O and the Preprocessor

Two unrelated topics that share a module because they are both "things you need before you
can write a real program".

---

## 1. Streams

`<stdio.h>` gives you buffered I/O over a `FILE *`. Three streams are open at start:
`stdin`, `stdout`, `stderr`.

```c
FILE *f = fopen("data.txt", "r");
if (f == NULL) { perror("fopen"); return -1; }   /* ALWAYS check */
...
fclose(f);                                        /* flushes the buffer */
```

### Modes

| Mode | Meaning |
|---|---|
| `"r"` | read; fails if the file does not exist |
| `"w"` | write; **truncates to zero** or creates |
| `"a"` | append; writes always go to the end |
| `"r+"` | read and write; file must exist |
| `"w+"` | read and write; truncates |
| `"a+"` | read and append |
| `+"b"` | binary — `"rb"`, `"wb"`. **No-op on POSIX, essential on Windows** |

On Windows, text mode translates `\n` ↔ `\r\n` and can stop at a `0x1A` byte. Always add
`b` when the data is not text — the cost on Linux is zero.

### Reading

| Function | Use it for |
|---|---|
| `fgets(buf, size, f)` | **Line-based text.** Bounded, keeps the `\n`. |
| `fread(buf, size, n, f)` | Binary blocks. Returns the number of **items** read. |
| `fgetc(f)` / `getc(f)` | One byte. Returns `int` so it can also return `EOF`. |
| `fscanf(f, ...)` | Parsing — same hazards as `scanf`. Prefer `fgets` + `sscanf`. |
| `getline(&p, &n, f)` | POSIX. Reads a whole line, reallocating as needed. Excellent. |

**`fgetc` returns `int`, not `char`.** It must be able to return 256 distinct byte values
*plus* `EOF` (−1). Storing it in a `char` makes the byte `0xFF` indistinguishable from
`EOF`, which is a genuine, hard-to-find bug.

### Detecting the end

```c
while (fgets(line, sizeof line, f) != NULL) { ... }
if (ferror(f)) { /* a real error */ }
else if (feof(f)) { /* clean end of file */ }
```

**Never write `while (!feof(f))`.** `feof` only becomes true *after* a read has already
failed, so the loop body runs one extra time on stale data. Test the read's return value
instead — that is what it is for.

### Buffering

- **stdout to a terminal**: line-buffered (flushes on `\n`).
- **stdout to a pipe or file**: **block**-buffered (4–8 KB). This is why `./prog | cat` can
  appear to reorder output relative to `stderr`, which is unbuffered.
- `fflush(stdout)` forces it. Do this after a prompt with no trailing newline.
- `setvbuf` changes the policy. `fflush(stdin)` is **undefined behaviour** — it does not
  do what tutorials claim.

### Positioning

`fseek(f, offset, SEEK_SET|SEEK_CUR|SEEK_END)`, `ftell(f)`, `rewind(f)`.
For files over 2 GB use `fseeko`/`ftello` (POSIX) — `ftell` returns `long`, which is 32-bit
on Windows.

The common "get the file size" idiom (`fseek` to end, `ftell`) is **not portable for text
streams** and racy in general. On POSIX, `stat()` is the correct answer.

---

## 2. Binary I/O

```c
fwrite(&record, sizeof record, 1, f);
fread (&record, sizeof record, 1, f);
```

Fast and simple, and **completely non-portable** as a file format, because it bakes in:

- **struct padding** (differs by compiler and flags),
- **endianness** (x86 is little, network order is big),
- **type sizes** (`long` is 4 bytes on Windows, 8 on Linux).

For a file or protocol anyone else will read, serialise **field by field with explicit
widths and a defined byte order**. `practice/02_binary_io.c` shows both, and shows the
struct-dump approach breaking.

---

## 3. The preprocessor

It runs before the compiler and does **pure text substitution**. It does not understand C.

### Object-like macros

```c
#define MAX_USERS 100
#define PI 3.14159265358979
```

Prefer `enum { MAX_USERS = 100 };` or `static const double PI = ...;` — both are typed,
scoped, and visible to the debugger.

### Function-like macros — the four rules

```c
#define SQUARE(x) ((x) * (x))
```

1. **Parenthesise every parameter.** `#define SQ(x) x*x` makes `SQ(a+b)` become `a+b*a+b`.
2. **Parenthesise the whole body.** `#define DBL(x) (x)*2` makes `1/DBL(5)` become `1/(5)*2`.
3. **Never use an argument twice** if callers might pass a side effect. `MAX(i++, j)`
   increments `i` twice.
4. **Prefer `static inline`** whenever the types are fixed. You get type checking, single
   evaluation, and a debugger symbol.

### Multi-statement macros

```c
#define SWAP(a, b) do { int t = (a); (a) = (b); (b) = t; } while (0)
```

The `do { } while (0)` wrapper makes the macro a single statement, so it works in
`if (c) SWAP(x,y); else ...`. A bare `{ }` breaks that; the stray semicolon ends the `if`.

### Stringify and paste

```c
#define STR(x)   #x               /* STR(hello)      -> "hello"   */
#define XSTR(x)  STR(x)           /* expands x FIRST, then stringifies */
#define CAT(a,b) a##b             /* CAT(foo, bar)   -> foobar    */
```

`#` and `##` do **not** expand their arguments, which is why the two-level `XSTR` idiom
exists.

### Conditional compilation

```c
#ifdef DEBUG / #ifndef / #if defined(A) && !defined(B) / #elif / #else / #endif
```

Standard predefined macros: `__FILE__`, `__LINE__`, `__func__` (actually C99 language, not
a macro), `__DATE__`, `__TIME__`, `__STDC_VERSION__`.

### Include guards

```c
#ifndef MYHEADER_H
#define MYHEADER_H
...
#endif
```

`#pragma once` is shorter and supported by every real compiler, but is not ISO C.

### X-macros

A genuinely powerful technique: define a list **once**, expand it several ways.

```c
#define COLOR_LIST      \
    X(RED,   0xFF0000)  \
    X(GREEN, 0x00FF00)  \
    X(BLUE,  0x0000FF)

typedef enum {
#define X(name, val) COLOR_##name,
    COLOR_LIST
#undef X
    COLOR_COUNT
} Color;

static const char *COLOR_NAMES[] = {
#define X(name, val) #name,
    COLOR_LIST
#undef X
};
```

The enum and the name table can never drift apart, because there is only one list.

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_text_io.c` | Line reading, the `feof` bug, buffering, a word-frequency counter |
| `practice/02_binary_io.c` | Records, endianness, portable vs non-portable serialisation |
| `practice/03_csv_parser.c` | A real CSV reader: quoted fields, embedded commas, empty fields |
| `practice/04_preprocessor.c` | Every macro pitfall, `do/while(0)`, stringify/paste, assertions |
| `practice/05_xmacros.c` | X-macros generating an enum, a name table, a parser, and a serialiser |

---

## Checklist

- [ ] You check every `fopen` for NULL.
- [ ] You never write `while (!feof(f))`.
- [ ] You store `fgetc`'s result in an `int`.
- [ ] You open binary files with `"rb"`/`"wb"`.
- [ ] You parenthesise every macro parameter and the whole body.
- [ ] You wrap multi-statement macros in `do { } while (0)`.
- [ ] You reach for `static inline` before a function-like macro.
