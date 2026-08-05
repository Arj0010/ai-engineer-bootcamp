# Module 06 — Structs, Unions, and Enums

How to build your own types. Also: why `sizeof` a struct is often bigger than the sum of
its members, and what to do about it.

---

## 1. Structs

```c
struct Point { int x, y; };            /* a new type: `struct Point` */
typedef struct { int x, y; } Point;    /* an anonymous struct + an alias */

/* The form to prefer — usable as both `struct Point` and `Point`,
   and self-referential, which the anonymous form cannot be. */
typedef struct Point { int x, y; } Point;
```

Initialisation:

```c
Point a = {1, 2};                  /* positional */
Point b = {.y = 2, .x = 1};        /* DESIGNATED (C99) — order-independent, clearer */
Point c = {0};                     /* everything zeroed */
Point d = a;                       /* structs ARE assignable and copyable */
```

**Prefer designated initialisers.** They survive field reordering and make missing fields
obvious. Any member you omit is zero-initialised.

Structs, unlike arrays, can be **assigned**, **passed by value**, and **returned**. That is
the standard trick for making a fixed array copyable: wrap it in a struct.

### Access

```c
Point p;  Point *pp = &p;
p.x       /* member of a struct */
pp->x     /* member through a pointer — shorthand for (*pp).x */
```

---

## 2. Padding and alignment — why `sizeof` surprises you

Every type has an **alignment requirement**: its address must be a multiple of that number.
On x86-64, an `int` wants a 4-byte boundary, a `double` and a pointer want 8.

The compiler inserts **padding** between members to satisfy this, and **tail padding** at
the end so that an array of the struct keeps every element aligned.

```c
struct Bad  { char a; int b; char c; };   /* 12 bytes: 1 + 3pad + 4 + 1 + 3pad */
struct Good { int b; char a; char c; };   /*  8 bytes: 4 + 1 + 1 + 2pad       */
```

**The rule: declare members in decreasing order of size.** It costs nothing and it can
shrink a struct by 30–50%, which on a million-element array is a large amount of cache.

Tools:
- `offsetof(struct S, member)` from `<stddef.h>` — where a member actually sits.
- `_Alignof(T)` (C11) — a type's alignment requirement.
- `_Alignas(N)` — force a stricter alignment (e.g. 64 for a cache line).
- `#pragma pack(1)` / `__attribute__((packed))` — remove padding entirely. **Only** for
  matching an external binary format, and be aware that unaligned member access is slow on
  x86 and a hard fault on some ARM configurations.

**Never `memcmp` two structs to compare them.** Padding bytes are uninitialised, so two
structs with identical members can compare unequal. Compare field by field.

---

## 3. Unions

All members share the same memory; the union is as big as its largest member.

```c
union Value { int i; float f; char bytes[4]; };
```

Writing one member and reading another is **type punning**. In C (unlike C++) this is
legal and well-defined, which makes unions the standard tool for inspecting bit patterns.

A bare union has no way of knowing which member is currently valid. The fix is a
**tagged union** (a "sum type" / "discriminated union"), which is one of the most useful
patterns in C:

```c
typedef enum { V_INT, V_FLOAT, V_STRING } ValueKind;

typedef struct {
    ValueKind kind;                  /* the TAG — says which member is live */
    union {
        long   i;
        double f;
        char  *s;
    } as;
} Value;
```

Always read through a `switch` on the tag. This is how every interpreter represents
dynamically typed values, and how ASTs represent nodes.

---

## 4. Enums

```c
enum Color { RED, GREEN, BLUE };            /* 0, 1, 2 */
enum Status { OK = 0, WARN = 10, ERR };     /* 0, 10, 11 — ERR continues from WARN */
```

- Enum constants are plain `int`s in scope at file level — they can collide, so prefix
  them (`COLOR_RED`).
- The enum *type* may be any integer type the compiler chooses. Do not rely on its size.
- There is no range checking: `(enum Color)99` is legal and produces 99.
- `enum { MAX = 100 };` is the idiomatic way to get a typed compile-time constant usable
  as an array size — better than `#define` because it is scoped and visible to the debugger.

The `X_COUNT` idiom keeps a table and an enum in sync:

```c
typedef enum { OP_ADD, OP_SUB, OP_MUL, OP_COUNT } Op;
static const char *OP_NAMES[OP_COUNT] = { "add", "sub", "mul" };
```

Add an enumerator without adding a name and the compiler complains about the array size.

---

## 5. Bitfields

```c
struct Flags {
    unsigned int visible  : 1;
    unsigned int selected : 1;
    unsigned int layer    : 4;      /* 0..15 */
};
```

Compact, and readable. But: **bit ordering within the storage unit is
implementation-defined**, so bitfields are *not* portable for describing wire formats or
hardware registers, however tempting that looks. For those, use explicit shifts and masks
(module 11). Bitfields are fine for internal, in-memory flags.

---

## 6. Opaque types — encapsulation in C

Put an incomplete type in the header and the definition in the `.c` file:

```c
/* stack.h */
typedef struct Stack Stack;          /* declared, not defined */
Stack *stack_create(size_t capacity);
void   stack_destroy(Stack *s);
int    stack_push(Stack *s, int v);
```

Callers can hold `Stack *` but cannot see or touch the fields, cannot allocate one on the
stack, and cannot depend on its size. You can change the representation entirely without
recompiling any caller. This is C's version of a private class, and it costs nothing.

The trade-off is that the object must be heap-allocated (the caller does not know its size).

---

## 7. Flexible array members

```c
typedef struct {
    size_t len;
    char   data[];       /* must be LAST; sizeof(Buffer) does not count it */
} Buffer;

Buffer *b = malloc(sizeof *b + n);   /* header and payload in ONE allocation */
b->len = n;
```

One allocation instead of two, one free, no extra pointer to chase, and the data shares a
cache line with the header. Used in module 05's refcounted buffer.

---

## Practice

| File | What it shows |
|---|---|
| `practice/01_structs.c` | Declaration forms, designated initialisers, copying, nesting, arrays of structs |
| `practice/02_padding.c` | `offsetof` and `_Alignof` on real structs; reordering that halves the size; packed structs; why `memcmp` lies |
| `practice/03_unions_and_enums.c` | Type punning, a tagged union evaluator, enum idioms, bitfields |
| `practice/04_opaque_type/` | A complete opaque `Stack` — header, implementation, client, and a `Makefile` |

---

## Checklist

- [ ] You use designated initialisers.
- [ ] You order members largest-first without thinking about it.
- [ ] You know why `sizeof(struct{char;int;char;})` is 12, not 6.
- [ ] You never `memcmp` structs.
- [ ] You reach for a tagged union rather than a bare one.
- [ ] You know how to hide a struct's fields behind an opaque pointer.
