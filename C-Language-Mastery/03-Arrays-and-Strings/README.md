# Module 03 — Arrays and Strings

An array in C is a contiguous block of objects with **no length attached to it**. A string
is an array of `char` that happens to end in a zero byte. Both of these facts cause more
security vulnerabilities than any other feature of the language, so this module is really
about how to work with them without shooting yourself.

---

## 1. Arrays

```c
int a[5];                    /* 5 ints, UNINITIALISED (garbage) */
int b[5] = {1, 2, 3, 4, 5};
int c[5] = {1, 2};           /* remaining elements are ZERO */
int d[5] = {0};              /* the idiom for "all zeros" */
int e[]  = {1, 2, 3};        /* size deduced: 3 */
int f[5] = {[4] = 9, [0] = 1};   /* designated initialisers, C99 */
```

- Indices run `0` to `n-1`. `a[5]` on `int a[5]` is **out of bounds**.
- **There is no bounds checking.** Ever. Reading or writing past the end is undefined
  behaviour: it may silently corrupt an adjacent variable, crash, or appear to work.
- The size must be a compile-time constant, unless you use a VLA (see below).

### Getting the length

```c
size_t n = sizeof arr / sizeof arr[0];
```

This works **only where the array itself is in scope**. The moment you pass it to a
function it decays to a pointer and `sizeof` gives you the pointer size (8), not the array
size. This is the single most common beginner bug:

```c
void f(int arr[]) {
    size_t n = sizeof arr / sizeof arr[0];   /* WRONG: 8/4 = 2, always */
}
void g(int *arr, size_t n) { ... }           /* correct: pass the length */
```

**Rule: a pointer parameter and its length always travel together.**

### Array decay

In almost every expression, an array name converts to a pointer to its first element:

```c
int a[5];
int *p = a;          /* same as &a[0] */
a[i]  ==  *(a + i)  ==  *(i + a)  ==  i[a]     /* all identical; the last is legal and cursed */
```

The three exceptions where an array does **not** decay:

1. `sizeof a` — gives the whole array's size.
2. `&a` — has type `int (*)[5]`, a pointer to the whole array, not to its first element.
3. A string literal initialising a `char` array — `char s[] = "hi"` copies, it does not point.

### Multidimensional arrays

```c
int grid[3][4];              /* 3 rows of 4 — ONE contiguous block of 12 ints */
grid[1][2] = 7;              /* == *(*(grid + 1) + 2) */
```

`grid` is stored **row-major**: `grid[0][0..3]`, then `grid[1][0..3]`, and so on. This
matters enormously for performance (module 13): iterating rows-then-columns is
cache-friendly; the reverse can be 10× slower.

When passing a 2D array, all dimensions except the first must be known:

```c
void f(int g[][4], int rows);        /* the 4 is required */
void g(int rows, int cols, int m[rows][cols]);   /* C99 VLA parameter — cleanest */
void h(int *flat, int rows, int cols);           /* manual: flat[r*cols + c] */
```

### Variable-length arrays (VLAs)

```c
void f(int n) { int a[n]; }      /* C99; OPTIONAL since C11 */
```

VLAs live on the stack, and their size is under the caller's control — so a large `n` is
a stack overflow, i.e. a remotely triggerable crash. MSVC does not support them at all,
and the Linux kernel banned them outright. **Use `malloc` instead.** Know what they are so
you recognise them; do not write them.

---

## 2. Strings

A C string is a `char` array terminated by `'\0'` (a byte with value zero). The terminator
is part of the data and takes a byte.

```c
char s[] = "hello";        /* 6 bytes: 'h','e','l','l','o','\0'  — modifiable */
const char *p = "hello";   /* pointer to a string LITERAL — read-only */
```

The difference is critical:

| | `char s[] = "hi"` | `const char *p = "hi"` |
|---|---|---|
| What it is | a modifiable array, initialised by copying | a pointer to static read-only storage |
| `sizeof` | 3 (the array) | 8 (the pointer) |
| `s[0] = 'H'` | fine | **undefined behaviour**, usually SIGSEGV |
| Lifetime | its enclosing scope | whole program |

Always write `const char *` for pointers to literals. The compiler will then stop you.

### `<string.h>` — the essential functions

| Function | Does | Danger |
|---|---|---|
| `strlen(s)` | length **excluding** the NUL | O(n) — never call it in a loop condition |
| `strcpy(d, s)` | copy including NUL | **no bound — buffer overflow** |
| `strncpy(d, s, n)` | copy at most n bytes | **may not NUL-terminate**; pads with zeros |
| `strcat(d, s)` | append | **no bound**; also O(len(d)) each call |
| `strncat(d, s, n)` | append at most n **plus a NUL** | `n` means something different from `strncpy`'s |
| `strcmp(a, b)` | <0, 0, >0 | returns **0 for equal** — easy to invert |
| `strncmp(a, b, n)` | compare first n | |
| `strchr(s, c)` / `strrchr` | first/last occurrence, or NULL | |
| `strstr(h, n)` | find substring, or NULL | |
| `strtok(s, delim)` | tokenise | **modifies the input**, uses global state, not thread-safe |
| `strdup(s)` | malloc + copy | POSIX, not ISO C; **caller must free** |
| `memcpy(d, s, n)` | copy n bytes | regions must **not overlap** — UB if they do |
| `memmove(d, s, n)` | copy n bytes | overlap-safe; use when unsure |
| `memset(p, c, n)` | fill n bytes with byte c | `memset(a, 1, n)` sets bytes, not ints |
| `memcmp(a, b, n)` | compare n bytes | on structs it compares **padding** too — unreliable |

### `strncpy` is not a safe `strcpy`

```c
char dst[8];
strncpy(dst, "abcdefghij", sizeof dst);   /* copies 8 bytes, NO terminator */
printf("%s", dst);                        /* reads past the end — UB */
```

`strncpy` was designed for fixed-width records in 1970s file formats, not for safety.
If you use it, terminate manually:

```c
strncpy(dst, src, sizeof dst - 1);
dst[sizeof dst - 1] = '\0';
```

**Better: use `snprintf`, which always terminates and tells you if it truncated.**

```c
int n = snprintf(dst, sizeof dst, "%s", src);
if (n < 0 || (size_t)n >= sizeof dst) { /* truncated */ }
```

`snprintf` returns *the length it would have written*, not the length written — that is
how you detect truncation. This is the single most useful safety idiom in C string work.

### Safe concatenation

Repeated `strcat` is both unsafe and quadratic (each call rescans the destination).
Track the offset yourself:

```c
char buf[128];
size_t off = 0;
for (int i = 0; i < n; i++) {
    int w = snprintf(buf + off, sizeof buf - off, "%d ", values[i]);
    if (w < 0 || (size_t)w >= sizeof buf - off) break;   /* full */
    off += (size_t)w;
}
```

---

## 3. Character classification — `<ctype.h>`

`isalpha`, `isdigit`, `isspace`, `isupper`, `tolower`, `toupper`, …

**All of them require their argument to be representable as `unsigned char` or be `EOF`.**
Passing a plain `char` that happens to be negative is undefined behaviour:

```c
isalpha(c)                  /* WRONG if char is signed and c > 127 */
isalpha((unsigned char)c)   /* correct */
```

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_arrays.c` | Initialisation, `sizeof` idiom, decay, 2D layout, row-major order, VLA hazards |
| `practice/02_strings.c` | Literals vs arrays, every important `<string.h>` function, `strtok` behaviour |
| `practice/03_safe_strings.c` | Overflow demonstrated and fixed; a bounded string-builder; a safe `strlcpy` |
| `practice/04_string_algorithms.c` | Reverse, palindrome, word count, tokenising without `strtok`, atoi/itoa by hand |

---

## Checklist

- [ ] You never write `sizeof arr` inside a function that received `arr` as a parameter.
- [ ] Every buffer pointer you pass is accompanied by its size.
- [ ] You use `snprintf` and check its return value for truncation.
- [ ] You know `strncpy` may leave the destination unterminated.
- [ ] You cast to `unsigned char` before calling anything in `<ctype.h>`.
- [ ] You can explain why `char *p = "hi"; p[0] = 'H';` is undefined behaviour.
