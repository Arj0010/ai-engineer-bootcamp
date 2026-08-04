# 1. Pipeline Testing: pytest, Airflow/Dagster, Postman

## The scenario

VulnPrioritize's nightly pipeline has three stages, orchestrated by a
DAG: `ingest_scan_results` pulls raw findings from customer scanners,
`score_risk` runs the ML model to rank them, and `draft_remediation`
calls an LLM to write the human-readable summary. This DAG runs for
every customer, every night, unattended.

Three weeks ago, `score_risk` started silently failing for one specific
customer segment — findings with a null `asset_criticality` field. The
task didn't crash. It caught the exception internally, logged a warning
nobody read, and passed an empty ranking downstream. `draft_remediation`
happily generated "no critical vulnerabilities found" summaries for a
customer that, it turned out, had several. Nobody caught it for eleven
days, until the customer got breached through exactly one of the
vulnerabilities that never made it onto their list.

That's the failure mode round 2 is really testing for: not "can you write
a pytest test," but "would your testing strategy have caught a pipeline
stage that fails *quietly* instead of loudly."

## Quick recap

You already know the mechanics: pytest discovers `test_*` functions and
fails on any exception; `pytest.raises` proves an exception happens on
purpose instead of hoping it does; fixtures (`conftest.py`) inject shared
setup like a fake `LLMClient` without duplicating it in every test; you
mock the LLM's HTTP call with `pytest-httpx` so tests never touch a real
network. Day 2's Dagster exercise took this one level up: `dg.materialize()`
proved that a broken data contract fails the *entire pipeline run*, not
just a silent bad value three stages downstream — which is exactly the
property that would have caught the `asset_criticality` bug above.

## Going deeper

**Unit tests are necessary but not sufficient for pipelines.** A unit
test on `score_risk()` in isolation, called directly with a clean
DataFrame, would have passed — the bug only exists in how the *orchestrated
stage* handles a specific malformed real-world input and what it does
with the failure. This is why the JD names Airflow/Dagster test utilities
separately from pytest: you need tests at the **task/stage boundary**,
not just the function boundary. Concretely: a test that runs
`score_risk` through the DAG framework with a deliberately malformed
upstream output and asserts the *run fails* (or routes to a dead-letter
path), not that the function returns some value. The `asset_criticality`
bug survived because the code had a broad `except Exception: log.warning(...)`
around the scoring logic — silent by design, just not on purpose. A test
that asserts "malformed input must fail the DAG run" would force that
`except` block to either re-raise or explicitly route to a quarantine
step, both of which are visible failures instead of an invisible one.

**DAG integrity tests are a distinct, cheap layer.** Before you even test
behavior, a lightweight pytest suite that imports every DAG module and
asserts: it parses without error, no cycles, every task has retries and a
timeout configured, no duplicate task IDs. This catches "someone's PR
breaks the DAG definition itself" before a human ever looks at it — cheap
insurance that's easy to describe even if you haven't built a full
Airflow environment.

**Postman vs. pytest-httpx — these solve different problems, not the same
one.** `pytest-httpx` is for permanent, automated, CI-enforced contracts:
"this internal scoring API must always return this shape." Postman earns
its place during **live investigation** — when an analyst reports "this
customer's remediation summary looks wrong," you don't want to write a
pytest test first; you want to hit the actual staging API by hand, with a
saved Postman collection of "known tricky customer payloads," poke at it,
and *then* codify what you found as a permanent pytest-httpx regression
test. Postman is exploratory and human-speed; pytest-httpx is the
permanent guardrail that comes out of that exploration. A team that only
has one of the two is missing something: pytest-httpx alone means every
investigation starts from zero; Postman alone means nothing you learn
sticks around to prevent a repeat.

**Regression testing a pipeline means golden fixtures, not golden
assertions.** For a stage like `score_risk`, hand-picking assert values
for every field doesn't scale. Instead: snapshot a known-good output for
a fixed, versioned input fixture (a "golden" customer scan), and on every
change, diff the new output against the snapshot. A diff isn't
automatically a failure — score changes are expected when the model
legitimately improves — but an *unexplained* diff is a prompt for a human
to look, which is a very different, more scalable posture than manually
maintaining hardcoded expected values per field.

## How to say this out loud

*"I think about pipeline testing in three layers: DAG integrity tests
that catch structural mistakes before anyone even runs the pipeline, unit
tests on individual stage logic with pytest and mocked dependencies via
pytest-httpx, and stage-boundary tests that run through the orchestrator
itself — because the most dangerous bugs in a scheduled pipeline aren't
'the function is wrong,' they're 'the function fails quietly and the
pipeline keeps going anyway.' I'd also draw a real distinction between
Postman and pytest-httpx: Postman is what I'd reach for live, investigating
a specific reported issue, and pytest-httpx is where that investigation's
findings become a permanent regression test so the same bug can't come
back unnoticed."*

## Check yourself

1. In the `asset_criticality` bug, exactly which test layer (unit, DAG
   integrity, stage-boundary, regression/golden-fixture) would have
   caught it, and which ones would NOT have — be honest about the ones
   that wouldn't.
2. Your `score_risk` stage now has a `try/except` that logs and continues
   on bad input, matching the real incident above. Design the specific
   test that would force that code to change. What does it assert on —
   the DAG run's overall status, or something else?
3. A teammate says "we don't need Postman, we have full pytest-httpx
   coverage." What's the gap in that argument?
