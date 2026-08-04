# 7. Observability: Catching Silent Model Failure

## The scenario

For eleven days (the same incident from Module 1), every dashboard that
mattered stayed green. The `score_risk` service's uptime was 100%. Its
API latency was normal. Its error rate was near zero. Every RED metric
(rate, errors, duration) said the system was healthy — because from the
service's point of view, it *was* healthy: it received requests and
returned 200s the entire time. What it returned was quietly, catastrophically
wrong for one customer, and nothing in the monitoring stack was watching
for that, because nothing in the monitoring stack was looking at *model
output*, only at *service health*.

This is the single most important idea in observability for ML/security
systems: **the failure that matters most doesn't look like a failure to
standard infrastructure monitoring.** Round 1 established you know
Prometheus scrapes and Grafana visualizes. Round 2 is testing whether you
know what to actually put on the dashboard.

## Quick recap

You already know the shape: Prometheus pulls metrics from a `/metrics`
endpoint on an interval, Grafana queries and visualizes them, Alertmanager
routes rule violations to a human. You know the RED method (rate, errors,
duration, always percentiles not just averages) as the baseline for any
service, and you know ML-specific signals exist on top of that: prediction
distribution histograms, drift metrics, token cost, and rolling
LLM-judge scores as a live production signal instead of only a CI-time
check.

## Going deeper

**The dashboard that would have caught the eleven-day incident is a
per-customer prediction-volume metric, not a service-health metric.**
Concretely: "count of findings scored as critical, per customer, per
day" — a metric that's almost embarrassingly simple compared to
sophisticated drift statistics, and would have shown one customer's line
drop to zero and stay there for eleven days, next to every other
customer's normal fluctuation. The lesson generalizes: the most valuable
ML observability metric is often not a clever statistical one, it's the
simplest possible "count of the thing the product exists to produce,"
sliced by the dimension (customer, region, finding category) where a
localized failure would otherwise average out and disappear into a
healthy-looking aggregate.

**Alerting thresholds need to be asymmetric, matching the asymmetric
cost of the two failure directions.** A drift alert tuned the same way
in both directions — "alert if critical-findings volume moves more than
X% from baseline, up or down" — misses the point. A spike upward in
critical findings might just mean a bad week for the internet (a new
mass exploit campaign) and is lower-urgency to page on immediately. A
*drop* toward zero, especially for one customer while others stay normal,
is far more likely to be a silent failure and deserves a much more
sensitive threshold and a faster page. Good ML alerting design in this
domain means thinking explicitly about which direction of an anomaly is
actually dangerous, instead of applying a symmetric statistical threshold
because it's mathematically convenient.

**Alert fatigue on the QA/ops side is the same problem the product
causes for customers, and it's worth naming that connection explicitly.**
If every minor statistical fluctuation pages someone, real signals get
lost in noise and people start ignoring pages — the exact alert-fatigue
failure mode VulnPrioritize exists to prevent for its customers, now
happening internally to the team that's supposed to be watching the
system. This argues for tiered severity (a Slack notification for "worth
a look Monday" vs. a page for "wake someone up now") and for tuning
alerts against the frozen reference-set idea from Module 3 where
possible, rather than against noisy live data, for the same reason
determinism testing needed a frozen input in Module 6.

**Cloud-native observability tools matter less than the underlying
question "what would I actually want to know, and how fast."** Whether
it's self-hosted Prometheus/Grafana, a cloud provider's native monitoring
(CloudWatch, Cloud Monitoring), or a SaaS observability platform, the
tool is secondary — what round 2 is actually listening for is whether you
can specify, unprompted, the *metrics that matter for this specific
product* (per-customer prediction volume, confidence-distribution
percentiles, token cost per call, judge-score rolling average) rather
than a generic "we'd use Prometheus and Grafana" answer that could apply
to literally any service and demonstrates no domain reasoning about
security analytics specifically.

## How to say this out loud

*"Standard RED metrics — rate, errors, duration — tell you the service is
up, but they can't tell you the model is right, and the failure mode I'd
worry about most in a security product is exactly the gap between those
two: a service that stays perfectly healthy by every infrastructure
metric while quietly producing wrong output for one customer. So I'd push
for simple, per-customer, per-category volume metrics — not just
sophisticated drift statistics — because a localized failure that would
disappear into a healthy global average is often the most dangerous kind,
and the simplest metric is usually what catches it. I'd also design
alerting asymmetrically: a drop in critical-finding volume deserves a
much more sensitive, faster-paging threshold than a rise, because in this
domain a silent drop-to-zero is far more likely to be a bug than a
legitimate spike is. And I'd be deliberate about alert fatigue on our own
side, because a team that starts ignoring noisy pages has the exact same
problem the product is supposed to prevent for customers."*

## Check yourself

1. Design the specific metric and alert threshold that would have caught
   the eleven-day incident within, say, 24 hours instead of 11 days. Be
   precise about what it's measuring and what triggers the alert.
2. Explain why a symmetric "alert on any large deviation, either
   direction" threshold is the wrong design for critical-finding volume
   specifically. Give the concrete scenario where each direction of
   deviation means something different.
3. Your team says "we already have Prometheus/Grafana set up." What's
   the follow-up question that actually tells you whether their
   observability strategy would catch a VulnPrioritize-shaped silent
   failure, versus just confirms the tooling exists?
