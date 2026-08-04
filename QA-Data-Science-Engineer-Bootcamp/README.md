# QA Data Science Engineer — 3-Day Interview Bootcamp

You're an experienced backend/AI engineer (Node.js, Python, LLM
orchestration, multi-agent systems) with **zero formal QA/testing
background** — no pytest, Pandera, Locust/k6, LangSmith, or CI-integrated
ML testing. This module is a 3-day, hands-on crash course built to close
that gap for a specific interview, not a general testing course.

**How this module works:** everything here is scaffolding and TODO
exercises, not finished code. You write the tests; this module gives you
the pipeline to test against, the exact task list, sample data with
planted bugs, and (in each day's `solutions/` folder) a reference
implementation to check against *after* you've attempted it yourself. The
Socratic part — quizzes, code critique, and the Day 3 mock interview —
happens by continuing the conversation with the assistant that built this,
using the checkpoint questions in each day's README as the script.

## The 3-day roadmap

```mermaid
flowchart TD
    subgraph Day1["Day 1 — LLM & Pipeline Testing Fundamentals"]
        D1A[pytest fundamentals\nunit tests, edge cases, parametrize]
        D1B[pytest-httpx\nmocking the LLM's HTTP call]
        D1C[LangSmith concepts\nmock regression + determinism check]
        D1D[LLM-as-judge\nrule-based + LLM hallucination/toxicity check]
        D1E[Cost/token/timeout monitoring\ndependency-injected clock]
        D1A --> D1B --> D1C --> D1D --> D1E
    end

    subgraph Day2["Day 2 — Data Contracts, Schemas, CI/CD"]
        D2A[Pandera schema\nvalidate good vs broken CSV]
        D2B[NumPy/SciPy checks\nbounds, class balance, drift]
        D2C[Dagster asset testing\nmaterialize good vs broken run]
        D2D[GitHub Actions CI\nreal workflow, red to green]
        D2E[Docker/K8s\nconceptual only]
        D2A --> D2B --> D2C --> D2D --> D2E
    end

    subgraph Day3["Day 3 — Performance, Observability, Mock Interview"]
        D3A[Locust load test\nrun it, read p95/throughput]
        D3B[Prometheus/Grafana\nconceptual only]
        D3C[Mock technical interview\nreframe YOUR projects as QA answers]
        D3A --> D3B --> D3C
    end

    subgraph Round2["Round 2 — Advanced Scenarios (Qualys)"]
        R2[VulnPrioritize scenario walkthrough\nall 8 JD themes, one running story]
    end

    Day1 --> Day2 --> Day3 --> Round2
```

## Day-by-day schedule (fits in a few focused hours each)

| Day | Focus | Time | Folder |
|-----|-------|------|--------|
| 1 | LLM & pipeline testing fundamentals | ~3-4h | [`day1_llm_pipeline_testing/`](./day1_llm_pipeline_testing/) |
| 2 | Data contracts, schemas, CI/CD | ~3-4h | [`day2_data_contracts_ci/`](./day2_data_contracts_ci/) |
| 3 | Performance, observability, mock interview | ~3-4h | [`day3_perf_observability_interview/`](./day3_perf_observability_interview/) |

## Setup (once, before Day 1)

```bash
cd QA-Data-Science-Engineer-Bootcamp
pip install -r requirements-qa.txt
```

Python 3.11+ assumed (matches this environment). Each day also has its own
`pytest.ini` so you can `cd` into that day's folder and just run `pytest`.

## Directory map

```
QA-Data-Science-Engineer-Bootcamp/
├── requirements-qa.txt
├── day1_llm_pipeline_testing/
│   ├── pipeline/            # the stub mock-LLM + rule-based classifier you test against
│   ├── tests/               # TODO test stubs -- your work
│   ├── langsmith_mock/      # LangSmith concept + local mock exercise
│   ├── llm_judge/           # rule-based + LLM-as-judge exercise
│   ├── genai_ops_monitoring/ # token/cost estimation + timeout-budget enforcement exercise
│   └── solutions/           # reference solutions -- attempt first!
├── day2_data_contracts_ci/
│   ├── data/                # good_predictions.csv + broken_predictions.csv (6 planted defects)
│   ├── schemas/              # Pandera schema TODO
│   ├── validation/           # NumPy/SciPy statistical checks TODO
│   ├── orchestration_testing/ # Dagster asset-pipeline testing exercise (+ Airflow concept notes)
│   ├── tests/                # tying it together
│   ├── docker_k8s_concepts.md
│   └── solutions/
├── day3_perf_observability_interview/
│   ├── mock_service/         # FastAPI mock inference endpoint
│   ├── load_test/            # Locust TODO exercise + k6 reference script
│   ├── observability_concepts/
│   ├── mock_interview/        # question bank for the live mock interview
│   └── solutions/
└── round2_advanced_scenarios/  # advanced, Qualys-specific scenario deep dives (round 2 prep)

.github/workflows/qa-ds-bootcamp-ci.yml   # real CI, scoped to this module, runs Day 1+2 tests
```

## Reality check — what's genuinely learnable hands-on in 3 days vs. talk-fluently-only

This is flagged per-day in each README, collected here for a fast pre-interview skim:

**Build hands-on (this module gets you there):**
- pytest fundamentals: assertions, fixtures, `parametrize`, `pytest.raises`
- Mocking HTTP-based LLM calls with `pytest-httpx`
- The *shape* of LangSmith's regression/determinism workflow (via a local mock)
- Rule-based + LLM-as-judge hallucination/toxicity checks
- Token usage estimation, cost calculation, and timeout-budget enforcement for GenAI calls (Day 1, Exercise E)
- Pandera schema definition and validation (including `lazy=True`)
- NumPy/SciPy statistical invariants (bounds, class balance, KS-test drift)
- Orchestration-layer testing with Dagster's `materialize()` (Day 2, Exercise C)
- Reading and adapting a real GitHub Actions ML CI workflow
- Running Locust against a mock service and correctly reading p95/throughput output

**Know conceptually, don't attempt hands-on (time-boxed out on purpose):**
- Real LangSmith account/dashboard setup — the concepts transfer directly, the UI doesn't need touching
- Fine-tuning or seriously prompt-engineering a production-grade judge model
- A real tokenizer (`tiktoken`) instead of Day 1 Exercise E's chars-per-token approximation
- Kubernetes-based test infrastructure (Jobs, sidecars, ephemeral namespaces) — `day2_data_contracts_ci/docker_k8s_concepts.md` covers what to say
- Airflow's actual test harness (`airflow dags test`, a real metastore) — `day2_data_contracts_ci/orchestration_testing/README.md` covers the concept and the lighter "DAG integrity test" pattern to describe instead
- GitLab CI / Jenkins specifically — concepts transfer directly from the GitHub Actions workflow you have working
- Postman — a manual/exploratory API tool; `pytest-httpx` covers the automated-testing angle the JD actually cares about
- k6 (read the reference script, understand it vs. Locust, don't install it)
- Standing up real Prometheus + Grafana + Alertmanager, or real CPU/memory resource-usage monitoring under load (Locust gives you latency/throughput, not resource metrics)
- Branch-protection administration across a real multi-repo org
- "Policy adherence" checks as a category distinct from hallucination/toxicity (same rule-based + LLM-judge pattern from Day 1 Exercise D applies — just a different rubric)

Being explicit about this split in the actual interview — "I built X hands-on
in my prep, and I understand Y conceptually but haven't set it up myself" —
reads as more credible than implying uniform depth across everything.

## Round 2 — Advanced Scenarios (for the follow-up round)

Passed round 1 and have a more advanced round coming up? See
[`round2_advanced_scenarios/`](./round2_advanced_scenarios/). It's written
specifically for **Qualys's QA Data Science Engineer role** — every JD
line item retold as a chapter in one running, illustrative security-analytics
scenario ("VulnPrioritize"), so the tools stop being disconnected trivia
and start being decisions you'd actually make, in order, on one system.
Each module follows: scenario → how it connects to what you already built
in Days 1-3 → the advanced reasoning a follow-up round is actually
testing for → a model spoken answer → check-yourself questions with no
answer key.

This is deliberately a reasoning/fluency document, not new hands-on
exercises — round 2 tests judgment under a concrete scenario more than it
tests whether you can type another pytest test.

## After you finish all 3 days

Come back to the conversation and ask to run the Day 3 mock interview using
your own real projects — that's the part of this bootcamp that isn't a
file in this repo, it's a conversation. If you have a follow-up round
coming up, work through `round2_advanced_scenarios/` the same way, then
bring your answers to the check-yourself questions back to the
conversation for critique.
