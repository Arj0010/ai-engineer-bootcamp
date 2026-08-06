# 2-Day Roadmap to Round 2

## Your assessment (honest)

| Dimension | Level | Evidence |
|---|---|---|
| **Reasoning / QA instinct** | **Strong** | Derived separation-of-concerns unprompted; found a real silent failure in your own drug-likeness project; correctly prioritized data validation first under time pressure |
| **Conceptual recall** | **Weak-moderate** | Missed `lazy=True`, fake clock, Pushgateway — all taught, none stuck |
| **Statistics** | **Improving** | Was weakest; now has a dedicated module (09) |
| **Python fundamentals** | **Gap** ⚠️ | Interviewer flagged it; couldn't define a decorator |
| **Hands-on testing** | **Zero** ⚠️ | Never written a test in any language |

## The diagnosis

You learned testing as **theory only**. Nothing is anchored to muscle
memory, so recall decays. Worse, the underlying Python features
(decorators, `with`, `yield`, `__call__`) were never learned, so every tool
looks like magic — and **magic doesn't stick**.

**This is not an intelligence gap.** Your reasoning is the hard part to
teach and you already have it. The fix is mechanical: learn the five Python
primitives, then write tests until it's automatic.

## Priority order (reordered after interviewer feedback)

```
1. Python fundamentals   ← unlocks retention of everything else
2. Hands-on test writing ← converts theory into muscle memory
3. Statistics            ← weakest topic, now standalone
4. Mock interview        ← pressure-test under realistic conditions
```

---

# DAY 1 — ~5 hours

## Block A: Python primitives (2h) ⭐ START HERE

**Read:** `00_python_fundamentals_for_testing.md`

Five features, in order:
1. **Decorators** — `@d` = `func = d(func)`
2. **Context managers** — what `with pytest.raises(...)` actually does
3. **Generators / `yield`** — how fixture teardown works
4. **Dunder methods / `__call__`** — why `FakeClock()` is callable
5. **`*args` / `**kwargs`** — how decorators forward arguments

**Do this, don't just read it.** Open a Python REPL and type out every
example. Break them on purpose. Remove `*args` from the wrapper and watch
the `TypeError`.

**Gate:** you can write a working decorator from scratch, no reference.

## Block B: Write your first tests (3h) ⭐ HIGHEST VALUE

```bash
cd ~/projects/ai-engineer-bootcamp/QA-Data-Science-Engineer-Bootcamp/day1_llm_pipeline_testing
pytest -v          # everything fails — that's the starting line
```

You write these yourself; paste them in chat for critique:

| # | Task | Concept |
|---|---|---|
| 1 | 4 unit tests on `classify()` | bare `assert`, naming |
| 2 | 2 tests using `pytest.raises` | proving exceptions happen on purpose |
| 3 | 1 `parametrize` test | data-driven tests, failure isolation |
| 4 | 1 fixture you write yourself | dependency injection by name |
| 5 | 2 `pytest-httpx` mocked tests | mocking the boundary you don't own |
| 6 | 1 test proving the LLM is NEVER called | register no mock → any call errors |

**Watching red turn green is what makes this stick.** No amount of reading
substitutes for it.

**Gate:** `pytest -v` is fully green and you can explain every line you wrote.

---

# DAY 2 — ~5 hours

## Block C: Pandera hands-on (1.5h)

```bash
cd ../day2_data_contracts_ci
```

1. Write `prediction_schema` yourself against `data/good_predictions.csv`
2. Run it on `data/broken_predictions.csv` with `lazy=True`
3. **Print `exc.value.failure_cases`** and read the actual table
4. Find all 6 planted defects
5. Write a test asserting the *specific* columns that failed

**Gate:** explain `SchemaError` vs `SchemaErrors` and which environment
wants which.

## Block D: Statistics (1.5h)

**Read:** `09_statistics_from_zero.md` — then prove the trap to yourself:

```python
import numpy as np
from scipy import stats

rng = np.random.default_rng(42)
for n in [100, 5_000, 80_000]:
    a = rng.normal(0.70, 0.10, n)
    b = rng.normal(0.705, 0.10, n)      # difference is TINY and FIXED
    r = stats.ks_2samp(a, b)
    print(f"n={n:>6}  statistic={r.statistic:.4f}  p={r.pvalue:.5f}")
```

Watch p collapse toward zero while the statistic stays tiny. **Seeing it
happen beats reading about it.**

**Gate:** explain why p = 0.001 with statistic = 0.02 is not actionable.

## Block E: Mock interview (1.5h)

Full round-2 simulation in chat, with pushback:
- Python fundamentals ("what's a decorator?") — the flagged weakness
- Testing strategy on VulnPrioritize scenarios
- **Your drug-likeness project reframed in QA terms**
- Statistics judgment calls

## Block F: Final review (30 min)

- `FINAL-REVISION-CHEATSHEET.md`
- The "say it out loud" section of each round-2 module
- Your honesty positioning line

---

# MODULE INDEX

| # | Module | When |
|---|---|---|
| **00** | **Python Fundamentals** ⭐ | **Day 1 Block A — first** |
| 01 | Pipeline testing (pytest, Airflow/Dagster, Postman) | Day 1, after hands-on |
| 02 | Data contracts & Pandera | Day 2 Block C |
| 03 | Model behavior validation | Day 2, with stats |
| 04 | Performance / load testing | Day 2, skim |
| 05 | CI/CD & containers | Day 2, skim |
| 06 | LLM testing (LangSmith, judge, cost) | Day 2 |
| 07 | Observability | Day 2, skim |
| 08 | GenAI stack (PyTorch, LangChain, vLLM) | Day 2, skim |
| **09** | **Statistics From Zero** ⭐ | **Day 2 Block D** |

---

# THE THREE FACTS YOU KEEP FORGETTING

Drill these until automatic:

### 1. `lazy=True`
- `validate(df)` → **fail-fast**, first error only, raises `SchemaError`
- `validate(df, lazy=True)` → **collect-all**, raises `SchemaErrors` (plural),
  gives `.failure_cases` DataFrame
- **CI → lazy** (see everything in one run) · **Production → fail-fast**
  (abort cheaply)

### 2. Fake clock
**Never sleep in a test.** Inject time as a parameter:
```python
clock=FakeClock([0.0, 5.0])    # simulates 5s elapsing, runs instantly
```
Works because `__call__` makes the object callable, same as
`time.perf_counter`. Same pattern for `random`, `datetime.now()`, UUIDs.

### 3. Effect size vs p-value
**p = "is it real?" · statistic = "is it big?"**
With n=80,000 a statistic of 0.02 gives p=0.001 — significant but
meaningless. Gate on both.

---

# HOW TO POSITION YOURSELF

> *"I'm not from a traditional QA background — I've been building AI
> systems, and I came at testing from the builder side. What that gives me
> is that I've made these mistakes myself, so when I look at a pipeline my
> instinct is 'where would this fail quietly.' What I'm still building is
> the formal QA vocabulary and tooling depth."*

Naming the boundary of your knowledge **increases** credibility. Bluffing a
tool you haven't touched is the fastest way to lose an interviewer.

## On the Python feedback specifically

If they revisit it, don't deflect:

> *"That was fair feedback — I'd been using decorators without being able to
> articulate the mechanism. I went back to fundamentals: a decorator is a
> function that takes a function and returns a modified version, and `@` is
> just shorthand for `func = decorator(func)`. Same for context managers —
> `pytest.raises` works because `__exit__` inspects the exception and can
> swallow it or fail if nothing was raised."*

**Taking feedback and visibly closing the gap in two days is itself a
strong signal.**
