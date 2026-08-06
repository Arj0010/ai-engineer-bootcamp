# C Cheat Sheet

Everything worth having on one page. Written for looking up, not for reading through.

---

## Compile

```bash
gcc -std=c17 -Wall -Wextra -Wpedantic -g -O2 prog.c -o prog -lm
gcc -std=c17 -Wall -Wextra -g -O0 -fsanitize=address,undefined prog.c -o prog   # debug
valgrind --leak-check=full ./prog
gdb ./prog                     # run, bt, p x, break main, next, step, finish
```

"undefined reference to X" is a **linker** error: declared but never defined, or you forgot
to compile/link the file that defines it. It is never a syntax problem.

---

## Types

| Need | Use |
|---|---|
| loop counter, small arithmetic | `int` |
| a size, a count, an array index | `size_t` (print with `%zu`) |
| difference of two indices/pointers | `ptrdiff_t` (`%td`) |
| exact width (files, protocols, hardware) | `int32_t`, `uint64_t` (`<stdint.h>`) |
| a real number | `double` |
| true/false | `bool` (`<stdbool.h>`) |

- `sizeof(char) == 1` always; nothing else has a guaranteed size.
- **Signed overflow is undefined behaviour. Unsigned wraps.**
- Mixing signed and unsigned converts the signed operand to unsigned: `-1 < 1u` is **false**.
- Plain `char` may be signed or unsigned — cast to `unsigned char` for `<ctype.h>`.

---

## printf / scanf

| Spec | Type | | Spec | Type |
|---|---|---|---|---|
| `%d` | `int` | | `%zu` | `size_t` |
| `%u` | `unsigned` | | `%td` | `ptrdiff_t` |
| `%ld` `%lld` | `long` `long long` | | `%f` `%g` `%e` | `double` |
| `%c` | char (as `int`) | | `%s` | `char *` |
| `%p` | `void *` (cast it) | | `%x` `%#x` | hex |

A mismatched specifier is **undefined behaviour**, not a formatting glitch.

Never `scanf("%s", buf)`. Use:
```c
char line[256];
if (fgets(line, sizeof line, stdin)) {
    line[strcspn(line, "\n")] = '\0';
    char *end; long n = strtol(line, &end, 10);
    if (end != line && *end == '\0') { /* valid */ }
}
```

---

## Pointers

```c
int *p = &x;      /* & = address of */
*p = 5;           /* * = the thing it points at */
p + 1             /* advances sizeof(*p) BYTES, not 1 */
a[i]  ==  *(a + i)
```

Read declarations right-to-left from the name:
```c
const int *p;      /* pointer to const int: *p read-only, p can move */
int * const p;     /* const pointer to int: p fixed, *p writable */
int (*f)(int);     /* pointer to function */
int *f(int);       /* function returning a pointer */
int (*a)[10];      /* pointer to an array of 10 */
int *a[10];        /* array of 10 pointers */
```

**Arrays decay to pointers** except under `sizeof`, `&`, and string-literal init.
`sizeof arr` inside a function that received `arr` gives the **pointer** size. Always pass
the length.

---

## Memory

```c
T *p = malloc(n * sizeof *p);      /* sizeof *p, NOT sizeof(T); no cast */
if (p == NULL) { /* handle */ }    /* every time */

T *tmp = realloc(p, n2 * sizeof *tmp);   /* NEVER p = realloc(p, ...) */
if (tmp == NULL) { /* p is still valid */ }
p = tmp;

free(p); p = NULL;                 /* free(NULL) is a defined no-op */
```

- `calloc(n, size)` zeroes **and** checks `n * size` for overflow. Use it when the count
  comes from outside your program.
- Grow arrays by **doubling**, never by one.
- `realloc` may **move** the block — every pointer into it dangles.
- One owner per allocation. Document who frees it, in the header.

### The cleanup pattern

```c
int f(void) {
    int rc = -1;
    T *a = NULL; FILE *fp = NULL;

    a = malloc(...);  if (!a)  goto out;
    fp = fopen(...);  if (!fp) goto out;
    ...
    rc = 0;
out:
    if (fp) fclose(fp);
    free(a);
    return rc;
}
```

---

## Strings

```c
strlen(s)                    /* O(n) — never in a loop condition */
snprintf(d, sizeof d, ...)   /* ALWAYS terminates; returns the length it WANTED */
strcmp(a, b) == 0            /* equal */
strcspn(s, "\n")             /* index of the newline */
memmove                      /* when regions may overlap; memcpy when they cannot */
```

- `strcpy`/`strcat`/`sprintf` are unbounded. Do not use them.
- `strncpy` may **not NUL-terminate**. It is not a safe `strcpy`.
- `char s[] = "hi"` is a modifiable copy; `const char *p = "hi"` is read-only.

---

## Structs

```c
typedef struct Point { int x, y; } Point;    /* tagged AND typedef'd: use this form */
Point p = {.y = 2, .x = 1};                  /* designated initialisers */
```

- **Declare members largest-first.** Padding can cost 30–50% otherwise.
- **Never `memcmp` two structs** — padding bytes are uninitialised.
- Structs can be assigned, passed by value, and returned. Arrays cannot.
- `=` is a **shallow** copy: pointer members are shared, not duplicated.

---

## Bit manipulation

```c
x |=  (1u << n)          /* set    */    x & (x - 1)      /* clear lowest set bit */
x &= ~(1u << n)          /* clear  */    x & -x           /* isolate lowest set bit */
x ^=  (1u << n)          /* toggle */    (x & (x-1)) == 0 /* power of two (and x != 0) */
x &   (1u << n)          /* test   */    __builtin_popcount(x)
```

Use **unsigned** types. `1 << 31` on a signed int is undefined; `1u << 31` is fine.
Parenthesise: `&` binds looser than `==`.

---

## Complexity

| | Access | Search | Insert | Delete |
|---|---|---|---|---|
| Dynamic array | O(1) | O(n) | O(1)* | O(n) |
| Linked list | O(n) | O(n) | O(1)† | O(1)† |
| Hash table | — | O(1)avg | O(1)avg | O(1)avg |
| Balanced tree | — | O(log n) | O(log n) | O(log n) |
| Heap | O(1) min | O(n) | O(log n) | O(log n) |
| Trie | — | O(k) | O(k) | O(k) |

\* amortised  † given the node

**Sorting:** quicksort (fast, O(n²) worst), merge (stable, O(n) space), heap (guaranteed,
in place), insertion (best under ~30 elements), counting/radix (beat O(n log n) for
integers).

---

## Undefined behaviour — the top ten

1. Signed integer overflow
2. Out-of-bounds array access (including computing `a + n + 1`)
3. Dereferencing NULL or a freed pointer
4. `x << n` with `n >= width` or `n < 0`
5. Strict aliasing violation (`*(int *)&some_float`) — use `memcpy`
6. Modifying an object twice without a sequence point (`i = i++`)
7. Falling off the end of a non-`void` function
8. Reading an uninitialised variable
9. Modifying a string literal
10. `memcpy` with overlapping regions

Defences: `-Wall -Wextra`, `-fsanitize=undefined,address`, valgrind.

---

## Threads

```c
pthread_create(&t, NULL, fn, arg);  pthread_join(t, &result);
pthread_mutex_lock/unlock
while (!condition) pthread_cond_wait(&cv, &mtx);    /* WHILE, never IF */
atomic_fetch_add(&counter, 1);
```

- A **data race is undefined behaviour**, not an occasional wrong answer.
- `volatile` is **not** a threading primitive — no atomicity, no ordering.
- Pad per-thread data to 64 bytes to avoid **false sharing**.
- Prefer not sharing at all: per-thread accumulators, combine at the end.

---

## Makefile

```makefile
CC     := gcc
CFLAGS := -std=c17 -Wall -Wextra -g -O2
SRCS   := $(wildcard *.c)
OBJS   := $(SRCS:.c=.o)

prog: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@    # -MMD: automatic header deps

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	rm -f prog $(OBJS) $(OBJS:.o=.d)
```

Recipes must be indented with a **TAB**. `$@` = target, `$<` = first prerequisite,
`$^` = all prerequisites. Put `-l` flags **after** the source files.
