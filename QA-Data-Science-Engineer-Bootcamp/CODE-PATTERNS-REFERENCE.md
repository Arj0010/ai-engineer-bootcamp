# CODE PATTERNS REFERENCE

Every tool in the JD, with annotated sample code. Goal: **recognize it on
sight, and be able to write pseudocode on a whiteboard.** You don't need
to memorize syntax — you need to know the shape.

---

# 1. DOCKER

## Dockerfile

```dockerfile
FROM python:3.11-slim                 # base image; -slim = smaller, fewer CVEs

WORKDIR /app                          # all later commands run here

# Copy deps FIRST, install, THEN copy code.
# Docker caches each layer — code changes don't re-trigger a pip install.
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .                              # now the source

ENV PYTHONUNBUFFERED=1                # logs appear immediately, not buffered

# Don't run as root — container-escape hardening
RUN useradd -m appuser
USER appuser

EXPOSE 8000                           # documentation; does NOT publish the port

# Container's health signal — orchestrators use this to decide "is it ready"
HEALTHCHECK --interval=30s --timeout=3s \
  CMD python -c "import httpx; httpx.get('http://localhost:8000/health')"

CMD ["uvicorn", "app:app", "--host", "0.0.0.0", "--port", "8000"]
```

**A test-runner image** is just a different `CMD`:
```dockerfile
CMD ["pytest", "-v", "--tb=short"]
```

### The QA points
- `COPY requirements.txt` before `COPY . .` → **layer caching**, huge CI speedup
- **Pinned versions** matter more in ML: a different NumPy can *silently change numeric output*, not just crash
- `--no-cache-dir` keeps the image small
- Non-root user = security hygiene

## docker-compose.yml — multi-service test environment

```yaml
version: "3.9"

services:
  inference-api:
    build: .
    ports: ["8000:8000"]              # host:container
    environment:
      - LLM_BASE_URL=http://mock-llm:9000   # points at the MOCK, not the real API
    depends_on:
      mock-llm:
        condition: service_healthy     # wait until healthy, not just started

  mock-llm:                            # the sidecar mock — pytest-httpx at infra level
    image: mockserver/mockserver:latest
    ports: ["9000:1080"]
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:1080/health"]
      interval: 5s
      retries: 5

  tests:
    build: .
    command: pytest -v                 # overrides the image's CMD
    depends_on: [inference-api]
    environment:
      - API_URL=http://inference-api:8000   # service NAME = hostname on the network
```

```bash
docker compose up --abort-on-container-exit --exit-code-from tests
```
That flag makes compose exit with the **test container's** exit code — so CI can gate on it.

## Kubernetes Job — the right primitive for tests

```yaml
apiVersion: batch/v1
kind: Job                              # Job, NOT Deployment — runs to completion, stops
metadata:
  name: ml-pipeline-tests
spec:
  backoffLimit: 2                      # retry twice on failure
  ttlSecondsAfterFinished: 3600        # auto-cleanup after 1h
  template:
    spec:
      restartPolicy: Never             # don't restart forever
      containers:
        - name: test-runner
          image: myregistry/vulnprioritize-tests:sha-abc123   # PINNED, not :latest
          command: ["pytest", "-v"]
          env:
            - name: LLM_BASE_URL
              value: "http://localhost:9000"   # the sidecar, same pod
          resources:
            requests: {memory: "512Mi", cpu: "500m"}
            limits:   {memory: "2Gi",   cpu: "2000m"}
            # exceed memory limit -> OOMKilled
            # exceed cpu limit    -> THROTTLED (slow, not killed)

        - name: mock-llm                        # SIDECAR — same pod, shares localhost
          image: mockserver/mockserver:latest
          ports: [{containerPort: 9000}]
```

**Say it:** *"A Job, not a Deployment, because tests run to completion. A sidecar mock in the same pod so tests hit localhost — that's `pytest-httpx` moved to the infrastructure layer. And resource limits matter: exceeding memory gets you OOMKilled, exceeding CPU gets you throttled, which is the usual cause of 'passes locally, times out in CI.'"*

---

# 2. CI/CD

## GitHub Actions

```yaml
name: ML CI

on:
  pull_request:
    paths: ["src/**", "tests/**"]      # only run when relevant files change
  schedule:
    - cron: "0 2 * * *"               # nightly at 2am — for slow suites

jobs:
  unit-and-schema:                     # jobs run in PARALLEL by default
    runs-on: ubuntu-latest
    steps:                             # steps run SEQUENTIALLY
      - uses: actions/checkout@v4      # `uses` = prebuilt action
      - uses: actions/setup-python@v5
        with: {python-version: "3.11"}
      - run: pip install -r requirements.txt    # `run` = shell command
      - run: pytest tests/unit tests/schema -v

  model-behavior-gate:                 # ML-SPECIFIC — the bit most teams miss
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: pytest tests/statistical --frozen-reference-set

  merge-gate:
    needs: [unit-and-schema, model-behavior-gate]   # waits for BOTH
    runs-on: ubuntu-latest
    steps:
      - run: echo "all required checks green"

# ⚠️ THIS FILE DOES NOT BLOCK MERGES.
# Settings -> Branches -> Require status checks -> select these job names.
# "CI ran" and "CI gated the merge" are two separate configurations.
```

## GitLab CI (same model, different syntax)

```yaml
stages: [test, gate]

unit-tests:
  stage: test
  image: python:3.11
  script:
    - pip install -r requirements.txt
    - pytest tests/unit -v
  rules:
    - changes: ["src/**/*", "tests/**/*"]

merge-gate:
  stage: gate
  needs: [unit-tests]
  script: echo "ok"
```
Merge blocking = **MR approval rules + pipeline status**, not the YAML.

## Jenkins (declarative)

```groovy
pipeline {
  agent { docker { image 'python:3.11' } }
  stages {
    stage('Test') {
      steps {
        sh 'pip install -r requirements.txt'
        sh 'pytest tests/ -v --junitxml=results.xml'
      }
    }
  }
  post { always { junit 'results.xml' } }
}
```
Merge blocking = branch-protection **plugins**.

> **All three are the same idea: a DAG of jobs triggered by an event, with a pass/fail gate. Only the syntax and the merge-block mechanism differ.**

---

# 3. PYTEST

```python
import pytest
import httpx

# ---------- conftest.py: shared fixtures ----------
@pytest.fixture
def llm_client():
    return LLMClient(base_url="https://mock-llm.local", api_key="test")

@pytest.fixture
def db_connection():
    conn = connect()          # SETUP
    yield conn                # test runs here
    conn.close()              # TEARDOWN — runs even if the test failed

@pytest.fixture(scope="session")     # created ONCE for the whole run
def heavy_model():
    return load_model("model.keras")


# ---------- basic ----------
def test_billing_keywords_classified_as_billing():
    result = classify("I need a refund on my invoice")
    assert result["label"] == "billing"


# ---------- exceptions ----------
def test_non_string_raises_type_error():
    with pytest.raises(TypeError):
        classify(123)

def test_error_message_is_useful():
    with pytest.raises(ValueError, match="exceeds max length"):   # regex on the message
        classify("a" * 10_000)


# ---------- parametrize: N separate tests ----------
@pytest.mark.parametrize("bad_input", [123, None, ["a"], {"k": "v"}])
def test_rejects_non_strings(bad_input):
    with pytest.raises(TypeError):
        classify(bad_input)

@pytest.mark.parametrize("text,expected", [
    ("I was charged twice",  "billing"),
    ("app crashes with 500", "technical"),
])
def test_labels(text, expected):
    assert classify(text)["label"] == expected


# ---------- mocking HTTP (mock the boundary you don't own) ----------
def test_low_confidence_falls_back_to_llm(llm_client, httpx_mock):
    httpx_mock.add_response(
        url="https://mock-llm.local/v1/complete",
        json={"completion": "technical"},
    )
    result = run_pipeline("something vague", llm_client)
    assert result["source"] == "llm_fallback"

def test_high_confidence_NEVER_calls_llm(llm_client, httpx_mock):
    # register NO mock -> any HTTP call raises -> passing proves zero calls
    result = run_pipeline("refund my invoice", llm_client)
    assert result["source"] == "rule_based"

def test_llm_error_propagates(llm_client, httpx_mock):
    httpx_mock.add_response(status_code=500)
    with pytest.raises(httpx.HTTPStatusError):
        run_pipeline("vague", llm_client)

def test_prompt_contains_original_text(llm_client, httpx_mock):
    httpx_mock.add_response(json={"completion": "technical"})
    run_pipeline("the thingamajig broke", llm_client)
    assert b"thingamajig" in httpx_mock.get_requests()[0].content   # SPY


# ---------- injecting time (never sleep in a test) ----------
class FakeClock:
    def __init__(self, values): self._v = iter(values)
    def __call__(self):        return next(self._v)

def test_slow_call_raises(llm_client, httpx_mock):
    httpx_mock.add_response(json={"completion": "ok"})
    client = InstrumentedLLMClient(llm_client, timeout_budget_s=2.0,
                                   clock=FakeClock([0.0, 5.0]))
    with pytest.raises(TimeoutBudgetExceeded):
        client.complete_with_metrics("hi")


# ---------- markers ----------
@pytest.mark.slow
def test_full_pipeline(): ...
# run with:  pytest -m "not slow"

@pytest.mark.skipif(not os.getenv("GPU"), reason="needs GPU")
def test_gpu_inference(): ...
```

```bash
pytest -v                 # verbose
pytest -k "billing"       # only tests matching a name
pytest -x                 # stop at first failure
pytest --tb=short         # shorter tracebacks
pytest -m "not slow"      # skip a marker
pytest --cov=src          # coverage
```

---

# 4. PANDERA

```python
import pandera.pandas as pa
from pandera.pandas import Check, Column, DataFrameSchema

scan_schema = DataFrameSchema(
    columns={
        "finding_id": Column(int, unique=True, checks=Check.ge(0)),
        "cvss_score": Column(float, checks=Check.in_range(0.0, 10.0)),
        "severity":   Column(str, checks=Check.isin(["low","medium","high","critical"])),
        "cve_id":     Column(str, nullable=True,
                             checks=Check.str_matches(r"^CVE-\d{4}-\d+$")),
        "scanned_at": Column("datetime64[ns]"),
    },
    checks=[   # DATAFRAME-level = cross-field invariants
        Check(lambda df: ~((df["severity"] == "critical") & df["cve_id"].isnull()),
              error="critical finding without a CVE reference"),
        Check(lambda df: df["scanned_at"] <= pd.Timestamp.now(),
              error="scan timestamp in the future (clock skew)"),
    ],
    strict=False,       # allow unknown columns (vendors add fields)
    coerce=True,        # try to cast to the declared dtype
)

# --- usage ---
df = scan_schema.validate(df)                 # fail-fast -> SchemaError   (PRODUCTION)
df = scan_schema.validate(df, lazy=True)      # collect-all -> SchemaErrors (CI)

# --- testing it ---
def test_broken_data_rejected():
    with pytest.raises(pa.errors.SchemaErrors) as exc:     # PLURAL for lazy
        scan_schema.validate(bad_df, lazy=True)
    failed = set(exc.value.failure_cases["column"])
    assert "cvss_score" in failed          # assert the SPECIFIC defect

# --- hard fail vs soft anomaly ---
CRITICAL_COLUMNS = {"cvss_score", "severity", "finding_id"}

def validate_customer_batch(df, customer_id):
    try:
        return scan_schema.validate(df, lazy=True)
    except pa.errors.SchemaErrors as e:
        hard = e.failure_cases[e.failure_cases["column"].isin(CRITICAL_COLUMNS)]
        if len(hard):
            quarantine(customer_id, hard)      # only THIS customer stops
            raise
        log.warning(f"soft anomalies for {customer_id}")
        return df                              # other 199 customers keep going
```

---

# 5. STATISTICAL CHECKS

```python
import numpy as np
import pandas as pd
from scipy import stats

def confidence_within_bounds(conf: np.ndarray) -> bool:
    return bool(np.all((conf >= 0.0) & (conf <= 1.0)))


def class_balance_check(labels: pd.Series, min_fraction=0.02) -> dict:
    fractions = labels.value_counts(normalize=True).to_dict()
    return {"balanced": all(f >= min_fraction for f in fractions.values()),
            "fractions": fractions}


def detect_drift(baseline, current, alpha=0.05, min_effect=0.1) -> dict:
    r = stats.ks_2samp(baseline, current)
    return {
        "drifted": bool(r.pvalue < alpha and r.statistic > min_effect),  # BOTH
        "p_value": float(r.pvalue),        # "is it real?"
        "statistic": float(r.statistic),   # "is it big?"
    }


def calculate_psi(baseline, current, bins=10) -> float:
    """No p-value, no sample-size trap. <0.1 stable, >0.25 act."""
    edges = np.histogram_bin_edges(baseline, bins=bins)
    b = np.histogram(baseline, bins=edges)[0] / len(baseline) + 1e-6
    c = np.histogram(current,  bins=edges)[0] / len(current)  + 1e-6
    return float(np.sum((c - b) * np.log(c / b)))


# --- per-class gate (NOT aggregate accuracy) ---
from sklearn.metrics import classification_report

def test_critical_recall_does_not_regress():
    rep = classification_report(y_true, y_pred, output_dict=True)
    assert rep["critical"]["recall"] >= 0.85, \
        f"critical recall {rep['critical']['recall']:.2f} — blocking regardless of accuracy"


# --- testing statistical code: SEED IT ---
def test_identical_distributions_no_drift():
    rng = np.random.default_rng(42)
    assert detect_drift(rng.normal(0.7, 0.1, 500),
                        rng.normal(0.7, 0.1, 500))["drifted"] is False
```

---

# 6. DAGSTER

```python
import dagster as dg
import pandas as pd

class ScanDataResource(dg.ConfigurableResource):
    path: str

@dg.asset
def raw_findings(scan_data: ScanDataResource) -> pd.DataFrame:
    return pd.read_csv(scan_data.path)

@dg.asset
def validated_findings(raw_findings: pd.DataFrame) -> pd.DataFrame:
    return scan_schema.validate(raw_findings, lazy=True)   # let it RAISE

@dg.asset
def risk_scores(validated_findings: pd.DataFrame) -> pd.DataFrame:
    required = ["asset_criticality", "cvss_score"]
    missing = validated_findings[required].isnull().any()
    if missing.any():
        raise ValueError(f"nulls in {missing[missing].index.tolist()}")   # LOUD
    return rank(model.predict(validated_findings))
```

```python
# --- tests: in-process, no scheduler, no containers ---
def test_good_data_pipeline_succeeds():
    result = dg.materialize(
        [raw_findings, validated_findings, risk_scores],
        resources={"scan_data": ScanDataResource(path="fixtures/good.csv")},
    )
    assert result.success is True
    assert len(result.output_for_node("risk_scores")) > 0

def test_bad_data_fails_the_whole_run():
    result = dg.materialize(
        [raw_findings, validated_findings, risk_scores],
        resources={"scan_data": ScanDataResource(path="fixtures/null_criticality.csv")},
        raise_on_error=False,          # capture instead of exploding
    )
    assert result.success is False     # ← the ONLY assertion that can't be gamed
```

---

# 7. AIRFLOW

```python
from airflow import DAG
from airflow.operators.python import PythonOperator
from datetime import datetime, timedelta

default_args = {
    "retries": 2,
    "retry_delay": timedelta(minutes=5),
    "execution_timeout": timedelta(hours=1),   # or it can hang forever
}

with DAG("vuln_nightly", start_date=datetime(2026,1,1),
         schedule="0 2 * * *", default_args=default_args, catchup=False) as dag:

    ingest = PythonOperator(task_id="ingest_scan_results", python_callable=ingest_fn)
    score  = PythonOperator(task_id="score_risk",          python_callable=score_fn)
    draft  = PythonOperator(task_id="draft_remediation",   python_callable=draft_fn)

    ingest >> score >> draft          # dependency: ">>" means "then"
```

```python
# --- DAG INTEGRITY TESTS: cheap, catch a broken PR before the scheduler does ---
from airflow.models import DagBag

def test_all_dags_import_without_error():
    dagbag = DagBag(include_examples=False)
    assert dagbag.import_errors == {}, f"DAG import errors: {dagbag.import_errors}"

def test_every_task_has_retries_and_timeout():
    for dag in DagBag(include_examples=False).dags.values():
        for task in dag.tasks:
            assert task.retries >= 1,                f"{task.task_id} has no retries"
            assert task.execution_timeout is not None, f"{task.task_id} can hang forever"

def test_no_duplicate_task_ids():
    for dag in DagBag(include_examples=False).dags.values():
        ids = [t.task_id for t in dag.tasks]
        assert len(ids) == len(set(ids))
```
```bash
airflow dags test vuln_nightly 2026-08-06     # run a full DAG in-process
```

---

# 8. LOCUST & k6

```python
from locust import HttpUser, task, between, LoadTestShape

class InferenceUser(HttpUser):
    wait_time = between(0.5, 2)         # think time; without it 10 users act like 1000

    def on_start(self):                  # runs once per simulated user
        self.client.headers = {"Authorization": "Bearer test"}

    @task(5)                             # weight — runs 5x as often
    def predict(self):
        with self.client.post("/predict", json={"text": "sample"},
                              catch_response=True) as r:
            if r.elapsed.total_seconds() > 2:
                r.failure("exceeded 2s SLA")     # slow == failed, by your definition

    @task(1)
    def health(self):
        self.client.get("/health")


class CVEDisclosureSpike(LoadTestShape):
    """baseline -> instant 40x spike -> plateau -> decay"""
    stages = [
        {"duration": 60,   "users": 10,  "spawn_rate": 5},     # baseline
        {"duration": 180,  "users": 400, "spawn_rate": 200},   # SPIKE (fast spawn)
        {"duration": 900,  "users": 400, "spawn_rate": 10},    # sustained plateau
        {"duration": 1200, "users": 10,  "spawn_rate": 10},    # decay
    ]
    def tick(self):
        t = self.get_run_time()
        for s in self.stages:
            if t < s["duration"]:
                return (s["users"], s["spawn_rate"])
        return None
```
```bash
locust -f locustfile.py --host http://localhost:8000              # web UI :8089
locust -f locustfile.py --headless -u 100 -r 50 --run-time 5m     # CI
```

```javascript
// k6 — pass/fail is IN THE SCRIPT, so `k6 run` is a CI gate by itself
import http from 'k6/http';
import { check } from 'k6';

export const options = {
  scenarios: {
    spike: { executor: 'ramping-vus', startVUs: 10,
             stages: [{duration:'10s',target:400}, {duration:'5m',target:400}] },
  },
  thresholds: {
    http_req_duration: ['p(95)<800'],   // exits non-zero if violated
    http_req_failed:   ['rate<0.01'],
  },
};

export default function () {
  const res = http.post('http://localhost:8000/predict',
                        JSON.stringify({text: 'sample'}),
                        {headers: {'Content-Type': 'application/json'}});
  check(res, {'status 200': (r) => r.status === 200});
}
```

---

# 9. PROMETHEUS

```python
from prometheus_client import Counter, Histogram, Gauge, start_http_server

# RED metrics
REQUESTS = Counter("predictions_total", "Total predictions", ["customer", "severity"])
LATENCY  = Histogram("prediction_latency_seconds", "Latency",
                     buckets=[.05,.1,.25,.5,1,2.5,5])
ERRORS   = Counter("prediction_errors_total", "Errors", ["error_type"])

# ML-SPECIFIC — RED stays green while the model is silently broken
CRITICAL_FINDINGS = Counter("critical_findings_total", "Criticals", ["customer"])
CONFIDENCE = Histogram("prediction_confidence", "Confidence dist",
                       buckets=[.1,.3,.5,.7,.9,1.0])
DRIFT_PSI  = Gauge("feature_drift_psi", "PSI vs baseline", ["feature"])
TOKEN_COST = Counter("llm_cost_usd_total", "LLM spend", ["model"])

@LATENCY.time()                                  # a DECORATOR that times the function
def predict(text, customer_id):
    result = model.predict(text)
    REQUESTS.labels(customer=customer_id, severity=result["severity"]).inc()
    CONFIDENCE.observe(result["confidence"])
    if result["severity"] == "critical":
        CRITICAL_FINDINGS.labels(customer=customer_id).inc()
    return result

start_http_server(8001)      # exposes /metrics — Prometheus PULLS from here
```

```yaml
# alert rules — ASYMMETRIC by design
groups:
  - name: ml_alerts
    rules:
      - alert: CriticalFindingsDroppedToZero        # the 11-day incident
        expr: sum by (customer) (rate(critical_findings_total[6h])) == 0
        for: 6h
        labels: {severity: page}                    # WAKE SOMEONE UP

      - alert: CriticalFindingsSpiked
        expr: sum by (customer) (rate(critical_findings_total[1h]))
              > 3 * sum by (customer) (rate(critical_findings_total[7d]))
        for: 2h
        labels: {severity: ticket}                  # look at it Monday

      - alert: HighLatencyP95
        expr: histogram_quantile(0.95, rate(prediction_latency_seconds_bucket[5m])) > 2
        for: 10m
```
> **Pushgateway** for short-lived batch jobs (a nightly Dagster job exits before a scrape).

---

# 10. LLM TESTING

```python
import json, re

# ---------- rule-based: cheap, deterministic, runs on every request ----------
def hallucination_check(source: str, output: str) -> dict:
    unsupported = sorted(set(re.findall(r"\b\d+\b", output)) -
                         set(re.findall(r"\b\d+\b", source)))
    return {"flagged": bool(unsupported), "unsupported_numbers": unsupported}

DANGEROUS = re.compile(r"\b(disable|remove|open|allow)\b.{0,30}\b(firewall|rule|block|acl)\b", re.I)

def policy_adherence_check(output: str) -> dict:
    """Policy != hallucination. Catches a DIRECTIONALLY INVERTED recommendation."""
    hits = DANGEROUS.findall(output)
    return {"flagged": bool(hits), "matches": hits}


# ---------- LLM judge ----------
JUDGE_PROMPT = (
    "Grade this remediation advice.\n"
    "Finding: {finding}\nAdvice: {advice}\n"
    'Reply with STRICT JSON only: {{"increases_exposure": true|false, "reasoning": "..."}}'
)

def llm_judge(finding, advice, llm_client) -> dict:
    raw = llm_client.complete(JUDGE_PROMPT.format(finding=finding, advice=advice))
    try:
        return json.loads(raw)
    except json.JSONDecodeError:
        # FAIL SAFE: can't verify -> block. (Fail OPEN would be for a low-stakes tool.)
        return {"increases_exposure": True, "reasoning": "judge output unparseable; failing safe"}


# ---------- cost regression gate ----------
def test_prompt_change_does_not_blow_up_cost(llm_client, httpx_mock):
    total = sum(InstrumentedLLMClient(llm_client).complete_with_metrics(p)["cost_usd"]
                for p in GOLDEN_PROMPTS)
    assert total <= BASELINE_COST * 1.10, \
        f"cost regression: {total:.4f} vs baseline {BASELINE_COST:.4f}"


# ---------- determinism against a FROZEN input ----------
def test_frozen_finding_produces_stable_output(llm_client, httpx_mock):
    httpx_mock.add_response(json={"completion": "Apply patch KB123."})
    out = draft_remediation(FROZEN_FINDING, llm_client)
    assert hashlib.sha256(out["summary"].encode()).hexdigest() == KNOWN_BASELINE_HASH
```

---

# 11. POSTMAN / NEWMAN

```json
{
  "info": {"name": "VulnPrioritize API"},
  "item": [{
    "name": "predict - valid finding",
    "request": {
      "method": "POST",
      "url": "{{base_url}}/predict",
      "body": {"mode": "raw", "raw": "{\"cvss_score\": 9.8}"}
    },
    "event": [{
      "listen": "test",
      "script": {"exec": [
        "pm.test('status 200', () => pm.response.to.have.status(200));",
        "pm.test('under 2s',  () => pm.expect(pm.response.responseTime).to.be.below(2000));",
        "pm.test('has severity', () => pm.expect(pm.response.json()).to.have.property('severity'));"
      ]}
    }]
  }]
}
```
```bash
newman run collection.json -e staging.json --reporters cli,junit
```
**Postman = live exploration. pytest-httpx = permanent guardrail. Newman = run collections in CI.**

---

# 12. PYTORCH (QA angle only)

```python
import torch

def test_model_output_contract():
    model = load_model()
    model.eval()
    x = torch.randn(4, 71)                        # batch of 4, seq len 71

    with torch.no_grad():
        out = model(x)

    assert out.shape == (4, 1),        f"expected (4,1), got {out.shape}"   # shape
    assert out.dtype == torch.float32                                        # dtype
    assert torch.all((out >= 0) & (out <= 1)), "sigmoid output outside [0,1]"
    assert not torch.isnan(out).any(),  "NaN in output"

def test_frozen_input_gives_stable_output():
    """A 'harmless' refactor shouldn't change numbers."""
    torch.manual_seed(42)
    out = load_model()(torch.load("fixtures/frozen_input.pt"))
    expected = torch.load("fixtures/frozen_output.pt")
    assert torch.allclose(out, expected, atol=1e-5)      # TOLERANCE, not exact

def test_batch_invariance():
    """Same input alone vs in a batch must give the same answer."""
    m, x = load_model(), torch.randn(1, 71)
    alone   = m(x)
    batched = m(torch.cat([x, torch.randn(3, 71)]))[0:1]
    assert torch.allclose(alone, batched, atol=1e-5)      # catches silent broadcasting bugs
```

---

# 13. LANGCHAIN (test YOUR logic, not the library)

```python
def draft_remediation(finding, llm_client, max_retries=3):
    # ── everything here except one line is YOUR code = YOUR tests ──
    if finding["severity"] not in ("high", "critical"):
        return None                                          # branch

    prompt = TEMPLATE.format(cve=finding["cve_id"])          # input construction

    for attempt in range(max_retries):                       # retry loop
        raw = llm_client.complete(prompt)                    # ← the only LLM line
        try:
            parsed = json.loads(raw); break                  # parsing
        except json.JSONDecodeError:
            if attempt == max_retries - 1:
                return {"summary": None, "error": "unparseable"}   # fallback

    if not parsed.get("steps"):
        return {"summary": None, "error": "empty steps"}     # output validation
    return parsed
```

| Test | Mock returns | Assert |
|---|---|---|
| skips low severity | *(none registered)* | LLM never called |
| happy path | valid JSON | correct output |
| retries then succeeds | garbage, garbage, valid | succeeds on 3rd |
| gives up | garbage × 3 | returns error, doesn't loop forever |
| empty steps | `{"steps": []}` | takes the error path |

```python
def test_agent_loop_is_bounded(llm_client, httpx_mock):
    for _ in range(20):
        httpx_mock.add_response(json={"completion": '{"tool":"search","args":{}}'})
    with pytest.raises(MaxIterationsExceeded):
        run_agent("loop forever", llm_client, max_iterations=5)
    assert len(httpx_mock.get_requests()) <= 5      # proves the cap FIRED
```

---

# PSEUDOCODE TEMPLATES (whiteboard-ready)

```
DATA CONTRACT TEST
  load fixture (good + deliberately broken)
  validate(good)                       -> expect no exception
  validate(broken, lazy=True)          -> expect SchemaErrors
  assert failure_cases contains THE SPECIFIC column

PIPELINE STAGE TEST
  materialize([stages], resources=bad_data, raise_on_error=False)
  assert result.success is False       # NOT the return value

MODEL BEHAVIOR GATE
  score frozen reference set with new model
  per-class report
  assert critical_recall >= threshold
  assert calibration_error < tolerance

LLM CORRECTNESS
  mock the HTTP response
  rule-based check first (cheap gate)
  judge second (nuance)
  handle unparseable judge -> fail safe or open (decide + defend)

LLM COST
  run golden prompt set
  sum estimated cost
  assert cost <= baseline * 1.10

LOAD TEST
  define shape (baseline -> spike -> plateau -> decay)
  hit ingest AND inference
  collect p95/p99 + CPU/memory
  assert graceful degradation: core path survives when the optional path times out

MONITORING
  expose /metrics (Prometheus pulls)
  RED + prediction distribution + drift + cost
  alert ASYMMETRICALLY (drop-to-zero pages; spike files a ticket)
```
