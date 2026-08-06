# 9. Statistics From Zero

> **No prior statistics assumed.** We start at "what is a distribution"
> and end at the one trap that separates people who've read a tutorial
> from people who've actually run a monitoring system.
>
> This module exists because statistical validation is the part of the JD
> most likely to expose whether you understand what you're doing or are
> reciting function names.

---

# PART 1 — THE FOUNDATIONS

## Step 1: What is a distribution?

Forget formulas. A **distribution** answers one question:

> *"What values show up, and how often?"*

Your model outputs confidence scores. Collect 1,000 and count them by range:

```
0.0-0.2  ██                          40 scores
0.2-0.4  ████                        90 scores
0.4-0.6  ████████████               280 scores
0.6-0.8  ██████████████████         420 scores
0.8-1.0  ████████                   170 scores
```

**That shape is the distribution.** It's a picture of where your data lives.
Nothing more complicated than that.

## Step 2: Why compare two distributions?

Last month vs. this month:

```
LAST MONTH                      THIS MONTH
0.0-0.2  ██                     0.0-0.2  ████████
0.2-0.4  ████                   0.2-0.4  ██████████
0.4-0.6  ████████████           0.4-0.6  ████████████
0.6-0.8  ██████████████████     0.6-0.8  ████
0.8-1.0  ████████               0.8-1.0  ██
```

The mass shifted left — the model got **less confident overall**.
Something changed. That's **drift**.

Your job as QA: decide whether that change is **a bug** or **normal variation**.

## Step 3: Why you can't just eyeball it

Flip a fair coin 10 times, twice:

- Round 1: **6 heads**, 4 tails
- Round 2: **4 heads**, 6 tails

Different results. Did the coin change? **No.** Same coin, pure randomness.

> **Every sample varies a little, even when nothing changed.**

So when two distributions look different, there are always two candidate
explanations:

1. Something really changed
2. Random luck — you'd see this much wobble anyway

**A statistical test's only job is to tell those two apart.** That's it.
That's what all of this is for.

## Step 4: The null hypothesis (H₀)

We start by assuming the **boring** explanation:

> **H₀: nothing changed. Both samples come from the same distribution.
> Any difference is just random luck.**

Then we ask: *"if that were true, how surprising is what I actually saw?"*

Very surprising → the boring explanation probably isn't right → **reject H₀**
→ something changed.

**Courtroom analogy:** assume innocent (H₀), then check whether the evidence
is strong enough to reject that assumption. You never *prove* guilt — you
show innocence is implausible.

**Important consequence:** "no drift detected" means *"not enough evidence
of drift,"* NOT *"proven identical."* Absence of evidence isn't evidence of
absence.

## Step 5: The p-value, precisely

> **p-value = if nothing really changed, what's the probability I'd see a
> difference at least this big, purely from luck?**

| p-value | Reading | Reaction |
|---|---|---|
| 0.60 | "60% chance this is just luck" | Shrug, totally normal |
| 0.20 | "20% chance from luck" | Meh |
| 0.03 | "only 3% chance from luck" | Surprising — probably real |
| 0.001 | "0.1% chance from luck" | Very surprising — almost certainly real |

**Low p = surprising = probably a real change.**

### Three things a p-value is NOT

Interviewers probe these, and most people get them wrong:

| ❌ Wrong | ✅ Right |
|---|---|
| "p=0.03 means 3% chance there's no drift" | It's the chance of *this data* given no drift — not the chance of no drift |
| "p-value tells me I'm 97% right" | It says nothing about whether your conclusion is correct |
| "p=0.001 means a big difference" | **It says nothing about size.** ← the critical one |

That last row is Part 3 of this module, and it's the whole game.

## Step 6: Alpha (α) — your decision line

You need a cutoff: *how surprising is surprising enough to act on?*

**Alpha is that line, and you choose it before running the test.**

```python
alpha = 0.05
drifted = p_value < alpha       # below the line → declare drift
```

α = 0.05 means: *"I'll act when there's less than a 5% chance this is luck."*

### The catch that matters operationally

5% of the time, **pure luck will produce p < 0.05 even when nothing changed.**
So:

> ## Alpha IS the false-alarm rate you're agreeing to.

Run 20 drift checks on perfectly stable data with α = 0.05 → expect about
**1 false alarm**. That's not a bug. That's the deal you signed.

| Alpha | Effect |
|---|---|
| 0.01 (strict) | fewer false alarms, but you miss subtle real changes |
| 0.05 (standard) | the usual compromise |
| 0.10 (loose) | catch more real changes, more noise |

## Step 7: The two ways to be wrong

|  | Nothing changed | Something changed |
|---|---|---|
| **You said "drift!"** | ❌ **Type I** — false alarm (rate = α) | ✅ correct |
| **You said "fine"** | ✅ correct | ❌ **Type II** — missed it (rate = β) |

- **Type I** = crying wolf → alert fatigue → people ignore the dashboard
- **Type II** = missing the wolf → the thing you built monitoring to prevent

**You cannot reduce both without more data.** So the real question is:
*which is more expensive here?*

For a security product: **missing a real problem (Type II) usually costs far
more than a false alarm.** That argues for a *looser* alpha on
safety-critical checks — but see Part 3 before you conclude that.

---

# PART 2 — THE ACTUAL TESTS

## The KS test (`ks_2samp`)

**Kolmogorov–Smirnov two-sample test.** Give it two sets of numbers, get
two values back:

```python
from scipy import stats

result = stats.ks_2samp(last_month, this_month)
result.statistic   # HOW DIFFERENT are they? (0 = identical, 1 = no overlap)
result.pvalue      # Is that difference real, or luck?
```

### What the statistic actually measures

Stack both distributions as cumulative curves and measure the **widest
vertical gap** between them. That gap is the statistic.

| statistic | Interpretation |
|---|---|
| < 0.05 | negligible — basically identical |
| 0.05 – 0.10 | small |
| 0.10 – 0.20 | moderate — worth investigating |
| > 0.20 | large — act |

### Properties worth naming in an interview

- **Non-parametric** — assumes no particular distribution shape. (A t-test
  assumes normality; KS doesn't. Good default when you don't know the shape.)
- **Compares the whole distribution**, not just the mean. Two datasets with
  identical means but different spreads → KS catches it, a t-test won't.
- **Continuous data only.** For categorical data (like `predicted_label`),
  use **chi-square** or **PSI** instead.

## Simpler checks that catch more bugs

Don't reach for a fancy test when a simple one works. These catch most
real ML failures:

```python
import numpy as np
import pandas as pd

# 1. BOUNDS — invariants that must always hold
def confidence_within_bounds(confidences: np.ndarray) -> bool:
    return bool(np.all((confidences >= 0.0) & (confidences <= 1.0)))

# 2. CLASS BALANCE — catches silent model collapse
def class_balance_check(labels: pd.Series, min_fraction: float = 0.02) -> dict:
    fractions = labels.value_counts(normalize=True).to_dict()
    balanced = all(f >= min_fraction for f in fractions.values())
    return {"balanced": balanced, "fractions": fractions}

# 3. DRIFT — distribution comparison
def detect_confidence_drift(baseline, current, alpha=0.05) -> dict:
    result = stats.ks_2samp(baseline, current)
    return {
        "drifted": result.pvalue < alpha,
        "statistic": float(result.statistic),
        "p_value": float(result.pvalue),
    }
```

**The class balance check is the highest-value one.** A model that collapses
to predicting one label 98% of the time passes every schema check — every
value is valid — but the product is broken. Only an aggregate check sees it.

> **Schema checks each ROW. Statistics check the BATCH.**
> Neither replaces the other.

## Testing statistical code — seed your randomness

```python
def test_similar_distributions_show_no_drift():
    rng = np.random.default_rng(42)          # ← SEED, or the test is flaky
    baseline = rng.normal(0.7, 0.1, 500)
    current  = rng.normal(0.7, 0.1, 500)
    assert detect_confidence_drift(baseline, current)["drifted"] is False

def test_different_distributions_show_drift():
    rng = np.random.default_rng(42)
    baseline = rng.beta(8, 2, 500)           # skewed high
    current  = rng.beta(2, 8, 500)           # skewed low
    assert detect_confidence_drift(baseline, current)["drifted"] is True
```

Unseeded random data in a test is one of the most common causes of
"passes locally, fails in CI." Same principle as injecting a fake clock:
**control your nondeterminism.**

---

# PART 3 — THE TRAP (the most important part)

## p-value depends on effect size AND sample size

Hold the difference **constant and tiny** (statistic = 0.02). Change only
the number of rows:

| Rows | statistic | p-value | Verdict |
|---|---|---|---|
| 100 | 0.02 | 0.98 | "no drift" |
| 5,000 | 0.02 | 0.31 | "no drift" |
| 80,000 | 0.02 | **0.001** | **"DRIFT!"** 🚨 |

> **The data is identical in all three rows. Only the sample size grew.**

With enough data, **every** microscopic difference becomes "statistically
significant." The test isn't broken — it's correctly answering *"is this
difference real?"* And yes, a 0.02 difference is real. It's just
**completely irrelevant.**

## Worked example

> Drift alert fires. p = 0.001, KS statistic = 0.02, n = 80,000.

- p = 0.001 → "this is almost certainly not random chance" ✓ **true**
- statistic = 0.02 → "the distributions overlap almost perfectly" ✓ **also true**

**Both are true.** It's a real, microscopic, meaningless difference.
**Do not page anyone at 3am for this.**

## The fix — use both numbers

```python
drifted = (result.pvalue < alpha) and (result.statistic > 0.1)
#          ↑ "not just luck"          ↑ "AND big enough to matter"
```

> ## p-value = "is it real?" · statistic = "is it big?" · You need both.

Saying this in an interview signals you've **operated** a monitoring system,
not just read the scipy docs. It's the single highest-value sentence in
this module.

## The multiple comparisons problem

Monitoring 20 features, each at α = 0.05. Probability of **at least one**
false alarm per run:

```
1 - (0.95)^20 ≈ 0.64
```

**64% chance of a spurious alert every single run.** This is why naive
per-feature drift dashboards get ignored within a week — and then the real
alert gets ignored too.

**Two standard fixes to name:**

| Method | How | Trade-off |
|---|---|---|
| **Bonferroni** | divide α by number of tests (0.05/20 = 0.0025) | simple, very conservative, misses real drift |
| **Benjamini-Hochberg (FDR)** | controls the *proportion* of false alarms among alerts | better suited to monitoring |

## PSI — the alternative that dodges the whole trap

**Population Stability Index.** Bins both distributions and sums a weighted
log-ratio. **No p-value, no null hypothesis, no sample-size sensitivity.**
Just one interpretable number:

| PSI | Meaning |
|---|---|
| < 0.1 | stable |
| 0.1 – 0.25 | moderate shift — investigate |
| > 0.25 | significant shift — act |

**Why many monitoring teams prefer it:** it's a pure effect-size measure, so
it doesn't get more alarmist as traffic grows. It also works on
**categorical** data (labels), where KS doesn't apply.

---

# PART 4 — JUDGMENT (round-2 level)

## "The model broke" vs. "the world changed"

A KS test tells you *something changed*. It **cannot** tell you whether
that's a bug or a legitimate shift — a new mass-exploit campaign genuinely
making more findings critical is real, not a regression.

**The QA answer: a frozen reference set.**

- Drift against **live production data** → ambiguous. Did the model change,
  or did the world?
- Drift against a **fixed, versioned batch with known-correct labels** →
  unambiguous. Same inputs in, different outputs out = **the model's
  decision boundary moved.** Now you can investigate whether that's an
  improvement or a regression, with ground truth to check against.

Same principle as the fake clock and the seeded RNG: **hold constant what
you're not testing.**

## Aggregate metrics hide the class that matters

> Overall accuracy improved after retraining. Ship it?

**No.** In vulnerability data, "medium severity" vastly outnumbers
"critical." Aggregate accuracy is dominated by the common class. A model
that nails every medium and misses half the criticals can still show
*improved* overall accuracy.

**The fix is slicing:** run every statistical check **per class**, especially
for classes where a miss is expensive — not just globally.

> *"A single accuracy number optimizes for the boring case and goes blind on
> the rare, high-stakes one."*

## Alerting thresholds should be asymmetric

Symmetric alerting ("page if volume moves ±30%") is the wrong design:

| Direction | Likely cause | Urgency |
|---|---|---|
| **Spike** in critical findings | genuine bad week on the internet | lower — investigate Monday |
| **Drop toward zero**, one customer only | **silent failure** | **page immediately** |

Think about **which direction of anomaly is actually dangerous** rather than
applying a symmetric threshold because it's mathematically tidy.

---

# SAY IT OUT LOUD

> *"For drift detection I'd use a KS test comparing a baseline distribution
> against current production data, but I'd gate alerts on effect size as well
> as p-value — with large samples a trivial difference becomes statistically
> significant, so p-value alone generates constant false alarms and people
> stop trusting the dashboard. For categorical features, or if I wanted
> something that doesn't scale with sample size, PSI is more practical.*
>
> *I'd also run those checks per class rather than in aggregate, because the
> class that matters most is usually the rarest and gets averaged away. And
> I'd compare against a frozen reference set with known labels, not just
> live data — otherwise you can't tell a model regression apart from the
> world legitimately changing."*

---

# CHECK YOURSELF

1. In plain English, what does p = 0.03 mean? Now say what it does **not** mean.
2. You set α = 0.05 and run 20 checks on stable data. How many false alarms
   do you expect, and why isn't that a bug?
3. p = 0.001, statistic = 0.02, n = 80,000. Real drift? What do you do?
4. Why does a schema check miss a model predicting one label 98% of the time?
5. Why is a frozen reference set better than live month-over-month comparison
   for deciding "is this a bug"?
6. Why should a *drop* in critical-finding volume page faster than a *spike*?

---

# QUICK REFERENCE

| Term | Meaning |
|---|---|
| **Distribution** | What values appear and how often |
| **H₀ (null)** | "Nothing changed; the difference is luck" |
| **p-value** | Chance of seeing this difference IF nothing changed |
| **Low p** | Surprising → probably a real change |
| **Alpha (α)** | Your cutoff — and your accepted false-alarm rate |
| **Type I** | False alarm (crying wolf) |
| **Type II** | Missed real change (missing the wolf) |
| **KS statistic** | Effect size: 0 = identical, 1 = no overlap |
| **The trap** | Big n makes trivial differences "significant" |
| **The fix** | `p < alpha AND statistic > 0.1` |
| **PSI** | Effect-size drift metric; no sample-size trap; works on categories |
| **Frozen reference set** | Fixed inputs + known labels → unambiguous drift signal |
| **Slicing** | Run checks per class, not just aggregate |
