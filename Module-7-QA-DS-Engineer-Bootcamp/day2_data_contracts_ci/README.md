# Day 2 — Data Contracts, Schemas, CI/CD

**Time budget: ~3-4 focused hours.**

## The pipeline you're protecting

```mermaid
flowchart TD
    A[Raw prediction CSV] --> B["Pandera schema.validate()"]
    B -->|structural checks pass| C["Statistical checks: numpy + scipy"]
    B -->|fails| X1[Blocked: bad row-level data]
    C -->|distributions look sane| D[pytest suite green]
    C -->|drift / imbalance detected| X2[Blocked: bad aggregate data]
    D --> E[GitHub Actions CI]
    E -->|all jobs green| F[Merge allowed]
    E -->|any job red| X3[Merge blocked]
    B --> G["Dagster asset pipeline\nraw -> validated -> drift_report"]
    G -->|broken data| X4[Whole materialize() run fails]
```

Schemas catch row-level structural problems (wrong type, out-of-range
value, unknown category). Statistical checks catch problems that only show
up when you look at the *whole batch* (drift, imbalance). CI is what makes
both of these run automatically on every push, instead of "only when
someone remembers to."

## Setup

```bash
cd Module-7-QA-DS-Engineer-Bootcamp/day2_data_contracts_ci
pip install -r ../requirements-qa.txt
pytest -v      # starts red -- schema_todo.py and statistical_checks_todo.py are unimplemented
```

## Order of operations

| # | Exercise | Files | Concept |
|---|----------|-------|---------|
| 1 | A | `schemas/schema_todo.py`, `data/*.csv` | Pandera schema definition, `lazy=True` validation |
| 2 | B | `validation/statistical_checks_todo.py` | NumPy invariants + SciPy `ks_2samp` drift detection |
| 3 | — | `tests/test_data_contracts_todo.py` | tying both into pytest |
| 4 | C | `orchestration_testing/dagster_pipeline_todo.py` | orchestration-layer testing (Dagster `materialize`, Airflow concept comparison) |
| 5 | — | `.github/workflows/qa-ds-bootcamp-ci.yml` (repo root) | reading a real ML CI pipeline |
| 6 | — | `docker_k8s_concepts.md` | conceptual only, read don't build |

`data/broken_predictions.csv` has 6 distinct planted defects — before you
open `schema_todo.py`, open the CSV and try to spot all of them yourself.
(Answer key: out-of-range confidence twice — too high and too low — an
unknown category, a non-integer id, a missing timestamp, a missing label,
a string where a float belongs, and a duplicate id.)

## About the CI workflow

`.github/workflows/qa-ds-bootcamp-ci.yml` at the repo root runs Day 1's and
Day 2's pytest suites on every push/PR that touches this module. **It will
show red (failing) right now, and that's intentional** — the exercises
aren't implemented yet. As you complete Day 1 and Day 2 exercises and push,
watch the jobs go from red to green. This is a genuine, if small-scale,
demonstration of a CI gate: **the workflow running is not the same thing
as it *blocking a merge***. GitHub Actions will happily report a red X
without stopping anyone from clicking the merge button — the actual block
comes from **branch protection rules** in repo Settings → Branches, where
you mark specific check names (e.g. `day1-pipeline-tests`,
`day2-data-contract-tests`) as **required status checks**. Be ready to
explain that distinction explicitly — "CI ran" and "CI gated the merge"
are two different configuration steps, and mixing them up is a common
mistake to catch in a QA-focused interview.

## Checkpoint quiz

1. Name one bug that a Pandera schema check WOULD catch and a statistical
   check WOULD NOT, and vice versa.
2. `detect_confidence_drift` used `alpha=0.05`. If this ran on every commit
   in a busy repo, roughly how often would you expect a false-positive
   drift alert purely by chance? Is that a problem, and if so what's one
   mitigation?
3. What's a "required status check" and why is a green CI workflow alone
   not sufficient to guarantee a merge is blocked?
4. `class_balance_check`'s `min_fraction` default is 0.02. What real-world
   change to the underlying data (not the code) would cause this check to
   start failing? Is that failure always a bug?
5. `dg.materialize(...)` runs your asset pipeline in-process for a test.
   What's the equivalent Airflow concept, and why did we pick Dagster's
   API to build hands-on instead of Airflow's in a 3-day window?
6. Why should `validated_predictions` fail the ENTIRE Dagster run instead
   of, say, logging a warning and passing an empty DataFrame downstream to
   `drift_report`? What would the second approach hide?

## Reality check

- **Realistic hands-on in 3 days:** Pandera schema basics, NumPy/SciPy
  invariant checks, reading and adapting a GitHub Actions YAML file,
  Dagster asset testing via `dg.materialize`.
- **Not realistic hands-on in 3 days:** setting up Kubernetes-based test
  infrastructure, a full statistical-monitoring service, branch-protection
  administration across a real org's multiple repos, a real Airflow
  environment (metastore + scheduler) to run `airflow dags test` against.
  Read `docker_k8s_concepts.md` and `orchestration_testing/README.md`,
  understand the diagrams, and be ready to talk through the tradeoffs —
  that's the appropriate depth here.
