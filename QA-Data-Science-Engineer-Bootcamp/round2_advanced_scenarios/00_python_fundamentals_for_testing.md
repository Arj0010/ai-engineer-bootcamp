# 0. Python Fundamentals for Testing

> **Read this FIRST.** Every testing tool is built from five Python
> features. If these feel like magic, the tools will too — and magic
> doesn't stick in memory. Learn these and pytest stops being syntax you
> memorize and starts being code you can reason about.
>
> An interviewer flagged "programming language fundamentals" as a weakness.
> This module is the direct fix.

---

## The five things

| Feature | Where you've already seen it |
|---|---|
| **Decorators** (`@`) | `@pytest.fixture`, `@pytest.mark.parametrize`, `@dg.asset`, `@task` |
| **Context managers** (`with`) | `with pytest.raises(TypeError):` |
| **Generators** (`yield`) | fixture setup/teardown |
| **Dunder methods** (`__call__`) | `FakeClock()` being callable |
| **`*args` / `**kwargs`** | how decorators pass arguments through |

---

# 1. DECORATORS

## Step 1: Functions are objects

A function is a value, like a number or a string. You can assign it, pass
it, and return it.

```python
def greet():
    return "hello"

x = greet          # NO parentheses — assigning the function ITSELF
print(x)           # <function greet at 0x7f...>
print(x())         # "hello"  ← now we CALL it
```

**`greet` = the function. `greet()` = run it, give me the result.**
Everything below depends on that distinction.

## Step 2: Functions can take functions

```python
def run_twice(func):        # func is a FUNCTION coming in
    func()
    func()

def say_hi():
    print("hi")

run_twice(say_hi)           # pass the function, no parentheses
# hi
# hi
```

## Step 3: Functions can return functions

```python
def make_multiplier(n):
    def multiply(x):        # a function defined INSIDE a function
        return x * n        # it remembers n  (this is called a "closure")
    return multiply         # return it — don't call it

double = make_multiplier(2)
triple = make_multiplier(3)
print(double(5))            # 10
print(triple(5))            # 15
```

`make_multiplier(2)` handed back a brand-new function that remembers `n=2`.

## Step 4: A decorator wraps a function

Say you want to time any function without editing it:

```python
import time

def timer(func):                          # takes a function
    def wrapper():                        # builds a new function around it
        start = time.time()
        result = func()                   # run the original
        print(f"took {time.time() - start:.2f}s")
        return result
    return wrapper                        # hand back the wrapped version

def slow_task():
    time.sleep(1)
    return "done"

slow_task = timer(slow_task)              # ← wrap it, reassign the name
print(slow_task())
# took 1.00s
# done
```

**That line `slow_task = timer(slow_task)` is the entire concept.**

## Step 5: `@` is shorthand for exactly that

```python
@timer
def slow_task():
    time.sleep(1)
    return "done"
```

is **identical** to:

```python
def slow_task():
    time.sleep(1)
    return "done"

slow_task = timer(slow_task)
```

> ## `@decorator` above a function means `func = decorator(func)`

That's the whole answer. **A decorator is a function that takes a function
and returns a modified version of it. `@` is syntax sugar for reassigning
the name.**

## Step 6: Decorators that take arguments

`@pytest.mark.parametrize("x", [1, 2])` has arguments, so it needs one more
layer of nesting:

```python
def repeat(times):                   # 1. takes the ARGUMENT
    def decorator(func):             # 2. takes the FUNCTION
        def wrapper():               # 3. the replacement function
            for _ in range(times):
                func()
        return wrapper
    return decorator

@repeat(3)
def hello():
    print("hi")

hello()      # prints "hi" three times
```

`@repeat(3)` calls `repeat(3)` → that **returns a decorator** → which then
wraps `hello`. Three layers: **argument → function → wrapper.**

## Step 7: Now the testing decorators aren't magic

```python
@pytest.fixture
def llm_client():
    return LLMClient(...)
```
= `llm_client = pytest.fixture(llm_client)` — pytest **registers** the
function in its fixture registry so it can inject it by name later.

```python
@pytest.mark.parametrize("bad_input", [123, None])
def test_rejects(bad_input): ...
```
= attaches **metadata** to the function. pytest reads it at collection time
and generates one separate test per value.

```python
@dg.asset
def raw_predictions(...): ...
```
= registers the function as a Dagster asset so `materialize()` knows it exists.

```python
@task(5)
def predict(self): ...
```
= the `@repeat(3)` shape — takes an argument (weight), returns a decorator.

**All the same mechanism.**

## Interview answer

> *"A decorator is a function that takes another function and returns a
> modified version of it. The `@` syntax is shorthand — `@timer` above a
> function is the same as writing `func = timer(func)`. It lets you add
> behavior around a function without changing that function's code:
> logging, timing, caching, or in pytest's case registering fixtures and
> attaching metadata like parametrize cases."*

---

# 2. CONTEXT MANAGERS (`with`)

## The problem it solves

You open a file. You must close it — even if an error happens in between.

```python
f = open("data.txt")
data = f.read()          # if this raises, close() never runs → leaked file handle
f.close()
```

The manual fix is ugly:

```python
f = open("data.txt")
try:
    data = f.read()
finally:
    f.close()            # runs no matter what
```

`with` does that for you:

```python
with open("data.txt") as f:
    data = f.read()
# f.close() automatically called here — even if read() raised
```

## How it works: two dunder methods

Any object with `__enter__` and `__exit__` works with `with`:

```python
class MyContext:
    def __enter__(self):
        print("setup")
        return self              # this is what `as x` receives

    def __exit__(self, exc_type, exc_value, traceback):
        print("teardown")        # ALWAYS runs, even on exception
        return False             # False = let exceptions propagate

with MyContext() as ctx:
    print("inside")

# setup
# inside
# teardown
```

The three `__exit__` arguments tell you **what exception happened** (or
`None`, `None`, `None` if none did). Returning `True` from `__exit__`
**swallows** the exception; returning `False` lets it propagate.

## So what does `pytest.raises` actually do?

```python
with pytest.raises(TypeError):
    classify(123)
```

Roughly:
1. `__enter__` — start watching
2. Body runs
3. `__exit__` receives the exception info and decides:

| What happened | Result |
|---|---|
| `TypeError` raised | ✅ matches → **swallow it** (return True) → test passes |
| `ValueError` raised | ❌ wrong type → **don't swallow** → test fails with that error |
| Nothing raised | ❌ → raise `Failed: DID NOT RAISE` → test fails |

**Now you know why `pytest.raises` beats `try/except/pass`** — the "nothing
raised" case is an explicit failure, because `__exit__` checks for it.
`except` has no equivalent hook; if nothing raises, nothing happens.

## Writing your own with `@contextmanager`

There's a shortcut that skips the class:

```python
from contextlib import contextmanager

@contextmanager
def timed_block(name):
    start = time.time()
    yield                                     # ← the `with` body runs HERE
    print(f"{name} took {time.time()-start:.2f}s")

with timed_block("scoring"):
    score_risk(df)
```

Everything before `yield` = setup. Everything after = teardown. Which
brings us to...

---

# 3. GENERATORS AND `yield`

## `return` ends. `yield` pauses.

```python
def normal():
    return 1
    return 2        # never reached — return ENDS the function

def generator():
    yield 1         # pause here, hand back 1
    yield 2         # resume here on next call
    yield 3

g = generator()
print(next(g))      # 1
print(next(g))      # 2
print(next(g))      # 3
```

**`yield` freezes the function mid-execution** and remembers exactly where
it stopped, including all local variables. `next()` resumes it.

```python
for value in generator():   # for-loops call next() until exhausted
    print(value)            # 1, 2, 3
```

## Why generators matter: memory

```python
# Loads ALL 10 million rows into RAM
def read_all(path):
    return open(path).readlines()

# Yields ONE row at a time — constant memory
def read_lazy(path):
    with open(path) as f:
        for line in f:
            yield line
```

For ML pipelines processing huge scan files, this is the difference between
running and getting OOMKilled.

## Now fixture teardown makes sense

```python
@pytest.fixture
def db_connection():
    conn = connect()        # SETUP — runs before the test
    yield conn              # ← test runs here, receives conn
    conn.close()            # TEARDOWN — runs after the test, even if it failed
```

pytest calls `next()` once to get the value (running setup, pausing at
`yield`), gives it to your test, then calls `next()` again afterward to run
the teardown half. **The fixture is a generator; pytest just drives it.**

Compare:
```python
@pytest.fixture
def simple():
    return LLMClient(...)   # return = no teardown possible

@pytest.fixture
def with_cleanup():
    c = LLMClient(...)
    yield c                 # yield = teardown possible
    c.close()
```

---

# 4. DUNDER METHODS (`__call__` and friends)

**Dunder** = "double underscore." These are hooks Python calls for you.

| You write | Python calls |
|---|---|
| `obj()` | `obj.__call__()` |
| `len(obj)` | `obj.__len__()` |
| `obj[3]` | `obj.__getitem__(3)` |
| `obj + x` | `obj.__add__(x)` |
| `print(obj)` | `obj.__str__()` |
| `with obj:` | `obj.__enter__()` / `obj.__exit__()` |
| `for x in obj:` | `obj.__iter__()` |

## `__call__` makes an object behave like a function

```python
class Multiplier:
    def __init__(self, n):
        self.n = n

    def __call__(self, x):        # makes instances CALLABLE
        return x * self.n

double = Multiplier(2)
print(double(5))                  # 10  ← calling an OBJECT like a function
```

## Which is exactly how FakeClock works

```python
class FakeClock:
    def __init__(self, values):
        self._values = iter(values)      # turn the list into an iterator

    def __call__(self):                  # ← this is why FakeClock() is callable
        return next(self._values)        # each call returns the next value

clock = FakeClock([0.0, 5.0])
print(clock())    # 0.0   (first call)
print(clock())    # 5.0   (second call)
```

The code under test does `self._clock()` — it doesn't know or care whether
that's `time.perf_counter` or your fake. **Both are callable, so both work.**
That's the whole trick behind injecting time.

> **Why a class instead of a plain function?** It needs to *remember* which
> value comes next. `__init__` stores state, `__call__` consumes it.

---

# 5. `*args` AND `**kwargs`

## What they mean

```python
def f(*args, **kwargs):
    print(args)      # a TUPLE of positional arguments
    print(kwargs)    # a DICT of keyword arguments

f(1, 2, name="x", age=3)
# (1, 2)
# {'name': 'x', 'age': 3}
```

`*args` = "collect any number of positional args." `**kwargs` = "collect
any number of keyword args."

## Why decorators need them

The `timer` decorator from earlier is broken:

```python
def timer(func):
    def wrapper():             # ← takes NO arguments
        return func()
    return wrapper

@timer
def add(a, b):                 # but add needs two!
    return a + b

add(2, 3)    # TypeError: wrapper() takes 0 positional arguments but 2 were given
```

The fix — accept anything, pass it straight through:

```python
def timer(func):
    def wrapper(*args, **kwargs):        # accept ANY arguments
        start = time.time()
        result = func(*args, **kwargs)   # forward them unchanged
        print(f"took {time.time()-start:.2f}s")
        return result
    return wrapper

@timer
def add(a, b):
    return a + b

add(2, 3)    # works
```

**Note the two different uses of `*`:**
- In the *definition* `def wrapper(*args)` → **collect** args into a tuple
- In the *call* `func(*args)` → **unpack** the tuple back into arguments

## One more: `functools.wraps`

Wrapping a function loses its name and docstring:

```python
@timer
def add(a, b): ...

print(add.__name__)     # "wrapper"  ← wrong! confusing in tracebacks
```

Fix it:

```python
from functools import wraps

def timer(func):
    @wraps(func)                     # copies name, docstring, etc.
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    return wrapper

print(add.__name__)     # "add"  ✓
```

**Mention `functools.wraps` if asked to write a decorator** — it signals
you've written real ones, not just read about them.

---

# PUTTING IT TOGETHER

Every piece of "magic" you've seen, decoded:

```python
@pytest.fixture              # DECORATOR: registers this in pytest's fixture registry
def db():
    conn = connect()
    yield conn               # GENERATOR: pause, give conn to the test
    conn.close()             # resumes here after the test finishes

@pytest.mark.parametrize("x", [1, 2])   # DECORATOR WITH ARGS: attaches metadata
def test_thing(x, db):                   # both injected by NAME
    with pytest.raises(ValueError):      # CONTEXT MANAGER: __exit__ checks the exception
        process(x, db)

clock = FakeClock([0.0, 5.0])            # DUNDER: __call__ makes it callable
```

**Nothing here is magic. It's five Python features you now know.**

---

# CHECK YOURSELF

1. Rewrite `@timer` above `def slow()` without using `@`. What single line replaces it?
2. Why does `wrapper` need `*args, **kwargs`? What breaks without them?
3. What's the difference between `return conn` and `yield conn` in a fixture?
4. `pytest.raises(TypeError)` — the body raises `ValueError`. Walk through
   what `__exit__` receives and why the test fails.
5. Why is `FakeClock` a class with `__call__` instead of a plain function?
6. In `def wrapper(*args)` vs `func(*args)` — what does `*` do in each?

---

# QUICK REFERENCE

| Concept | One-liner |
|---|---|
| **Decorator** | `@d` above a function = `func = d(func)` |
| **Decorator w/ args** | one extra nesting layer: arg → func → wrapper |
| **`with`** | calls `__enter__`, runs body, always calls `__exit__` |
| **`pytest.raises`** | `__exit__` swallows the right exception, fails on wrong/none |
| **`yield` in fixture** | before = setup, after = teardown |
| **Generator** | pauses and resumes; constant memory over big data |
| **`__call__`** | makes an object callable like a function |
| **`*args/**kwargs`** | collect (in def) / unpack (in call) any arguments |
| **`functools.wraps`** | preserves the wrapped function's name & docstring |
