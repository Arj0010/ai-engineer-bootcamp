# Module 11 — Advanced C

The parts of the language you can write real programs without knowing — until the day you
cannot. Bit manipulation, C11 generics, varargs, non-local jumps, and a proper treatment of
undefined behaviour.

---

## 1. Bit manipulation

| Idiom | Meaning |
|---|---|
| `x & (1u << n)` | test bit n |
| `x \|= (1u << n)` | set bit n |
| `x &= ~(1u << n)` | clear bit n |
| `x ^= (1u << n)` | toggle bit n |
| `x & (x - 1)` | clear the lowest set bit |
| `x & -x` | **isolate** the lowest set bit |
| `x \| (x - 1)` | set all bits below the lowest set bit |
| `(x & (x - 1)) == 0` | is x a power of two? (plus `x != 0`) |
| `x >> 1` vs `x / 2` | identical for unsigned; the compiler already does this |

**Always use unsigned types for bit work.** `1 << 31` on a signed 32-bit `int` is undefined
behaviour; `1u << 31` is fine. Right-shifting a negative signed value is
implementation-defined.

GCC/Clang builtins worth knowing — they compile to single instructions:
`__builtin_popcount` (count set bits), `__builtin_clz` (count leading zeros),
`__builtin_ctz` (count trailing zeros), `__builtin_bswap32`.
C23 standardises these as `<stdbit.h>`.

---

## 2. `_Generic` — compile-time type dispatch (C11)

```c
#define type_name(x) _Generic((x),        \
    int:    "int",                        \
    double: "double",                     \
    char *: "char *",                     \
    default: "unknown")
```

`_Generic` selects an expression based on the *static type* of a controlling expression.
Only the selected branch is compiled, so the others need not even be valid for that type.

This gives you real, type-safe, zero-cost overloading — it is how `<tgmath.h>` makes
`sqrt(x)` call `sqrtf`, `sqrt`, or `sqrtl` depending on the argument.

Gotchas: an array argument decays to a pointer before matching; `const` qualifiers must be
listed separately; and string literals have type `char[N]`, not `char *`.

---

## 3. Variadic functions

Covered in module 02; the additional points here are the *default argument promotions*
(`char`/`short` → `int`, `float` → `double`, so `va_arg(ap, char)` is always wrong) and how
to forward a `va_list` (`vprintf`, not `printf`).

---

## 4. `setjmp` / `longjmp` — non-local jumps

C's only mechanism resembling exceptions.

```c
jmp_buf env;
if (setjmp(env) == 0) {   /* first time through: saves the context */
    risky();              /* somewhere inside, longjmp(env, 1) */
} else {                  /* arrived here via longjmp */
    handle_error();
}
```

Real constraints, all of which matter:
- The function containing `setjmp` must **not have returned** when `longjmp` runs.
- Local variables not declared `volatile` have **indeterminate** values after a `longjmp`.
- **Nothing is unwound.** Open files stay open; allocations leak. There are no destructors.
- `setjmp` may only appear in a handful of contexts (as a whole controlling expression, or
  compared against an integer constant).

Use it for deep parser error recovery or interpreter-level exceptions. Not for control flow.

---

## 5. `volatile`, `restrict`, and atomics

**`volatile`** means "this object may change in ways the compiler cannot see" — a
memory-mapped hardware register, or a variable written by a signal handler
(`volatile sig_atomic_t`). It forces every read and write to actually happen.
**It is not a threading primitive** and provides no atomicity or ordering.

**`restrict`** is a promise that, for the lifetime of the pointer, the object it points to
is accessed *only* through it. That lets the compiler keep values in registers across
writes. `memcpy` is declared with `restrict`; `memmove` is not — that is precisely the
difference between them. **Breaking the promise is undefined behaviour with no diagnostic.**

**`<stdatomic.h>`** (C11) is the actual threading tool: `atomic_int`, `atomic_fetch_add`,
memory orders. Module 12 covers it.

---

## 6. Undefined behaviour — the part that bites experienced people

UB is not "implementation-defined" or "it crashes". It means the standard imposes **no
requirements at all**, and optimisers exploit that:

```c
int f(int x) {
    if (x + 1 < x) return -1;   /* signed overflow is UB, so the compiler */
    return x + 1;               /* assumes it cannot happen and DELETES the check */
}
```

The most common sources:

| UB | Notice |
|---|---|
| Signed integer overflow | The compiler assumes it never happens. |
| Out-of-bounds access | Including computing `a + n + 1`. |
| Dereferencing NULL or a freed pointer | |
| `x << n` with `n >= width` or `n < 0` | `1 << 31` on a signed int included. |
| Strict aliasing violation | `*(int *)&some_float`. |
| Modifying an object twice without a sequence point | `i = i++`. |
| Falling off the end of a non-`void` function | |
| Reading an uninitialised variable | |
| Modifying a string literal | |
| `memcpy` with overlapping regions | |

Defences: `-Wall -Wextra`, `-fsanitize=undefined`, and — when you genuinely need wrapping —
`-fwrapv` to make signed overflow defined.

---

## Practice

| File | Contents |
|---|---|
| `practice/01_bit_manipulation.c` | Every bit idiom, a bitset, popcount methods compared, bit-level tricks |
| `practice/02_generic.c` | `_Generic` dispatch, a type-safe max, a generic print, `<tgmath.h>`-style overloading |
| `practice/03_setjmp_and_errors.c` | Non-local jumps, a try/catch macro layer, and the resource-leak problem |
| `practice/04_qualifiers.c` | `volatile` demonstrated with optimisation on, `restrict` and its measured effect |
| `practice/broken/05_undefined_behaviour.c` | Ten UB constructs, each with the compiler's actual reaction |

---

## Checklist

- [ ] You use unsigned types for anything bitwise.
- [ ] You know `x & (x-1)` clears the lowest set bit and why that gives an O(popcount) loop.
- [ ] You can write a `_Generic` dispatch macro.
- [ ] You know `volatile` is not a threading primitive.
- [ ] You know why `memcpy` has `restrict` and `memmove` does not.
- [ ] You can name five sources of undefined behaviour without looking.
