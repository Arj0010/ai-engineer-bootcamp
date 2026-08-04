# 2. Data Contracts & Schemas at the Ingest Boundary

## The scenario

VulnPrioritize ingests scan output from hundreds of customer
environments. Every one of those environments is, from your system's
point of view, **untrusted input** — different scanner versions, different
OS/package ecosystems, occasional customer-side misconfigurations, and
sometimes just garbage from a scanner that crashed mid-run. Six months
ago, a scanner vendor pushed an update that changed how it encoded CVSS
scores — from a float (`7.5`) to a string with a trailing descriptor
(`"7.5 HIGH"`). Nothing in the ingest path validated the type. The score
got silently coerced somewhere downstream, `7.5 HIGH` became `NaN`, and
`NaN` sorted to the *bottom* of the risk ranking instead of raising an
error. For one customer, for seseveral days, their highest-severity
findings were quietly deprioritized to the bottom of their remediation
list.

This is the data-contract story: in security specifically, a schema
violation isn't just "ugly data" — it's a **detection gap**, and detection
gaps are the worst possible failure mode for this kind of product.

## Quick recap

You already built the mechanics: a `pandera.DataFrameSchema` with typed
`Column`s and `Check`s (range, `isin`, uniqueness), validated with
`.validate(df, lazy=True)` to collect every violation instead of stopping
at the first, and you wired that schema into a Dagster asset so a broken
contract fails the whole materialized run instead of flowing downstream
silently — directly the property that would have caught the CVSS-encoding
bug.

## Going deeper

**Not every violation deserves the same response.** A blanket "validation
failed, reject the whole batch" policy sounds rigorous but is actually
naive for a system ingesting from hundreds of independent customer
environments: one customer's malformed batch shouldn't block processing
for the other 199. The real design question round 2 is probing is: which
violations are **hard failures** (block this record, maybe alert) vs.
**soft anomalies** (flag it, quarantine it, but don't crash the whole
customer's run)? A CVSS score outside `[0, 10]` is unambiguous — hard
fail, quarantine that record. A new, previously-unseen `scanner_version`
string is not inherently invalid — soft flag, let it through, alert if the
volume of "unknown but tolerated" values spikes.

**Schema evolution across untrusted producers is the actual hard
problem.** Pandera's `strict=False` lets unexpected columns through
un-validated; `strict=True` rejects anything you didn't explicitly
declare. For an ingest boundary fed by scanner vendors who ship changes
on their own schedule, `strict=True` sounds safer but will break your
pipeline the day a vendor adds a legitimately useful new field you
haven't accounted for yet — a classic false-positive-driven outage in
your *own* system, ironic for a security product. The more defensible
posture: strict validation on the fields you depend on for scoring
(type, range, required), permissive on everything else, plus a lightweight
alert on schema drift (new/renamed/removed columns) so a human notices
the change even though it doesn't hard-fail the pipeline.

**Custom validation rules encode domain knowledge Pandera's type system
can't.** "CVSS score must be a float in `[0, 10]`" is a Pandera `Check`.
"A finding marked `severity=critical` must have a non-null CVE reference"
is a **cross-field invariant** — Pandera supports this via
`Check` at the DataFrame level (not just per-column), but the *decision
about what counts as an invariant* is domain reasoning, not tooling
knowledge. In a security context, the invariants worth encoding are
usually the ones that map to a real detection-integrity risk: severity
consistent with supporting evidence, asset ID must exist in the
customer's known asset inventory (an orphaned finding referencing an
unknown asset is itself a signal something's wrong upstream), timestamp
not in the future (clock skew from a customer's scanner is a real,
recurring bug class).

**API compatibility across services is a schema problem at a different
boundary.** The JD calls this out separately from "Pandera + custom
rules" for a reason: Pandera validates *data at rest* (a DataFrame);
API compatibility is about *the contract between two services' request/
response shapes over time*. If `score_risk`'s output schema changes (a
field renamed, a type widened), every downstream consumer — the
`draft_remediation` stage, the customer-facing dashboard API — needs to
either tolerate it or the change needs to be caught before deploy. This
is where OpenAPI schema validation or a lightweight "does this response
still match the last known-good shape" contract test earns its place
alongside Pandera, not instead of it.

## How to say this out loud

*"I treat the ingest boundary as the highest-value place to invest in
data contracts, because in a security product, a data quality bug there
doesn't just produce bad output — it produces a detection gap, which is
the worst failure mode we have. I'd use Pandera for structural and
range validation with `lazy=True` so a bad batch surfaces every problem
at once, not one at a time. But I'd deliberately split violations into
hard failures that quarantine a record versus soft anomalies that get
flagged and monitored, because a rigid 'reject everything' policy on
data from hundreds of independent, untrusted producers will cause its
own outages. And I'd track schema drift — new or changed fields from a
scanner vendor — as a signal to review, even when it doesn't hard-fail,
because `strict=True` alone will eventually break on a legitimate upstream
change."*

## Check yourself

1. Design the specific Pandera `Check` (or cross-field check) that would
   have caught the CVSS `"7.5 HIGH"` bug, and say exactly where in the
   pipeline it should run — as early as possible, or after some
   normalization step? Defend the choice.
2. You're asked to make ingest validation `strict=True` "for safety."
   What's the concrete failure mode you'd warn the team about before
   agreeing?
3. A finding has `severity=critical` but a null CVE reference. Is that a
   hard fail or a soft anomaly in VulnPrioritize's context, and why does
   that answer depend on what happens to the record next (does it still
   reach an analyst, or does it get silently dropped)?
