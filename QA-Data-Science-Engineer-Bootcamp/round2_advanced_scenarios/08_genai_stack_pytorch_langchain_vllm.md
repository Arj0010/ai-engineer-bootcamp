# 8. Knowing the GenAI Stack: PyTorch, LangChain, vLLM

## The scenario

A round-1-style question — "are you familiar with vLLM?" — gets a
round-1-style answer: "yes, it's a fast LLM inference engine." True, and
useless. A round-2 question sounds more like: "our `draft_remediation`
stage is served through vLLM, and during the CVE-disclosure load spike
from Module 4, latency didn't degrade smoothly — it stayed flat, then
fell off a cliff all at once. Why might that be, and how would it change
how you load-test this specific stage?" That question can't be answered
from "vLLM is fast" — it requires knowing *how* vLLM is fast, well enough
to reason about where it stops being fast.

This module isn't about becoming an ML engineer in the time you have
left. It's about having just enough of a mental model of each tool in the
stack to reason about its **testable surface area** — where the
interesting bugs and load characteristics actually live — because that's
the level the JD's "familiarity" line is actually testing at for a QA
role, not implementation depth.

## Quick recap

You don't have hands-on depth here by design — this was explicitly
scoped out of the 3-day hands-on plan. What you do have: a working mental
model of the pipeline these tools sit inside (Day 1's mock LLM client
stands in for "the thing LangChain/vLLM would really be"), and the
testing patterns from every other module, which is what actually
transfers.

## Going deeper

**PyTorch's testable surface, from a QA angle, is the input/output
contract, not the model's internals.** You are not going to be asked to
debug a gradient. You might reasonably be asked: "what would you check
about a PyTorch model's inputs and outputs before trusting it in a
pipeline?" The answer is the same shape as Module 2's data contracts,
applied to tensors instead of DataFrames: expected input shape and dtype
(a shape mismatch is often a silent broadcasting bug, not a crash),
output range sanity (a risk score model outputting values outside
`[0, 1]` after a softmax is a real, seen-in-practice bug class from a
mismatched loss function), and — directly connecting to Module 3 —
behavioral invariants under known inputs (a frozen test tensor should
produce a stable, known output; if it doesn't after a "harmless"
refactor, something changed that shouldn't have).

**LangChain's testable surface is the orchestration logic around the LLM
call, and that's exactly what Day 1's mocking pattern already covers.**
A LangChain chain or agent is, structurally, the same shape as your
`run_pipeline` function: some deterministic logic (which tool to call,
how to parse a response, whether to retry) wrapped around one or more
LLM calls. The QA-relevant question is never "does LangChain work" (it's
a maintained library, not your code) — it's "does *my* chain's logic
handle a malformed or unexpected LLM response correctly," which you test
exactly the way you tested `run_pipeline`: mock the LLM call at the HTTP
layer (or via LangChain's own fake/test LLM utilities), and assert on
what your orchestration logic does with a good response, a malformed
response, and a slow response. An agent that can call tools adds one more
concrete thing to test: a runaway tool-calling loop (Day 1's cost/timeout
module's reasoning applies directly — bound the loop, test that the
bound is enforced).

**vLLM's testable surface is throughput-under-batching, and it directly
explains the "flat-then-cliff" latency shape from the scenario above.**
vLLM's core optimization is continuous batching: it dynamically groups
concurrent requests into batches to maximize GPU utilization, which is
why throughput stays high and latency stays flat *up to* the point where
the batch is full and the GPU is saturated — and then, unlike a
naively-scaling system that degrades gradually, additional requests queue
behind an already-saturated batch and latency jumps sharply rather than
climbing smoothly. This is precisely why Module 4's "traffic shape
matters more than peak volume" argument matters even more for a
vLLM-served endpoint specifically: a load test needs to find *where the
batch saturation point is*, not just report an average throughput number,
because the failure mode on the other side of that point is a cliff, not
a slope — and a smooth-ramp load test graph can genuinely make a cliff
look like a gentle curve if your sampling resolution near the saturation
point is too coarse.

**The connective tissue across all three: every one of these tools sits
inside the same testable boundary you already know how to reason about —
input contract in, output contract out, orchestration logic in between —
and the specific tool's internals matter to QA mainly insofar as they
change *where the interesting failure modes are*, not because you need
to reimplement or deeply understand the tool itself.**

## How to say this out loud

*"I think about PyTorch, LangChain, and vLLM from a QA angle rather than
an ML-engineering angle — I care about their testable surface area, not
their internals. For a PyTorch model, that means input/output contract
validation, the same shape as schema testing applied to tensors. For
LangChain, it means the orchestration logic around the LLM call is what
I'd actually test, using the same HTTP-mocking pattern I'd use for any
LLM integration — LangChain itself is a maintained dependency, not my
code, so I'm not testing that it works, I'm testing that my chain's logic
handles a bad or slow response correctly. And for vLLM specifically, I'd
want to understand its continuous-batching behavior well enough to know
that latency under load isn't going to degrade gradually — it stays flat
until the batch saturates, then jumps — which changes how I'd design a
load test for a vLLM-served endpoint: I need fine-grained sampling near
the saturation point, not just a smooth ramp and an average number,
because a coarse test can make a real cliff look like a gentle slope."*

## Check yourself

1. A PyTorch-based risk-scoring model occasionally outputs a score
   slightly above 1.0 after a refactor. Without touching the model code,
   what would you check first, and why is this the same category of
   problem as Module 2's data contracts?
2. Explain in your own words why testing "does LangChain work" is the
   wrong framing for a QA engineer, and what the right framing is
   instead.
3. Given vLLM's continuous-batching behavior, design a load test
   sampling strategy that would actually find the saturation cliff,
   instead of averaging over it. What would a too-coarse test get wrong?
