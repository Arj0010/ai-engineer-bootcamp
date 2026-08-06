# Round 2 — Advanced Scenarios for a Security Analytics QA Role

You passed round 1. Round 2 goes deeper and expects you to reason like
someone who's actually going to own QA for an ML/GenAI security product —
not just recite tool names. This section is written for **Qualys's QA Data
Science Engineer role** specifically: a company whose core business is
vulnerability management and risk-based security analytics.

## The running scenario: VulnPrioritize

Every module below is a chapter in the same story, so the tools stop being
disconnected trivia and start being decisions you'd actually make, in
order, on one system. **VulnPrioritize is a hypothetical, illustrative
system** — modeled on the general problem space Qualys operates in
(vulnerability scanning, risk-based prioritization, remediation guidance),
**not a claim about Qualys's real internal architecture**, which this
bootcamp has no visibility into. Use it to reason about the *shape* of the
problems, then translate that reasoning into your own words in the
interview.

> **VulnPrioritize**, in one paragraph: customer environments run
> vulnerability scans (open ports, outdated packages, misconfigurations).
> A traditional ML model scores each finding's real-world risk (not just
> CVSS severity — also asset criticality, exposure, exploit activity in
> the wild) and ranks a prioritized remediation list. A GenAI layer then
> drafts a plain-English executive summary and remediation steps for each
> high-priority finding, for security teams who don't have time to read
> raw scan output. Both layers ingest data from hundreds of customer
> environments, run continuously, and have to be right — a missed
> critical vulnerability is a customer getting breached; a wrong or
> hallucinated remediation instruction is a customer either wasting
> engineering time or, worse, disabling a control they shouldn't have.

That asymmetry — a security product's mistakes have real consequences in
both directions (missed threats vs. false alarms vs. bad guidance) — is
the thread that ties every module together. Round 1 tested "do you know
what Pandera is." Round 2 tests "given THIS system, why does that matter,
and what would you actually check."

## How to use this section

Each file below follows the same shape:
1. **The scenario** — a concrete situation inside VulnPrioritize
2. **Quick recap** — one paragraph tying it back to what you already
   built hands-on in Day 1-3
3. **Going deeper** — the advanced reasoning round 2 is actually testing
4. **How to say this out loud** — a model spoken answer, first person,
   short enough to actually say in an interview without rambling
5. **Check yourself** — questions with no answer key; if you can't answer
   them cleanly, that's your signal for what to re-read before Wednesday

## Start here

**[→ ROADMAP.md](./ROADMAP.md)** — your level assessment, the 2-day plan,
and what to drill. Read this before anything else.

## Index

| # | File | JD line(s) covered |
|---|------|---------------------|
| **0** | **[Python fundamentals for testing](./00_python_fundamentals_for_testing.md)** ⭐ | Prerequisite for everything — decorators, `with`, `yield`, `__call__`, `*args`. Read first. |
| 1 | [Pipeline testing: pytest, Airflow/Dagster, Postman](./01_pipeline_testing_pytest_airflow_dagster_postman.md) | "Functional and regression testing of ML pipelines using pytest and Airflow/Dagster test utilities and API testing tools" |
| 2 | [Data contracts & schemas at the ingest boundary](./02_data_contracts_schemas_pandera.md) | "Validate data contracts, schemas, and API compatibility across services using Pandera, and custom validation rules" |
| 3 | [Model behavior: statistical validation](./03_model_behavior_statistical_validation.md) | "Model behavior validation (input/output ranges, invariants, edge cases) using NumPy, SciPy, and statistical assertions" |
| 4 | [Performance testing under attack-shaped load](./04_performance_load_testing.md) | "Runtime and performance testing for inference latency, throughput, and resource usage using Locust, k6" |
| 5 | [CI/CD and containerized ML deploys](./05_cicd_containerized_workflows.md) | "Integrate ML-specific tests into CI/CD pipelines... alongside containerized workflows (Docker, Kubernetes)" |
| 6 | [LLM testing: LangSmith, judges, cost/token/timeout](./06_llm_testing_langsmith_judge_cost.md) | "Implement LLM-specific testing..." (all three JD sub-bullets) |
| 7 | [Observability: catching silent model failure](./07_observability_monitoring_alerting.md) | "Verify logging, monitoring, and alerting for ML services using Prometheus, Grafana, and cloud-native observability tools" |
| 8 | [Knowing the GenAI stack: PyTorch, LangChain, vLLM](./08_genai_stack_pytorch_langchain_vllm.md) | "Familiarity and experience of GenAI applications and tools" |
| **9** | **[Statistics from zero](./09_statistics_from_zero.md)** ⭐ | Underpins "statistical assertions" in module 3 — taught from first principles, no prior stats assumed |

**Suggested order:** module **0** first (it unlocks the rest), then 1-8 in
sequence — the VulnPrioritize story builds across them — and module **9**
last, as the deep dive behind module 3's statistical checks.

Before the interview, skim the "How to say this out loud" sections back to
back as your final review pass.
