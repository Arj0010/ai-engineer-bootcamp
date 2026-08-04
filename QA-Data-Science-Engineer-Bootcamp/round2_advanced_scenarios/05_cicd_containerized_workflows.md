# 5. CI/CD and Containerized ML Deploys

## The scenario

A data scientist retrains VulnPrioritize's risk model, the offline
metrics look good, the PR passes all existing CI checks (unit tests,
schema tests, lint), and it merges. Two hours after deploy, the on-call
security analyst team starts getting flooded — the new model is flagging
30% more findings as "critical" than the previous version, across nearly
every customer simultaneously. It's not that the model is *wrong*
exactly — it's more conservative, which sounds safe until you realize
"more conservative" at this scale means thousands of analysts suddenly
drowning in findings that used to be triaged automatically, real alert
fatigue, real angry customers, within hours of a routine merge.

Nothing in CI caught this because nothing in CI was *designed* to catch
it — every existing check tested "is the code correct," and this wasn't a
code bug. It was a legitimate model behavior change that needed a human
decision before 200 customers experienced it simultaneously. This is the
gap between "software CI" and "ML CI," and it's the exact gap the JD is
pointing at by naming CI/CD and containerized workflows together instead
of separately.

## Quick recap

You already have a real, working GitHub Actions workflow
(`.github/workflows/qa-ds-bootcamp-ci.yml`) gating merges on Day 1 and
Day 2's pytest suites, and you know the distinction between "CI ran and
went red" and "CI is wired as a required status check that actually
blocks the merge button" — two different pieces of GitHub configuration,
easy to conflate. You also know the conceptual shape of containerized
test environments: ephemeral Jobs, sidecar mock services for hermetic
tests, and why Kubernetes enters the picture mainly when tests need
multiple coordinated services or environment parity with production.

## Going deeper

**Standard CI answers "is the code correct"; ML CI needs an additional
question: "is the new model's behavior acceptable," and that's a
different kind of check.** The scenario above passed every code-correctness
gate because the code *was* correct — it did exactly what the retrained
model told it to do. What was missing is a **shadow-deployment
comparison stage**: run the new model version in parallel with the
current production model against a slice of real (or replayed) traffic,
*without* it affecting any customer yet, and diff the outputs —
volume of findings by severity, class distribution shifts, the same
statistical checks from Module 3 — before a human signs off on
promoting it. This is the CI/CD-specific answer to Module 3's statistical
validation: the checks are the same tools, but shadow deployment is
*where in the pipeline* they need to run to actually prevent the incident,
not just detect it after the fact.

**"Integrate ML-specific tests into CI/CD" means the merge gate has to
include the model-behavior checks from Modules 2 and 3, not just unit
tests.** Concretely: a required status check that runs the Pandera schema
suite, the statistical invariant/drift suite against a frozen reference
set, *and* the standard pytest suite — all three, all required, before
merge is even possible. A team that only gates on pytest and treats
schema/statistical checks as "something we run manually sometimes" has,
in effect, decided model-behavior regressions don't block releases. That's
a real, nameable gap worth being able to point at.

**Multi-tool CI fluency (GitHub Actions / GitLab CI / Jenkins) is about
recognizing the same underlying model wearing different syntax, not
memorizing three separate systems.** All three express the same idea —
a DAG of jobs/stages triggered by an event, with dependencies between
them and a pass/fail gate — using different YAML shapes and different
mechanisms for the actual merge-block (`required status checks` in
GitHub, merge request approval rules + pipeline status in GitLab,
branch-protection plugins in Jenkins). If you can explain *why* the
GitHub Actions workflow in this repo is structured the way it is —
separate jobs per concern, `needs:` for a merge-gate job that depends on
both — you can describe the equivalent GitLab `stages:`/`needs:`
structure or Jenkins declarative `stage` blocks even without having typed
the exact syntax before, because the reasoning transfers and the syntax
doesn't need to be memorized cold.

**Containerization's actual QA payoff here is reproducibility of the
*shadow comparison*, not just "tests run in Docker."** The shadow-deployment
stage above needs the new model version and the current production
version running side-by-side against identical input — which only means
anything if both are running in environments that match production
closely enough that a difference in output reflects the model, not an
environment inconsistency (a different NumPy version silently changing
floating-point behavior, for instance). This is the concrete reason
"containerized workflows" sits next to "CI/CD" in the JD rather than
being a separate bullet: without environment parity, a shadow-deployment
diff is noise you can't trust.

## How to say this out loud

*"I'd draw a hard line between what standard software CI checks — is the
code correct — and what ML CI additionally needs to check — is the new
model's behavior acceptable, which a code-correctness test genuinely
can't answer. Concretely, I'd want the statistical and schema checks from
earlier in my testing strategy wired in as required status checks
alongside pytest, not run manually and separately. And for model changes
specifically, I'd push for a shadow-deployment stage — run the new model
against real or replayed traffic in parallel with production, diff the
behavior, and require a human sign-off on the diff before promotion —
because the actual incident this kind of gap causes isn't a code bug at
all, it's a legitimate but unreviewed behavior shift hitting every
customer at once. And I'd insist that comparison run in matched,
containerized environments, because a shadow-deployment diff is only
trustworthy if you can rule out environment drift as the explanation."*

## Check yourself

1. The retrained model passed every existing CI check. Design the
   specific new CI stage that would have caught the 30%-more-criticals
   problem before merge — what does it compare, against what, and who
   or what makes the actual promote/reject decision?
2. Why isn't "add a unit test for the new model's output" a real answer
   to the scenario above? What's structurally different about this
   regression compared to a code bug a unit test would normally catch?
3. Explain, in your own words, why an un-pinned or drifted container
   environment between the shadow model and the production model could
   produce a misleading diff — give one concrete example of a dependency
   version difference that could change numeric output silently.
