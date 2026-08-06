# ROUND 2 — MASTER CHEAT SHEET

Read order: **Flashcards → Scenarios → Your Project → Phrases**.
Flashcards first: those are the things you've repeatedly gotten wrong.

---

# PART 1 — FLASHCARDS (drill until automatic)

## ⭐ The 5 you keep missing

### 1. Precision vs Recall

```
                     Model says CRITICAL     Model says NOT critical
Actually critical      TP                      FN  ← MISSED THREAT
Not critical           FP  ← false alarm       TN
```

| | Formula | Question | Measures |
|---|---|---|---|
| **Recall** | TP / (TP + **FN**) | "of all criticals that EXIST, how many did we catch?" | **misses** |
| **Precision** | TP / (TP + **FP**) | "of all we CALLED critical, how many really were?" | **false alarms** |

- **Recall** → did we **recall** everything out there? → denominator = **reality**
- **Precision** → when we spoke, were we right? → denominator = **our claims**
- **Security: recall matters more.** Missed critical = breach. False positive = wasted hour.

### 2. p-value vs effect size
- **p-value** = "is this difference REAL?" (not chance)
- **statistic / effect size** = "is this difference BIG?"
- **The trap:** p depends on effect size AND sample size. n=120,000 makes a 0.03 statistic give p=0.001.
- **Gate on both:** `p < alpha AND statistic > 0.1`
- **PSI** avoids the trap entirely (<0.1 stable, 0.1–0.25 moderate, >0.25 act)

### 3. Calibration
"90% confident" should mean **right 90% of the time**.
Right 60% of the time = **overconfident = miscalibrated**.
Matters because confidence drives automation ("above 0.9 auto-approve").
**Accuracy can be unchanged while calibration silently breaks.**

### 4. SchemaError vs SchemaErrors

| | `validate(df)` | `validate(df, lazy=True)` |
|---|---|---|
| Behavior | fail-fast, first error only | collect ALL errors |
| Raises | `SchemaError` (singular) | `SchemaErrors` (**plural**) |
| Extra | — | `.failure_cases` DataFrame |
| Use in | **production** (abort cheap) | **CI** (see everything at once) |

### 5. Fake clock — never sleep in a test
```python
class FakeClock:
    def __init__(self, values): self._values = iter(values)
    def __call__(self):        return next(self._values)   # __call__ = callable object

client = InstrumentedLLMClient(llm, timeout_budget_s=2.0, clock=FakeClock([0.0, 5.0]))
# start=0.0, end=5.0 -> elapsed=5.0 -> raises. Runs in microseconds.
```
Better than monkeypatching global `time` — changes **one object**, not the whole process.
Same trick for `random`, `datetime.now()`, UUIDs.

## Python fundamentals (the flagged weakness)

### Decorator
**`@d` above a function means `func = d(func)`.** A function that takes a function and returns a modified version.

Three purposes:
| Purpose | Example |
|---|---|
| **Wrap behavior** | `@timer`, `@retry`, `@lru_cache` |
| **Register** | `@pytest.fixture`, `@dg.asset` (returns func unchanged, records it in a dict) |
| **Tag metadata** | `@pytest.mark.parametrize` (attaches data a runner reads later) |

`*args, **kwargs` in the wrapper = accept and forward any arguments.
`@functools.wraps(func)` = preserve the original name/docstring.

### Context manager (`with`)
Object with `__enter__` and `__exit__`. `__exit__` **always** runs and receives the exception info.

`pytest.raises(TypeError)`:
| Body raises | `exc_type` | `__exit__` does | Result |
|---|---|---|---|
| `TypeError` | TypeError | swallow (return True) | ✅ PASS |
| nothing | **None** | raise `DID NOT RAISE` | ❌ FAIL |
| `ValueError` | ValueError | propagate (return False) | ❌ FAIL |

**Why `try/except/pass` is dangerous:** `except` has no hook for "nothing was raised," so if the code stops raising the test silently passes.

### `yield` in a fixture
```python
@pytest.fixture
def db():
    conn = connect()   # SETUP
    yield conn         # <- test runs here
    conn.close()       # TEARDOWN, runs even if the test failed
```
`return` ends the function. `yield` **pauses** it. pytest calls `next()` twice: once for setup, once after the test for teardown.

### Exceptions
| Error | Means |
|---|---|
| `TypeError` | wrong **kind** of thing (`"abc" + 5`) |
| `ValueError` | right kind, **bad value** (`int("hello")`) |
| `KeyError` | dict key missing (API renamed a field) |
| `AttributeError` | object has no `.that` (called `.predict()` on `None`) |
| `AssertionError` | an assert was False = **a test failed** |
| `JSONDecodeError` | LLM returned prose instead of JSON |

**Read a traceback BOTTOM-UP:** last line = *what*, bottom-most `File` line = *where*.

### assert
`assert X` = "X must be true, else raise `AssertionError`." Identical to `if not X: raise AssertionError`.
In a test: no exception escapes = PASS. A false assert raises = FAIL.

---

# PART 2 — THE 8 SCENARIOS

**The single thread: every failure was silent. Nothing crashed. Output looked plausible.**

| # | Failure | Why invisible | The gate |
|---|---|---|---|
| 1 | Stage swallowed exception | empty result = valid answer | `assert result.success is False` |
| 2 | Vendor changed CVSS encoding | `coerce` → NaN → sorted last | Pandera at ingest |
| 3 | Retrain dropped critical recall 90→60% | aggregate accuracy hid it | `assert critical_recall >= 0.85` |
| 4 | CVE spike, 40× in 6 min | only smooth ramps tested | `LoadTestShape` + degradation test |
| 5 | Model 30% more conservative | the code WAS correct | shadow deploy + human sign-off |
| 6 | Inverted remediation / 2× token cost | invisible in code review | policy rubric + cost assertion |
| 7 | 11 days of wrong output | RED metrics all green | per-customer volume, asymmetric alert |
| 8 | vLLM latency cliff | smooth-ramp averaging | fine-grained sampling at saturation |

## The two recurring villains
```python
except Exception:  return pd.DataFrame()   # scenario 1
pd.to_numeric(x, errors="coerce")          # scenario 2
```
Both mean *"if something goes wrong, quietly produce something that looks fine."*

## Scenario details

**1 — Pipeline.** Four layers: DAG integrity (parses? retries set?), unit (function correct?), **stage-boundary** (pipeline fails loudly?), golden fixture (output drifted?).
**Assert on run status, not return value** — the only assertion a developer can't satisfy with dummy data.
Postman = live exploration; pytest-httpx = permanent guardrail. Newman = Postman's CLI.

**2 — Data contracts.** Ingest from hundreds of customers = untrusted input.
**Hard fail** (CVSS out of range → quarantine that record) vs **soft anomaly** (new scanner_version → flag, don't block other 199 customers).
`strict=True` breaks the day a vendor adds a field → strict on fields you depend on, permissive + drift alert elsewhere.
Cross-field invariants: critical severity must have a CVE; timestamp not in the future (clock skew is real).

**3 — Model behavior.** Aggregate accuracy is dominated by the common class → **slice per class**.
**Frozen reference set** (fixed inputs, known labels) separates "model broke" from "world changed."
Business invariants enforced **outside** the model (actively-exploited CVE never scores below medium).

**4 — Performance.** **Shape > peak volume.** Smooth ramp lets pools/caches/autoscalers warm up; a spike doesn't.
Load-test **ingest** (writes) not just inference (reads).
Latency says *that* it broke; resource metrics say *why*.
Pick numbers from **production peaks**, never invent them.
Types: load / stress / soak (memory leaks) / spike / scalability.

**5 — CI/CD.** Software CI asks "is the code correct." **ML CI must also ask "is the new model's behavior acceptable."**
**Shadow deployment**: new + current model on the same replayed traffic, diff outputs, human signs off before promotion.
Required status checks must include schema + statistical suites, not just pytest.
**A red X does NOT block a merge** — branch protection rules do. Two separate configurations.
Containers matter because a shadow diff is only trustworthy with identical environments.

**6 — LLM testing.** Two failure classes: **correctness/safety** and **cost/operability**.
Policy adherence ≠ hallucination — "disable the firewall" fabricated nothing, it **inverted direction**.
Rule-based first pass (dangerous verb near security noun) → LLM-judge second ("does this reduce or increase exposure?").
Determinism needs a **frozen input** — live threat intel changes daily by design.
Cost assertion in CI: `assert cost_per_call <= baseline * 1.10`.
Test the **fallback**, not just the timeout detection.

**7 — Observability.** RED metrics (rate/errors/duration) say the service is up, not that the model is right.
The metric that catches it: **count of critical findings, per customer, per day** — simple, sliced.
**Asymmetric alerting**: a drop to zero for one customer pages immediately; a spike waits for Monday.
Prometheus **pulls** (scrapes `/metrics`). **Pushgateway** for short-lived batch jobs.
Alert fatigue on your team = the same disease the product treats for customers.

**8 — GenAI stack.** Test the **testable surface**, not internals.
- **PyTorch**: input/output contract — tensor shape/dtype (shape mismatch = silent broadcasting bug), output range after softmax, frozen tensor → stable output
- **LangChain**: **your orchestration logic** — branching, retries, parsing, fallbacks. The library is a dependency, not your code. Bound the tool-calling loop and test the bound fires.
- **vLLM**: **continuous batching** — a finished request's slot is reused immediately instead of waiting for the batch. Latency stays FLAT while slots are free, then JUMPS when the GPU saturates. Test implication: sample finely near saturation, vary payload length, and know throughput *improves* with concurrency up to the cliff.

---

# PART 3 — YOUR DRUG-LIKENESS PROJECT, QA-FRAMED

**They will ask about your work. This is your best asset — a real ML system you built and can critique.**

## The 30-second description
> *"A deep-learning drug-likeness predictor. You give it a molecule as a SMILES string — a text encoding of chemical structure — and a hybrid CNN-BiLSTM-LSTM model predicts whether it has drug-like properties. Trained on 250,000 molecules from ZINC, about 85% accuracy, 0.89 ROC-AUC. Flask/Gradio front end, deployed on HuggingFace Spaces."*

## Critiquing your own test suite (say this unprompted — it's a strong move)
> *"I do have a test suite, but it's a hand-rolled runner with a global pass/fail counter — not pytest. If I rewrote it today: convert each check to a `test_*` function so failures isolate instead of one bad case masking the rest, `parametrize` the SMILES validation cases, and move model loading into a fixture so it loads once per session instead of per test."*

**Self-critique demonstrates the QA mindset better than any claim of coverage.**

## Silent failures in YOUR project

| Silent failure | Why it's silent | The gate |
|---|---|---|
| **Invalid SMILES** | RDKit returns `None`; tokenizer produces garbage; model outputs a confident number anyway | `assert Chem.MolFromSmiles(s) is not None` **before** inference |
| **Molecule > 71 chars** | silently truncated → **you scored a different molecule** | length check that raises, not truncates |
| **Unknown token** (vocab = 89) | out-of-vocab char → mapped to padding → wrong encoding | explicit OOV test |
| **Class imbalance** | 85% accuracy could be near-baseline if the dataset is mostly drug-like | report precision/recall/ROC-AUC, not accuracy alone |
| **0.5 threshold is arbitrary** | nobody chose it for a business reason | tune to cost: for screening, FN (missing a real drug) costs more than FP |
| **tokenizer.pkl ↔ model drift** | vocab and model input layer can diverge | test that vocab size matches the model's expected input |
| **Model file missing/corrupt** | fails at first request, not at deploy | startup smoke test |

**Your two strongest lines** (they show ML judgment, not just testing mechanics):
> *"85% accuracy on its own isn't meaningful without knowing the class balance — I'd want precision, recall and a confusion matrix, since ROC-AUC of 0.89 tells me more than accuracy does."*

> *"The 0.5 decision threshold was a default, not a decision. For a screening tool, a false negative — discarding a real drug candidate — costs far more than a false positive that just wastes a bit of lab time. I'd tune the threshold to that asymmetry."*

## If asked "how would you productionize this?"
> *"Pandera schema on the input batch, RDKit validity as a hard gate before inference, output range assertion after the sigmoid, per-class metrics in CI rather than aggregate accuracy, and a frozen reference set of ~100 molecules with known labels re-scored on every model change so I can tell a regression from a legitimate improvement."*

---

# PART 4 — PHRASES THAT LAND

- *"A failure state and a valid empty state had the same representation."*
- *"Write assertions that can only be satisfied by fixing the actual problem."*
- *"Schema checks each row; statistics check the batch. Neither replaces the other."*
- *"Aggregate metrics optimize for the common case and go blind on the rare, high-stakes one."*
- *"Statistical significance isn't practical significance."*
- *"Standard metrics tell you the service is up. They don't tell you the model is right."*
- *"CI running and CI gating a merge are two different configurations."*
- *"Mock the boundary you don't own."*
- *"Traffic shape matters more than peak volume."*
- *"Slow and failed are different signals."*
- *"A tester finds bugs. A QA engineer defines what 'correct' means and builds the gate."*

## Honesty positioning (use it — don't bluff)
> *"I'm not from a traditional QA background — I've been building AI systems and came at testing from the builder side. That means I've made these mistakes myself, so my instinct when I look at a pipeline is 'where would this fail quietly.' What I'm still building is the formal QA vocabulary and tooling depth."*

## On the Python feedback from round 1
> *"That was fair feedback — I'd been using decorators without being able to articulate the mechanism. A decorator is a function that takes a function and returns a modified version; `@` is shorthand for `func = decorator(func)`. Frameworks also use them for registration rather than wrapping — `@pytest.fixture` just records the function in a registry so it can be injected by name."*

**Taking feedback and visibly closing the gap in two days is itself a strong signal.**

## Questions to ask them
- "What's your split between unit, integration, and eval tests for the GenAI workflows?"
- "How do you decide what runs per-commit versus nightly?"
- "For LLM features — offline eval on a golden set, production sampling, or both?"
- "What's the most common way things break in production today?"
- "Is there a shadow-deployment or staged-rollout step for model changes?"

---

# THE 60-SECOND PRE-INTERVIEW SCAN

1. **Recall** = TP/(TP+FN) = misses · **Precision** = TP/(TP+FP) = false alarms
2. **p = is it real · statistic = is it big** — need both
3. **`SchemaErrors`** (plural) = lazy = CI · **`SchemaError`** = fail-fast = production
4. **Decorator** = `func = d(func)`
5. **`pytest.raises`** works because `__exit__` sees `exc_type is None` when nothing raised
6. **Red CI ≠ blocked merge** — branch protection is separate
7. **Prometheus pulls**; Pushgateway for batch jobs
8. **Percentiles, never averages**
9. **Empty result ≠ no findings** — make failure distinguishable
10. **Every insight ends in a gate, not a finding**
