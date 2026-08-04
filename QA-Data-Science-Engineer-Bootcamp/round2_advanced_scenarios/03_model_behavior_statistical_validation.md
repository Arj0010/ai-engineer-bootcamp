# 3. Model Behavior: Statistical Validation

## The scenario

VulnPrioritize's risk model gets retrained monthly on fresh exploit
intelligence. After last month's retrain, the model's overall accuracy
on the held-out test set actually *improved* — everyone was ready to
ship it. But a QA engineer noticed the *distribution* of risk scores had
shifted: the model was now assigning "critical" far less often across
the board, and "medium" far more. Aggregate accuracy looked fine because
most findings genuinely are medium-severity, so getting the majority
class right dominates the accuracy number. But recall on the *critical*
class — the class that matters most — had quietly dropped. A single
accuracy number hid a real regression in exactly the failure mode this
product exists to prevent.

This is what "model behavior validation" actually means at this level:
not "is the model accurate," but "does the model's behavior, examined at
the distribution level and the invariant level, still make sense — and
can you tell a real regression apart from the world (attack landscape)
legitimately changing?"

## Quick recap

You already built the primitives: `confidence_within_bounds` (a NumPy
range invariant), `class_balance_check` (fraction-per-class, catching
silent collapse toward one label), and `detect_confidence_drift` using
`scipy.stats.ks_2samp` to compare two distributions and flag when
`p_value < alpha`. Those three functions, individually simple, are
exactly the tools that would have caught the critical-recall regression
above — if they'd been run per-class instead of only in aggregate.

## Going deeper

**Aggregate metrics hide the failure mode that matters most in security.**
This is the single most important idea in this module. Precision/recall/
F1 computed across all classes together will always be dominated by
whichever class is most common — and in vulnerability data, "medium
severity, no immediate action needed" vastly outnumbers "critical, act
now." A QA strategy that only checks aggregate accuracy is optimizing for
(and will only alert on) the boring, common case, while being blind to
regressions in the rare, high-stakes case. The fix isn't a different
metric — it's **slicing**: run your statistical checks (class balance,
confidence distribution, drift) per-class, especially for the classes
where a miss is expensive, not just globally.

**Distinguishing "the model broke" from "the world changed" is a genuine
judgment call, not something a single test can resolve alone.** A KS-test
flagging drift between last month's confidence distribution and this
month's tells you *something changed* — it does not tell you whether that
change is a bug or a legitimate shift (e.g., a new large-scale exploit
campaign genuinely made more findings critical this month). What you can
build as QA: a **held-out reference set that doesn't change** — a fixed,
versioned batch of findings with known-correct labels, re-scored every
retrain. Drift against a live, ever-changing production distribution is
ambiguous; drift against a *frozen* reference set, where you know what the
"right" answer is supposed to be, is not — if the model's behavior on the
exact same frozen inputs changes, that's evidence of a shift in the
model's decision boundary, which you can then investigate as either
"is this an improvement" or "is this a regression," with actual ground
truth to check against, instead of guessing.

**Calibration matters as much as classification correctness here.** A
model that's 90% confident and right 90% of the time is calibrated. A
model that's 90% confident and right 60% of the time is miscalibrated —
and in this product, the confidence score itself often gets surfaced to
analysts or used downstream to decide "does this get an LLM-generated
summary drafted automatically, or does it need human review first."
Miscalibration silently breaks that downstream decision even when raw
classification accuracy looks unchanged. A reliability check (bucket
predictions by confidence, compare each bucket's predicted confidence to
its actual accuracy) is a statistical assertion the JD's "invariants"
language is pointing at, beyond simple range/bounds checks.

**Invariants are assumptions your team is already making without writing
them down — your job is to make them explicit and enforced.** "A finding
with an actively-exploited CVE (per threat intel) should never be scored
below medium risk, regardless of what the model's raw output says" is a
business rule, not a learned pattern — and it's exactly the kind of
guardrail you'd enforce as a post-model invariant check, independent of
whatever the model's internals happen to output that month. This is a
strong, concrete talking point: statistical validation isn't only about
catching drift, it's also about encoding hard business/safety invariants
the model itself is never guaranteed to respect on its own.

## How to say this out loud

*"I don't trust a single aggregate accuracy number for a system like
this, because the class that matters most — critical findings — is also
the rarest, and aggregate metrics get dominated by the common case. I'd
run statistical checks per class, not just globally, and I'd maintain a
frozen, versioned reference set with known-correct labels specifically so
that drift detection isn't ambiguous — comparing against live production
data can't tell you if a shift is a regression or a legitimate change in
the threat landscape, but comparing against a fixed reference set with
ground truth can. I'd also treat calibration as a first-class check, not
just classification accuracy, because confidence scores often drive
downstream automation decisions, and a model can be accurate on average
while being badly miscalibrated. And I'd enforce hard business invariants
— like actively-exploited CVEs never scoring below a floor — as a
guardrail independent of the model, because a learned model has no
inherent guarantee it'll respect a rule like that on its own."*

## Check yourself

1. Aggregate accuracy improved but critical-class recall dropped. Design
   the specific check (which function, computed on what, compared to
   what) that would have surfaced this before ship, and explain why
   checking it in aggregate wouldn't have worked.
2. Why is a frozen reference set more useful than comparing live
   month-over-month production distributions when you're trying to
   decide "is this a bug"? What does the frozen set give you that live
   data can't?
3. Give one invariant specific to VulnPrioritize's domain — not a generic
   "confidence in [0,1]" check — that you'd enforce independently of
   whatever the model predicts, and explain what real-world harm it
   prevents if the model alone can't be trusted to respect it.
