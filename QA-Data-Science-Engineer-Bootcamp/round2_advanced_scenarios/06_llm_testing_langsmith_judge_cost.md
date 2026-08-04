# 6. LLM Testing: LangSmith, Judges, Cost/Token/Timeout

## The scenario

VulnPrioritize's `draft_remediation` stage takes a scored finding and
asks an LLM to write two things: a plain-English summary for the
executive dashboard, and step-by-step remediation instructions for the
engineer who has to fix it. One night, for a finding about an exposed
database port, the LLM's remediation instructions said to "disable the
firewall rule blocking external access to complete the fix" — the exact
opposite of correct, a hallucinated inversion of the actual remediation
(which was to *add* a blocking rule, not remove one). An engineer who
trusted it without double-checking would have made the customer's
exposure worse, not better, in a security product whose entire value
proposition is reducing exposure.

Separately, that same week, a routine prompt-template tweak — adding one
extra paragraph of "context" to make summaries friendlier — quietly
doubled the average tokens-per-call. Nobody noticed until the monthly
GenAI infrastructure bill came in nearly 2x normal, for a change nobody
would have called a big deal at review time.

Two different failure classes, same feature: one is a **correctness/safety**
failure, one is a **cost/operability** failure. The JD lists them as
separate sub-bullets on purpose — they need separate testing strategies.

## Quick recap

You've built the mechanics for both. For correctness: a mock
`MockLangSmithClient` that logs runs and diffs a new output's hash against
a stored baseline (a determinism/regression check), plus
`rule_based_hallucination_check` (flagging output numbers unsupported by
the source) and `llm_judge` (a structured-JSON verdict from a second LLM
call) working together — rule-based first as a cheap fast gate, judge
second for nuance. For cost: `InstrumentedLLMClient.complete_with_metrics`
wraps a call with token estimation, cost calculation, and a
dependency-injected clock enforcing a timeout budget that fails even a
technically-successful call if it's too slow.

## Going deeper

**The firewall-rule hallucination is a "policy adherence" failure, and
it needs a rubric your rule-based/hallucination checks from Day 1 don't
already cover.** Your existing `rule_based_hallucination_check` catches
unsupported *numbers* — a good, cheap first pass, but it wouldn't catch
"the LLM inverted the polarity of a security recommendation" because
that's not a numeric fact, it's a *directional/safety* claim. Policy
adherence checks need their own rubric, evaluated the same way (rule-based
first, LLM-judge second) but asking a different question: not "is this
grounded in the source," but "does this recommendation violate a known
safety invariant" — e.g., a rule-based check for a fixed list of
dangerous action verbs near security-control nouns ("disable," "remove,"
"open" near "firewall," "rule," "block") as a fast, blunt first-pass flag,
escalated to an LLM-judge prompt specifically asking "does this
recommendation reduce or increase the customer's exposure, relative to
their current state?" This is the concrete, buildable answer to "policy
adherence" as its own JD line item, distinct from hallucination/toxicity.

**Determinism checks need a different bar for a product where the
"same" input legitimately isn't stable.** Your Day 1 mock assumed
`temperature=0` implies same input → same output, and flagged any hash
mismatch as regression-worthy. In VulnPrioritize, the input to
`draft_remediation` includes live threat-intel context that changes daily
even for the "same" underlying finding — so a naive hash-diff would flag
drift constantly, on data that's supposed to change. The real determinism
check here is narrower and more useful: hold the *input* completely fixed
(a frozen, versioned finding — same idea as Module 3's frozen reference
set) and verify the output stays stable *for that exact frozen input*
across prompt/model changes. Determinism isn't "the system never
varies" — it's "the same fixed input produces the same output," which
requires you to actually control for what's allowed to vary before you
can meaningfully test what shouldn't.

**Cost/token/timeout monitoring belongs in the same regression-testing
loop as correctness, not off in a separate dashboard-only world.** The
"friendlier paragraph" prompt change that doubled token cost would have
been caught in code review by *nobody*, because it read as a harmless
wording tweak — the problem is invisible in the diff and only visible in
the metric. The fix: your `InstrumentedLLMClient`'s cost/token output
should run against the same frozen golden dataset used for correctness
regression testing, on every prompt change, with an assertion like "cost
per call must not increase by more than X% without explicit sign-off" —
turning an invisible-in-review change into a visible, blocked CI failure,
the same mechanism as Module 5's shadow-deployment idea, just applied to
cost instead of classification behavior.

**Timeout-budget enforcement is a safety mechanism, not just a
performance one, in this specific product.** A remediation summary that
takes 45 seconds to generate isn't merely "slow" — during an active
incident, an analyst waiting 45 seconds per finding across dozens of
findings is a real, compounding cost to response time. Your
`TimeoutBudgetExceeded` design — raising even on a technically-successful
call — is the right shape; the round-2-level addition is connecting it to
a **fallback**: what does the system actually do when a summary times
out? Serve the finding with its raw scored data and no LLM summary
(Module 4's graceful-degradation idea, again) rather than block the
entire finding from reaching the analyst. Testing the timeout without
testing the fallback path only proves you can detect the problem, not
that the system survives it.

## How to say this out loud

*"I split LLM testing into two tracks that need different rigor: correctness/
safety, and cost/operability — because a prompt change that looks
completely harmless in a code review can silently break either one
independently. For correctness, I'd layer rule-based checks first —
cheap, fast, deterministic — and escalate to an LLM-judge for nuance,
and I'd build a dedicated policy-adherence rubric separate from general
hallucination checks, because in a security product, a directionally-inverted
recommendation is its own distinct and more dangerous failure mode. For
determinism, I'd test against a frozen, versioned input specifically,
because this system's real inputs include live threat intel that's
supposed to change day to day — testing determinism against live data
would just generate noise. And I'd run cost and token metrics through the
same regression pipeline as correctness, against the same golden dataset,
because a cost regression is often invisible in code review and only
shows up as a surprise on a bill weeks later if nothing catches it
sooner."*

## Check yourself

1. Design the specific rule-based policy-adherence check that would have
   flagged the firewall-rule inversion before it reached an engineer.
   What does it pattern-match on, and what's its realistic false-positive
   rate — would you ship it as a hard block or a "flag for human review"
   gate, and why?
2. Explain precisely why hash-diffing every `draft_remediation` output
   against a fixed baseline would produce constant false alarms in
   production, but the same hash-diff approach is still useful in a test
   suite. What's different between the two settings?
3. The token-cost regression from the "friendlier paragraph" change
   wasn't caught by any existing test. Walk through exactly where in
   your CI pipeline you'd add the check that catches it, and what
   specific number it asserts on.
