# 1. Pipeline Testing: pytest, Airflow/Dagster, Postman

> **Read this even if you're new to testing.** We start from "what is a
> pipeline" and end at the reasoning a round-2 interviewer is listening for.

---

## LEVEL 0 — Plain English first

### What's a "pipeline"?

A pipeline is just **a series of steps that run one after another, on a
schedule, without a human watching.**

```
Step 1: get the data  →  Step 2: process it  →  Step 3: write the result
```

VulnPrioritize's nightly pipeline:

```
ingest_scan_results  →  score_risk  →  draft_remediation
(pull scan data)        (ML ranks it)    (LLM writes summary)
```

It runs at 2am for every customer. Nobody is watching it.

### What's a "DAG"?

**D**irected **A**cyclic **G**raph. Scary name, simple idea:

- **Directed** — steps flow one way (A then B, not B then A)
- **Acyclic** — no loops (A→B→A would be a cycle, and it'd run forever)
- **Graph** — boxes connected by arrows

That's it. A DAG is a flowchart of your steps. Airflow and Dagster are
tools that **run DAGs on a schedule**.

### What's an "orchestrator"?

The tool that runs the DAG — starts step 1, waits for it, then starts step
2, retries if something fails, and tells you what happened. Airflow and
Dagster are orchestrators. Think of them as a very reliable alarm clock
plus a to-do list.

---

## THE STORY (what actually went wrong)

Three weeks ago, `score_risk` broke for one customer segment — findings
where the `asset_criticality` field was empty (`null`).

But it **didn't crash.** The code looked like this:

```python
def score_risk(findings_df):
    try:
        scored = model.predict(findings_df)
        return rank(scored)
    except Exception as e:
        log.warning(f"scoring failed: {e}")   # writes to a log nobody reads
        return pd.DataFrame()                  # returns an EMPTY table
```

So:
1. Scoring failed silently
2. It returned an **empty table**
3. `draft_remediation` received an empty table and wrote
   *"no critical vulnerabilities found"*
4. That customer actually had several
5. **Eleven days later**, they got breached through one of them

Every step "worked." The pipeline was green. Nobody was alerted.

---

## LEVEL 1 — The basic lesson

### The bug is the `except` block. Two separate mistakes:

**Mistake 1: `except Exception` catches everything.**

```python
except Exception as e:      # catches EVERY possible error
```

This swallows bugs you never anticipated. It's like putting tape over
every warning light in your car.

**Mistake 2 (the deadly one): the error state looks identical to a valid state.**

An empty table means "no vulnerabilities found." That is a **perfectly
normal, valid business answer** — a secure customer would genuinely have
an empty list.

So downstream, there is **no way to tell these two apart:**

| What happened | What downstream sees |
|---|---|
| Customer is genuinely secure | empty table |
| Scoring crashed | empty table |

**This is the whole lesson.** Say it in the interview like this:

> *"The bug wasn't in the model or the LLM. It was that a failure state and
> a valid empty state had the same representation."*

### The fix

```python
def score_risk(findings_df):
    required = ["asset_criticality", "cvss_score", "exposure"]
    missing = findings_df[required].isnull().any()
    if missing.any():
        raise ValueError(f"null values in required fields: {missing[missing].index.tolist()}")
    return rank(model.predict(findings_df))
```

Fail **loudly**, **early**, and **name what's wrong**. If it can't do its
job, it must say so — not quietly return something that looks fine.

---

## LEVEL 2 — The four layers of pipeline testing

Different tests catch different bugs. Here's the full picture:

### Layer 1: DAG integrity tests (cheapest)

Don't test behavior. Just test that the DAG **is structurally valid**.

```python
def test_all_dags_are_valid():
    for dag in load_all_dags():
        assert dag is not None                      # it parses
        assert not dag.has_cycles()                 # no infinite loops
        for task in dag.tasks:
            assert task.retries >= 1                # retries configured
            assert task.execution_timeout is not None  # won't hang forever
```

**Catches:** someone's PR broke the DAG file, a task can hang forever, a
typo'd task ID. Cheap insurance, runs in seconds.

### Layer 2: Unit tests (what you already know)

Test one function alone with clean input.

```python
def test_score_risk_ranks_by_severity():
    df = pd.DataFrame({"cvss_score": [9.8, 3.1], "asset_criticality": [1.0, 0.2]})
    result = score_risk(df)
    assert result.iloc[0]["cvss_score"] == 9.8      # highest risk first
```

**Catches:** wrong logic. **Misses our bug entirely** — nobody writes a
unit test for null `asset_criticality` unless they already thought of it.

### Layer 3: Stage-boundary tests ⭐ (the one that catches our bug)

Run the step **through the orchestrator** with deliberately bad input, and
check the **whole run fails**.

```python
def test_malformed_input_fails_the_run_instead_of_returning_empty():
    result = dg.materialize(
        [ingest_scan_results, score_risk, draft_remediation],
        resources={"scan_data": ScanResource(path="fixtures/null_criticality.csv")},
        raise_on_error=False,     # capture the failure instead of exploding the test
    )
    assert result.success is False       # ← the run must FAIL
```

**Why assert on `result.success` and not the returned data?**

This is the most important technique in this module. If you wrote:

```python
assert len(output) > 0            # ✗ weak
```

...a developer could make that pass by returning dummy rows. Test green,
silence still there.

But `assert result.success is False` can only be satisfied two ways:
1. Re-raise the exception, or
2. Explicitly route to a quarantine/dead-letter path

**Both are visible.** You've made silence *impossible to implement*.

> **The principle:** write the assertion so the only way to make it pass
> is to actually fix the problem — not to satisfy the assertion.

### Layer 4: Golden fixture regression

Snapshot a known-good output, then compare future runs against it.

```python
def test_scoring_output_matches_snapshot():
    output = score_risk(pd.read_csv("fixtures/golden_customer_scan.csv"))
    expected = pd.read_json("fixtures/golden_scoring_output.json")
    pd.testing.assert_frame_equal(output, expected)
```

**"Golden fixture" vs "golden assertion" — why this matters:**

```python
# Golden ASSERTIONS — doesn't scale, brittle
assert output.iloc[0]["risk_score"] == 0.87
assert output.iloc[1]["risk_score"] == 0.72
# ...40 more lines, all break when the model legitimately improves

# Golden FIXTURE — one comparison, whole output
pd.testing.assert_frame_equal(output, expected)
```

**Important nuance:** here, a failure is **not automatically a bug.** If
you retrained the model, scores *should* change. The value is that the
change is **surfaced** instead of silent. A human reviews the diff, and if
it's intentional, regenerates the snapshot.

This is unusual — normally red means broken. Here red means *"a human must
confirm this was on purpose."*

### The scorecard for our bug

| Layer | Would it have caught it? |
|---|---|
| DAG integrity | ❌ The DAG was structurally fine |
| Unit test | ❌ Passes — nobody tested null criticality |
| **Stage-boundary** | ✅ **This is the one** |
| Golden fixture | ⚠️ Only if a null-criticality customer was in the fixture |

**Being honest about the misses is a stronger interview answer** than
claiming your whole strategy would have caught it.

---

## LEVEL 3 — Postman vs. pytest-httpx

Both deal with APIs. They are **not** competitors — they do different jobs.

| | pytest-httpx | Postman |
|---|---|---|
| **When** | every commit, forever | live, while investigating |
| **Speed** | CI-speed (milliseconds) | human-speed (you clicking) |
| **Purpose** | permanent guardrail | exploration |
| **Finds** | *known* bugs coming back | *unknown* bugs, first time |

### The workflow that connects them

```
1. Analyst: "this customer's summary looks wrong"
2. You open Postman → hit staging API by hand with tricky payloads
3. You find the trigger (say, a null field)
4. You write a pytest-httpx test that locks it in forever  ← the key step
```

**Postman discovers. pytest-httpx makes it permanent.**

A team with only pytest-httpx starts every investigation from zero. A team
with only Postman learns things that never stick.

**Bonus term:** **Newman** is Postman's command-line runner — so a Postman
collection *can* run in CI as a smoke test against live staging. That's
the bridge between the two worlds.

---

## SAY IT OUT LOUD

> *"I think about pipeline testing in layers. DAG integrity tests catch
> structural mistakes before anyone runs anything. Unit tests catch logic
> bugs in individual stages. But the most dangerous bugs in a scheduled
> pipeline aren't 'the function is wrong' — they're 'the function fails
> quietly and the pipeline keeps going.' So the layer I'd push hardest for
> is stage-boundary tests that run through the orchestrator with
> deliberately bad input and assert the whole run fails. I'd assert on run
> status rather than return value specifically, because that's the only
> assertion a developer can't satisfy by returning dummy data — it forces
> the failure to become visible.*
>
> *On Postman versus pytest-httpx, I see them as different jobs: Postman is
> what I reach for live while investigating a reported issue, and
> pytest-httpx is where that investigation becomes a permanent regression
> test so the same bug can't come back unnoticed."*

---

## CHECK YOURSELF

1. Which test layer(s) would have caught the `asset_criticality` bug, and
   which would **not**? Be honest about the misses.
2. You need to force that `try/except` to change. What exactly does your
   test assert on, and **why not** the return value?
3. A teammate says *"we don't need Postman, we have full pytest-httpx
   coverage."* What's the gap in that argument?

---

## GLOSSARY

| Term | Meaning |
|---|---|
| **Pipeline** | Steps that run in order, on a schedule, unattended |
| **DAG** | Directed Acyclic Graph — a flowchart of steps, no loops |
| **Orchestrator** | Tool that runs the DAG (Airflow, Dagster) |
| **Stage / task** | One step in the pipeline |
| **Silent failure** | Broke, but nothing crashed or alerted |
| **Dead-letter / quarantine** | Where bad records go instead of being dropped |
| **Golden fixture** | A saved known-good output you compare against |
| **`dg.materialize()`** | Dagster's "run these steps right now, in this test" |
| **Newman** | Postman's CLI runner, for using collections in CI |
