# QA / Data Science Engineer — Final Revision Cheat Sheet

Everything from the bootcamp, compressed. Read top to bottom in ~20 minutes.

---

## THE ONE-PARAGRAPH FRAME

Traditional software QA tests **code correctness**. ML QA adds two more categories:
**data contracts** (is the data shaped as promised?) and **behavioral/statistical
validation** (is the system's aggregate behavior still sane?). Most ML failures are
*silent* — no crash, no error log, just quietly wrong output. That's why schemas,
drift detection, and LLM evaluation exist.

---

## 1. PYTEST FUNDAMENTALS

**Discovery:** pytest finds files named `test_*.py`, then functions named `test_*`.
Pure naming convention — rename a test and it silently stops running.

**Pass/fail:** function returns normally = PASS. *Any* exception escaping = FAIL.

**Assertion rewriting:** pytest rewrites bytecode so a bare `assert` shows actual
values on failure (`assert 6 == 5`). That's why you never need `self.assertEqual`.

**Testing exceptions:**
```python
with pytest.raises(TypeError):
    classify(123)
```
Never use `try/except/pass` — if the code stops raising entirely, that test still
passes. `pytest.raises` *demands* the exception; it fails with `DID NOT RAISE`.
Also, `pytest.raises(TypeError)` fails if a `ValueError` shows up — it checks the
*right* error, not just any error.

**Fixtures** = reusable setup, injected by parameter name.
```python
@pytest.fixture
def llm_client():
    return LLMClient(base_url="https://mock-llm.local", api_key="test-key")

def test_thing(llm_client):     # pytest matches the name and injects it
    ...
```
- `conftest.py` = fixtures shared across all test files in that directory/subdirs
- `yield` instead of `return` adds teardown after the test
- 5-6 fixtures on one test = design smell (too many dependencies)

**Parametrize** = same test, many inputs. Generates N *separate* tests.
```python
@pytest.mark.parametrize("bad_input", [123, None, ["a"]])
def test_rejects_bad_types(bad_input):
    with pytest.raises(TypeError):
        classify(bad_input)
```
Better than a `for` loop because failures are isolated — one bad case doesn't
hide the rest.

**pytest is a test RUNNER, not a test type.** Unit, integration, data-contract,
E2E tests all run in pytest.

---

## 2. MOCKING

**Rule: mock the boundary you don't own (the network), not the code you do own.**

```python
# BAD - patches your own method, skips all real logic
with patch.object(LLMClient, "complete", return_value="technical"): ...

# GOOD - real code runs, only the network is faked
httpx_mock.add_response(url="https://mock-llm.local/v1/complete",
                        json={"completion": "technical"})
```
Method-patching hides bugs: rename the URL path or a JSON key and the test still
passes while production breaks. HTTP-level mocking exercises real URL building,
headers, and response parsing.

**Three uses:**
```python
httpx_mock.add_response(url=..., json={...})          # canned success
httpx_mock.add_response(status_code=500)              # simulated failure
httpx_mock.add_exception(httpx.ConnectTimeout("x"))   # network failure
requests = httpx_mock.get_requests()                  # spy on what was SENT
```

**Registering NO mock proves a call never happened** — httpx_mock raises on any
unexpected request, so a passing test = zero calls made. That's how you prove the
expensive path wasn't taken.

---

## 3. LLM TESTING (LangSmith concepts)

**Core problem:** you cannot `assert output == "expected"`. So you test for
**regression** ("did it get worse?") instead of **correctness** ("is it equal?").

**Four concepts:**
1. **Traces/runs** — every LLM call logged: input, output, latency, tokens, nested
   child calls. Makes multi-hop agent failures debuggable.
2. **Datasets** — curated golden set of (input, expected) pairs.
3. **Evaluators** — heuristic (exact match, regex, valid JSON) or LLM-as-judge.
4. **Regression testing** — change prompt → re-run golden set → diff scores vs
   baseline. Catches "improved 2 examples, silently broke 5."

**Determinism checks:**
- At `temperature=0`, same input should give same output. Drift means investigate:
  provider silently changed the model, prompt template injects a timestamp/unordered
  set, retrieval returned different docs, or a cache bug.
- At `temperature>0`, variation is expected — check the output still *passes the
  same evaluators*, not that text is identical.

---

## 4. LLM-AS-JUDGE + RULE-BASED

**Two layers, both needed:**

*Rule-based* (free, instant, deterministic): banned words, regex PII, numbers in
output not present in source (hallucination proxy), JSON validity, length bounds.
**Blind spot:** semantic errors. "The contract was NOT renewed" vs "was renewed" —
every word and number matches; pattern matching has no concept of negation.

*LLM-as-judge* (nuanced, costly, non-deterministic): another model grades against a
rubric, returns strict JSON.

**The interview gold — what happens when the judge returns invalid JSON:**
- **Fail safe** (`hallucinated: True`) → block it. For customer-facing/medical/legal.
- **Fail open** (`False`) → let it through. For low-stakes internal tools.
- **Raise** → surface as system error.
Having made the choice *deliberately* is the signal. There's no universal answer.

**Judge weaknesses:** position bias (favors first option), verbosity bias (rates
longer answers higher), run-to-run inconsistency. Mitigate: temperature=0, use a
stronger model as judge, validate against human labels periodically.

**Production layering:** rule-based on every request → judge on sampled traffic +
CI golden set → human review for disagreements.

---

## 5. COST / TOKEN / TIMEOUT

Different failure class: not "is it correct" but "was it affordable and fast enough."

- A prompt change that stuffs a whole document instead of a snippet still produces
  a *correct-looking answer* — costs 50x more. Only a token assertion catches it.
- A 200 response that took 6s against a 2s SLA is a **failure**, even though
  functionally it "worked."

**Dependency-injected clock** — the reusable technique:
```python
def __init__(self, client, timeout_budget_s=2.0, clock=time.perf_counter):
    self._clock = clock        # inject, don't hardcode

# in tests:
FakeClock([0.0, 5.0])          # simulates 5 seconds elapsing, instantly
```
Better than monkeypatching global `time` because it changes exactly one object,
not everything in the process. Same trick for `random`, `datetime.now()`, UUIDs.

**Separation of concerns:** `LLMClient` talks to the API; `InstrumentedLLMClient`
wraps it and measures. This is the **decorator pattern** — same interface, extra
behavior. Benefits: optionality, testability, readability.

---

## 6. PANDERA DATA CONTRACTS

Turns **implicit** DataFrame assumptions (in someone's head) into an **explicit**,
executable contract.

```python
import pandera.pandas as pa
from pandera.pandas import Check, Column, DataFrameSchema

prediction_schema = DataFrameSchema({
    "id":              Column(int, unique=True, checks=Check.ge(0)),
    "predicted_label": Column(str, checks=Check.isin(["billing","technical","account","other"])),
    "confidence":      Column(float, checks=Check.in_range(0.0, 1.0)),
})
```

Four dimensions: **type**, **nullability**, **uniqueness**, **value constraints**.

**`validate()` does 3 things:** checks all rules → raises on violation → **returns
the DataFrame** if clean (it's a pass-through, not a bool).

**lazy=True — near-guaranteed question:**
- `validate(df)` → fail-fast, raises `SchemaError` on the FIRST problem
- `validate(df, lazy=True)` → collect-all, raises `SchemaErrors` (plural) with
  `.failure_cases` DataFrame listing every violation
- **CI → always lazy** (see all problems in one run, not 5 round trips)
- **Production runtime → fail-fast** (abort cheaply)

**Test both directions, and assert on SPECIFIC columns:**
```python
with pytest.raises(pa.errors.SchemaErrors) as exc:
    prediction_schema.validate(df, lazy=True)
assert "confidence" in set(exc.value.failure_cases["column"])
```
Asserting *which* column failed proves you caught the planted defect, not just
that "something" broke.

**Real story:** upstream changes confidence from fraction (0.87) to percent (87.0)
and doesn't tell you. Without a schema → silently garbage dashboards for weeks.
With it → CI fails tonight with the exact row and value.

---

## 7. STATISTICAL VALIDATION & DRIFT

**Schema checks each ROW. Statistics check the BATCH.** A model predicting "other"
for 98% of tickets passes every schema check — every value is valid — but the model
has collapsed.

```python
# class balance - catches model collapse
fractions = labels.value_counts(normalize=True).to_dict()

# drift - KS two-sample test
result = stats.ks_2samp(baseline, current)
result.statistic   # EFFECT SIZE: how different (0=identical, 1=disjoint)
result.pvalue      # SIGNIFICANCE: probability this gap is chance
```

**Hypothesis testing:**
- **H0 (null)**: both samples from the same distribution (no drift)
- **p-value** = probability of seeing a gap this large *IF H0 were true*
- p < alpha → reject H0 → drift detected

**p-value is NOT:** the probability H0 is true, the probability you're wrong, or a
measure of how big the difference is.

**alpha** = threshold YOU choose = your accepted **false-positive rate**. alpha=0.05
means ~1 in 20 stable runs will falsely alarm. That's not a bug, it's the rate you chose.

| | H0 true (no drift) | H0 false (real drift) |
|---|---|---|
| Reject H0 | **Type I** (false positive, rate=alpha) | correct |
| Don't reject | correct | **Type II** (false negative) |

Lower alpha → fewer false alarms, more missed real drift. Can't reduce both without
more data.

**THE BIG ONE — effect size vs significance:** with n=50,000, a `statistic` of 0.02
(practically nothing) gives `p<0.001` (wildly significant). p-value depends on
effect size AND sample size.
```python
drifted = (result.pvalue < alpha) and (result.statistic > 0.1)
```
Saying this proves you've operated a monitoring system, not just read a tutorial.

**Multiple comparisons:** 20 features at alpha=0.05 → `1-(0.95)^20 = 64%` chance of
at least one false alarm EVERY run. Fixes: **Bonferroni** (alpha/n, conservative) or
**Benjamini-Hochberg / FDR** (better for monitoring).

**KS test:** non-parametric (no distribution assumption), compares whole
distributions not just means, **continuous data only**. For categorical → chi-square
or PSI.

**PSI (Population Stability Index):** <0.1 stable, 0.1-0.25 moderate, >0.25
significant. Pure effect size, no sample-size sensitivity, works on categorical.
Preferred by many monitoring teams for exactly that reason.

**Testing randomness requires seeding:**
```python
rng = np.random.default_rng(42)     # or test is flaky
```

---

## 8. THE FOUR ML PIPELINES (cadence matters)

```
1. TRAINING          - infrequent (weekly/monthly/triggered)
   data -> clean -> features -> TRAIN -> evaluate -> register model

2. BATCH INFERENCE   - nightly/hourly  <- the usual "nightly job"
   new data -> features -> LOAD existing model -> predict -> write

3. REAL-TIME         - per request
   request -> features -> predict -> response

4. MONITORING        - nightly/hourly
   yesterday's data + predictions -> drift + quality checks -> alerts
```

**Nightly jobs are almost never retraining.** Same model artifact runs every night;
the *data* is new, not the model. Training is expensive and risky.

**Retraining triggers:** scheduled (simple, wasteful) → drift-triggered (monitoring
fires it) → performance-triggered (accuracy drops; needs ground-truth labels).

**Where each thing runs:**
| Thing | Where |
|---|---|
| pytest, mocking | CI only |
| Pandera | **Both** — CI tests + runtime gates |
| Drift/statistical | Mostly production monitoring; logic unit-tested in CI |
| Dagster materialize() | CI |
| LLM-judge | CI golden set + production sampling |
| Locust | Pre-release / scheduled, NOT per-commit |

Same logic, two roles: Pandera on a fixture CSV = **test**. Same schema on tonight's
real data = **runtime gate**. CI failure = "dev broke code." Runtime failure =
"upstream sent bad data."

---

## 9. ORCHESTRATION (Dagster / Airflow)

**Neither is a CI/CD tool.** CI triggers on **code change** (is the code correct?).
Orchestrators trigger on **schedule/data events** (run the workflow forever).

```python
@dg.asset
def validated_predictions(raw_predictions: pd.DataFrame) -> pd.DataFrame:
    return prediction_schema.validate(raw_predictions)

# test it in-process, no scheduler, no containers:
result = dg.materialize([raw_predictions, validated_predictions, drift_report],
                        resources={"csv_path": CSVPathResource(path="good.csv")})
assert result.success is True
```
Dependencies wire by **parameter name** (same idea as pytest fixtures).

| | Airflow | Dagster |
|---|---|---|
| Abstraction | **Tasks** ("run this") | **Assets** ("produce this data") |
| Testing | needs metastore/scheduler | `materialize()` in-process |
| Data awareness | task-centric | asset-centric, tracks lineage |
| Maturity | older, enterprise-standard | newer, better DX |

**Airflow testing:** `airflow dags test <id> <date>`, plus **DAG integrity tests** —
import every DAG, assert it parses, no cycles, no duplicate task_ids, every task has
retries/timeout set.

**Containers:** orchestrators run containerized in production, but **tests run as
plain Python** — that's the whole point.

---

## 10. CI/CD & MERGE GATES

```yaml
on:
  pull_request:
    paths: ["QA-Data-Science-Engineer-Bootcamp/**"]   # only when relevant files change
jobs:
  tests:                          # jobs run in PARALLEL by default
    runs-on: ubuntu-latest
    steps:                        # steps run SEQUENTIALLY
      - uses: actions/checkout@v4        # uses = prebuilt action
      - run: pytest -v                   # run = shell command
```

**THE key point: a red X does NOT block a merge.** The workflow only *reports*
status. Blocking requires **branch protection rules** → "Require status checks to
pass before merging" → select job names.
**"CI ran" and "CI gated the merge" are two separate configurations.**

**ML-specific CI decisions:**
- Per-commit: unit + schema tests (fast, deterministic)
- Nightly/pre-release: load tests, full drift analysis, large LLM-judge evals
- **LLM tests MUST be mocked in CI** — real calls = cost, flakiness, secrets in runner
- Flaky tests are worse than no tests — they train people to hit "re-run" past real
  failures

---

## 11. DOCKER / K8S (concepts only)

Version differences in ML don't just crash — they **silently change numerical
output**. Pinned images remove that variable.

- **Job** (not Deployment) = right primitive for tests: runs to completion, stops
- **Sidecar** = mock service in the same pod, tests hit `localhost` (pytest-httpx
  at the infra layer)
- **Ephemeral namespace per PR** = disposable integration environment
- **Resource limits**: exceed memory → **OOMKilled**; exceed CPU → **throttled**
  (slow, not killed). #1 cause of "works locally, times out in CI"

GitHub Actions runners are already containers — for a single Python suite you often
don't need K8s at all.

---

## 12. PERFORMANCE TESTING

**RPS** = Requests Per Second = throughput.
**k6** = load testing tool by Grafana Labs (JavaScript). **k8s** = Kubernetes.
Unrelated, similar names are coincidence. **k6 and Locust are competitors.**

```python
class InferenceUser(HttpUser):
    wait_time = between(0.5, 2)     # think time; without it 10 fake users act like 1000
    @task(5)                         # weight: runs 5x as often
    def predict(self): self.client.post("/predict", json={"text": "..."})
    @task(1)
    def health(self): self.client.get("/health")
```
`-u 100 -r 50` = 100 users, spawned 50/sec.

**Reading output — ALWAYS percentiles, never average.** 99 requests at 100ms + 1 at
10,000ms = 200ms average (looks fine!) but one user waited 10 seconds. SLAs are
written as "p95 < 500ms", never "average < 500ms."

**Real measured results (this repo's mock service):**
| | 5 users | 100 users |
|---|---|---|
| RPS | 3.08 | 59.44 |
| median | 190ms | 200ms |
| p95 | 310ms | 340ms |
| failures | 0% | 0% |
→ 20x load, latency essentially flat, zero errors = **nowhere near saturation.**

**Saturation signature:**
```
Users:  10    50    100   200    400
RPS:    20    95    140   145    143    <- PLATEAU
p95:   120   180   400  3200   9000ms   <- EXPLODES
```
Service can only process ~145/sec; beyond that requests **queue**, and queue time
adds to latency. The **knee** is your real capacity.

**Locust vs k6:** k6 has **`thresholds`** in the script (`p(95)<800`) so
`k6 run` exits non-zero — a CI gate by itself. Locust prints a report; a human decides.

**Types of performance testing:**
| Type | Question |
|---|---|
| **Load** | handles expected peak? |
| **Stress** | where does it break, and how? |
| **Soak/endurance** | memory leaks over hours? (underrated) |
| **Spike** | survives sudden 10x? |
| **Scalability** | does 2x servers = 2x throughput? |

**Tail latency causes:** queueing, GC pauses, cold start/cache, noisy neighbor,
pool exhaustion, payload variance (huge for LLMs — cost/latency scale with tokens),
network retries.

**"Slow" != "failed"** — different signals, different fixes.

**What makes a load test unrealistic:** too-smooth arrival pattern (real traffic is
bursty), identical payloads, no cold starts, no resource visibility (Locust shows
latency not CPU/memory — pair with `docker stats`/cAdvisor), and vLLM's continuous
batching means throughput can *improve* with concurrency.

---

## 13. OBSERVABILITY

```
Service exposes /metrics -> Prometheus SCRAPES (pull!) -> TSDB
                                    |-> PromQL -> Grafana dashboards
                                    |-> alert rules -> Alertmanager -> PagerDuty
```

**Prometheus PULLS.** Your app never pushes; Prometheus polls `/metrics` on an
interval. (Exception: **Pushgateway** for short-lived batch jobs — that's the answer
for "how do you monitor a nightly Dagster job that exits?")

**RED method:** **R**ate, **E**rrors, **D**uration (percentiles). Same thing Locust
measured for one run — Prometheus is that running forever in production.

**ML-specific metrics** (RED stays green while a model is silently broken):
| Metric | Catches |
|---|---|
| Prediction distribution | model collapse (98% one class) |
| Input drift (KS/PSI) | production diverging from training data |
| Token usage & $/request | prompt change that 50x'd cost |
| Latency by model/provider | one backend degrading |
| Rolling LLM-judge score | quality regression as a trend |

---

## 14. QA MINDSET — THE REFRAME

Every answer shifts from *"here's what I built"* to *"here's how I'd know if it broke."*

**Three questions to expect:**
1. Where could this fail **silently**? (no crash, no log, wrong output) — THE ML QA question
2. How would you **know** it broke?
3. What did you **NOT** handle?

**Silent failure -> detection table:**
| Silent failure | Detection |
|---|---|
| Model collapses to one class | class balance check |
| Input drifts from training | KS / PSI |
| Prompt change doubles cost | token/cost assertion in CI |
| Upstream changes units (0.87 -> 87.0) | Pandera range check |
| LLM returns valid JSON failing downstream schema | schema validation on LLM output |
| Fast but subtly wrong response | LLM-judge on golden set |
| Agent loops, burns cost | iteration cap + test proving it fires |
| Retrieval returns junk, answer still fluent | grounding/faithfulness judge |

**Phrases that land:**
- "Standard metrics tell you the service is up. They don't tell you the model is right."
- "CI running and CI gating a merge are two different configurations."
- "You can't assert equality on LLM output, so you test for regression instead of correctness."
- "Schema checks each row; statistical checks the batch. Neither replaces the other."
- "Statistical significance isn't practical significance — gate drift alerts on effect size too."
- "Mock the boundary you don't own."
- "Slow and failed are different signals."

**Honesty positioning (use this):**
> "I've built AI systems for a few years and came at QA from the builder side, so my
> instinct is to ask what I'd need to verify before trusting this in production.
> I've worked hands-on with pytest, mocking, Pandera and drift detection; for
> Kubernetes test infrastructure and Prometheus setup I understand the patterns but
> haven't operated them myself."

Naming the boundary of your knowledge **increases** credibility.

**Questions to ask them:**
- "What's your split between unit, integration, and eval tests for GenAI workflows?"
- "How do you decide what runs per-commit vs nightly?"
- "For LLM features — offline eval on a golden set, production sampling, or both?"
- "What's the most common way things break in production today?"
