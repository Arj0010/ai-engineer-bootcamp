# 4. Performance Testing Under Attack-Shaped Load

## The scenario

VulnPrioritize's load tests, run quarterly, always looked great: smooth
ramp from 0 to 500 requests/second over 5 minutes, p95 latency comfortably
under budget the whole way. Then a major CVE went public — the kind of
event that makes every customer's automated scanner kick off an
out-of-cycle scan simultaneously, within the same hour, because everyone's
patch-management tooling reacts to the same public disclosure at once.
Ingest volume didn't ramp smoothly; it went from baseline to 40x baseline
in about six minutes and stayed there for a day. The scoring service,
never tested against anything but smooth ramps, fell over — not
gracefully, just stopped responding, during the exact multi-hour window
when every customer most needed accurate prioritization.

Round 1 tested "can you run Locust and read p95." Round 2 tests "does
your load test's *traffic shape* actually resemble how this system fails
in reality" — and for a security product, the answer is almost never
"smooth ramp."

## Quick recap

You already built and ran a Locust load test against a mock inference
endpoint with `wait_time = between(...)` and a weighted task mix, and
read p95 latency and requests/sec off real output. You also read the k6
reference script and know it bakes pass/fail thresholds
(`http_req_duration`, `http_req_failed`) directly into the script instead
of a human reading a report — relevant for wiring load tests into CI as
a hard gate rather than a manual check.

## Going deeper

**Traffic shape is the actual variable that matters, not just peak
volume.** A smooth ramp to 500 req/s and an instant step-function jump to
500 req/s can produce wildly different failure behavior even at identical
peak load — the smooth ramp gives connection pools, autoscalers, and
caches time to warm up; a step function doesn't. Locust supports this
directly: instead of a flat `between()` wait time and a fixed user count,
a `LoadTestShape` class lets you script exactly this kind of
"instant spike, sustained plateau, gradual decay" pattern — the CVE-disclosure
shape from the scenario above — instead of only ever testing the easy
case.

**Load testing the inference path alone misses where security systems
actually buckle first: ingest.** Everyone reflexively load-tests the
API/inference endpoint because that's what Locust's HTTP-user model makes
easy. But in VulnPrioritize, the CVE-disclosure spike hits *ingest*
first — thousands of scanners uploading results simultaneously — and
ingest volume, not query volume, is what actually took the system down.
A load-testing strategy for this class of product has to include a
custom load generator (not necessarily Locust's HTTP-user pattern) that
simulates bulk, bursty *write* traffic into the pipeline, not just read
traffic against a scoring API.

**Resource-usage monitoring during the test is what turns "it got slow"
into "here's why."** Latency and throughput numbers alone tell you *that*
something degraded, not *what* degraded. Correlating a Locust run with
CPU/memory/connection-pool metrics (via `docker stats`, cAdvisor, or
whatever the deployment target exposes) is what lets you say "throughput
flattened at 300 req/s because the database connection pool saturated,"
which is actionable, instead of "it got slow around 300 req/s," which
isn't. This is the piece a load test run in isolation, without any
resource telemetry attached, structurally cannot give you.

**Graceful degradation is itself a testable requirement, and it's a QA
responsibility to define what "graceful" means before an incident forces
the definition on you.** When the LLM-summarization stage falls behind
during a load spike, what should happen — should scoring (the safety-critical
half) degrade along with it, or should the system serve prioritized
findings *without* the LLM summary rather than serve nothing at all? That's
a product decision, but it's QA's job to turn it into a testable
assertion: "under sustained overload, `score_risk` output must still be
served even if `draft_remediation` times out" — and then actually build
a load test scenario that forces that exact condition (e.g., artificially
slow the LLM dependency mid-test) to prove the fallback path really
works, not just that it exists in the code.

## How to say this out loud

*"For a security product specifically, I don't think smooth-ramp load
tests tell you much, because the traffic patterns that actually break
these systems are event-driven spikes — a major CVE drops and every
customer's scanner fires at once. I'd build load test scenarios that
match that shape: instant jumps, sustained plateaus, not just gradual
ramps, and I'd load-test the ingest path specifically, not only the
inference API, because in a system like this the write side is often
where the real volume hits first. I'd also always correlate a load test
against resource metrics, not just latency and throughput, because
knowing the system got slow at 300 req/s is much less useful than
knowing it got slow because a connection pool saturated. And I'd treat
graceful degradation as something to explicitly load-test, not assume —
if the LLM summarization path is supposed to degrade independently of the
core risk-scoring path under overload, I want a test that actually forces
that condition and proves the fallback holds, not just code that claims
to handle it."*

## Check yourself

1. Design a Locust `LoadTestShape` (in words, not code) that reproduces
   the CVE-disclosure spike from the scenario. What three parameters
   define the shape, and why does each one matter?
2. Why would a load test that only hits the scoring API, and never the
   ingest path, miss the actual failure mode that took the system down?
3. You're told "the LLM summarization service degrades gracefully under
   load." What's the specific load test you'd design to verify that
   claim is actually true, rather than just documented?
