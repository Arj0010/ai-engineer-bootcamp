# Module 01 — Fundamentals

Types, how numbers are actually stored, operators, and I/O. This is the layer where most
"weird C behaviour" originates, so it is worth more care than it usually gets.

---

## 1. The type system is a description of memory

A type in C answers two questions: **how many bytes**, and **how do I interpret them**.
That is all. There is no runtime type information; the compiler bakes the interpretation
into the instructions and then forgets.

### Integer types

| Type | Guaranteed minimum | Typical on 64-bit Linux | Range (typical) |
|---|---|---|---|
| `char` | 1 byte, ≥8 bits | 1 byte | −128…127 *or* 0…255 — **signedness is implementation-defined** |
| `signed char` | 1 byte | 1 byte | −128…127 |
| `unsigned char` | 1 byte | 1 byte | 0…255 |
| `short` | ≥16 bits | 2 bytes | −32768…32767 |
| `int` | ≥16 bits | 4 bytes | −2147483648…2147483647 |
| `long` | ≥32 bits | 8 bytes (4 on Windows!) | ±9.2×10¹⁸ |
| `long long` | ≥64 bits | 8 bytes | ±9.2×10¹⁸ |

**The standard guarantees ranges, not sizes.** `sizeof(int)` is 4 on every machine you are
likely to meet and 2 on some embedded targets. The only guarantee is
`sizeof(char) == 1` and `char ≤ short ≤ int ≤ long ≤ long long`.

When the exact width matters — file formats, network protocols, hardware registers — use
`<stdint.h>`:

```c
#include <stdint.h>
int8_t  int16_t  int32_t  int64_t      /* exactly this many bits */
uint8_t uint16_t uint32_t uint64_t     /* unsigned, exact */
intptr_t uintptr_t                     /* big enough to hold a pointer */
size_t                                 /* from <stddef.h>: result of sizeof, unsigned */
ptrdiff_t                              /* difference of two pointers, signed */
```

Rule of thumb: **`int` for loop counters and small arithmetic, `size_t` for sizes and
indices, fixed-width types for anything that touches the outside world.**

### Floating point

| Type | Size | Precision | Notes |
|---|---|---|---|
| `float` | 4 bytes | ~7 decimal digits | IEEE-754 binary32 |
| `double` | 8 bytes | ~15–16 digits | IEEE-754 binary64. **The default — use it.** |
| `long double` | 10/16 bytes | 18+ digits | x86-only 80-bit; not portable |

`3.14` is a `double`. `3.14f` is a `float`. Module 15 covers why `0.1 + 0.2 != 0.3`.

### Other

- `void` — "no type". Used for functions that return nothing, parameter lists that take
  nothing, and `void *` (a pointer to unknown type).
- `_Bool` / `bool` — with `#include <stdbool.h>` you get `bool`, `true`, `false`.
  Any non-zero value assigned to a `bool` becomes exactly `1`.

---

## 2. How integers are actually stored

### Two's complement

Signed integers use two's complement: to negate, flip every bit and add 1.

```
  5  as int8_t  =  0000 0101
 -5  as int8_t  =  1111 1011      (flip -> 1111 1010, +1 -> 1111 1011)
```

Consequences you must internalise:

- The top bit is the sign bit. `1000 0000` is −128, and there is **no +128**, so the range
  is asymmetric: `INT_MIN` has no positive counterpart. `-INT_MIN` is undefined behaviour.
- `x >> 1` on a *negative* signed value is implementation-defined (in practice arithmetic
  shift, preserving sign). Shifting unsigned values is always well-defined.
- Bit patterns are unchanged when you cast between signed and unsigned of the same width;
  only the *interpretation* changes.

### Overflow: the single most important distinction

```c
int      a = INT_MAX;  a + 1;   /* UNDEFINED BEHAVIOUR — anything may happen */
unsigned b = UINT_MAX; b + 1;   /* well-defined: wraps to 0 */
```

Signed overflow being UB is not pedantry. The compiler *uses* it: it assumes `i + 1 > i`
is always true, and will delete your overflow check. This is why `-fsanitize=undefined`
exists, and why you should never write an overflow test as `if (a + b < a)` for signed types.

Correct signed-overflow check:

```c
if (a > 0 && b > INT_MAX - a) { /* would overflow */ }
```

Or use the GCC/Clang builtins: `__builtin_add_overflow(a, b, &result)`.

---

## 3. Conversion rules (where the bugs live)

### Integer promotion

Anything smaller than `int` (`char`, `short`, `_Bool`, bitfields) is promoted to `int`
before arithmetic. So:

```c
char a = 100, b = 100;
char c = a + b;     /* a+b computed as int = 200, then truncated to char = -56 */
```

### Usual arithmetic conversions

When operands have different types, they are converted to a common type:

1. If either is `long double`/`double`/`float`, convert the other to it.
2. Otherwise promote both to at least `int`.
3. If both are signed or both unsigned, the smaller rank converts to the larger.
4. **If one is unsigned and its rank ≥ the signed one's, the signed operand becomes
   unsigned.** This is the killer.

```c
int i = -1;
unsigned u = 1;
if (i < u)                 /* FALSE! -1 becomes 4294967295, which is > 1 */
    puts("never printed");
```

The most common real form:

```c
for (int i = 0; i < strlen(s) - 1; i++)   /* strlen returns size_t (unsigned) */
```
If `s` is empty, `strlen(s) - 1` is `SIZE_MAX`, and the loop runs 18 quintillion times.

**Defence:** compile with `-Wsign-compare` (included in `-Wextra`), and pick one
signedness per comparison deliberately.

---

## 4. Operators

### Precedence, in the order you actually need it

```
()  []  ->  .  ++(post) --(post)          highest
!  ~  ++(pre) --(pre)  +/-(unary)  *  &  sizeof  (cast)
*  /  %
+  -
<<  >>
<  <=  >  >=
==  !=
&
^
|
&&
||
?:
=  +=  -=  *=  /=  %=  &=  ^=  |=  <<=  >>=
,                                          lowest
```

The two traps that bite everyone:

```c
if (a & b == c)      /* parses as a & (b == c)   — == binds tighter than & */
if (x << 1 + 2)      /* parses as x << (1 + 2)   — + binds tighter than << */
```
**Just use parentheses around bitwise operators. Always.**

### `%` is remainder, not modulo

`-7 % 3 == -1` in C (the result takes the sign of the dividend), not `2`.
For a true positive modulo: `((a % n) + n) % n`.

### Short-circuit evaluation

`&&` and `||` evaluate left to right and stop as soon as the answer is known. This is
guaranteed, and it is the standard idiom for guarding:

```c
if (p != NULL && p->value > 0)     /* p->value is never touched when p is NULL */
if (i < n && arr[i] == target)     /* arr[i] is never read out of bounds */
```

### Undefined behaviour from multiple modification

```c
i = i++ + ++i;      /* UB — i modified twice without a sequence point */
arr[i] = i++;       /* UB */
printf("%d %d", i++, i++);   /* UB — argument evaluation order is unspecified */
```
Rule: **do not modify the same object twice in one expression, and do not both read
and modify it unless the read is used to compute the new value.**

---

## 5. Input and output

### `printf` conversion specifiers

| Spec | Type | Example |
|---|---|---|
| `%d` / `%i` | `int` | `printf("%d", 42)` |
| `%u` | `unsigned int` | |
| `%ld` / `%lu` | `long` / `unsigned long` | |
| `%lld` / `%llu` | `long long` | |
| `%zu` | `size_t` | `printf("%zu", sizeof(int))` — **use this, not `%d`** |
| `%f` | `double` (also for `float`, which promotes) | `%.2f` for 2 decimals |
| `%e` / `%g` | scientific / shorter of `%f`,`%e` | |
| `%c` | `int` holding a character | |
| `%s` | `char *` — must be NUL-terminated | |
| `%p` | `void *` — cast the argument: `(void *)p` | |
| `%x` / `%o` | hex / octal unsigned | `%#x` prints the `0x` |
| `%%` | a literal `%` | |

Width and precision: `%-10s` left-justify in 10 columns, `%08.3f` zero-pad to 8 wide with
3 decimals, `%*d` takes the width as an extra `int` argument.

**A mismatched specifier is undefined behaviour, not a formatting glitch.** `printf` cannot
see the types of its variadic arguments; it trusts the format string and reads that many
bytes off the stack/registers. `-Wformat` (in `-Wall`) checks literal format strings — one
more reason to keep them literal and never build them at runtime from user input
(that is the *format string vulnerability* class).

### Reading input: why `scanf` is a trap

```c
scanf("%d", &n);
```
- Returns the number of items successfully assigned. **Check it.** On bad input it returns
  0 *and leaves the offending characters in the buffer*, so a naive retry loop spins forever.
- `scanf("%s", buf)` has no bound and will overflow `buf`. `%9s` for a 10-byte buffer.
- Mixing `scanf("%d")` with `fgets` leaves a stray `\n` behind and the `fgets` reads it.

**Preferred pattern — read a whole line, then parse it:**

```c
char line[256];
if (fgets(line, sizeof line, stdin) != NULL) {
    line[strcspn(line, "\n")] = '\0';       /* strip the trailing newline */
    char *end;
    long n = strtol(line, &end, 10);
    if (end == line || *end != '\0') { /* not a valid number */ }
}
```

`practice/03_io_and_input.c` implements this as a reusable `read_int()`.

---

## 6. Constants, `const`, and literals

```c
#define MAX 100          /* preprocessor text substitution — no type, no scope */
const int max = 100;     /* a real, typed, scoped, read-only variable */
enum { MAX = 100 };      /* a compile-time int constant — usable as an array size */
```

Prefer `enum` or `const` over `#define` for numbers; the compiler can then type-check them
and the debugger can see them. `#define` is still right for conditional compilation and
for things that must be a compile-time token.

Literal suffixes: `42u`, `42L`, `42ULL`, `3.14f`, `3.14L`.
A leading `0` means **octal**: `010` is 8, not 10. This has caused real outages.
`0x1F` is hex, `0b1010` is binary (a GNU extension, standard in C23).

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_types_and_sizes.c` | Every type's size, alignment, and range on your machine |
| `practice/02_integer_representation.c` | Two's complement, bit patterns, overflow, wraparound |
| `practice/03_io_and_input.c` | Safe input reading; why `scanf` fails |
| `practice/04_operators.c` | Precedence traps, short-circuit, `%` with negatives |
| `practice/05_conversion_traps.c` | Promotion and the signed/unsigned comparison bug |

```bash
cd 01-Fundamentals
for f in practice/*.c; do gcc -std=c17 -Wall -Wextra -g "$f" -o /tmp/p && /tmp/p; done
```

See `exercises.md` for problems with worked solutions.

---

## Checklist

- [ ] You can say what `size_t` is and why you print it with `%zu`.
- [ ] You know signed overflow is UB and unsigned overflow wraps.
- [ ] You can explain why `-1 < 1u` is false.
- [ ] You never write `scanf("%s", buf)`.
- [ ] You parenthesise bitwise operators without thinking about it.
