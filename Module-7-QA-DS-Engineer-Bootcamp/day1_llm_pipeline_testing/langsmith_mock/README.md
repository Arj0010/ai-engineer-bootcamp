# LangSmith, conceptually

LangSmith (smith.langchain.com) is LangChain's observability + evaluation
platform for LLM apps. You don't need an account to understand the ideas
that matter for this interview — but you should be able to explain all of
these fluently:

## The core concepts

1. **Tracing.** Every LLM call (and every step of a chain/agent) is
   recorded as a *run*: input, output, latency, token usage, nested child
   runs. This is what lets you debug "why did the agent do that" after
   the fact, instead of only living in `print()` statements.

2. **Datasets.** A curated set of `(input, expected_output)` examples —
   your "golden set." You build this from real production traces (mark
   good/bad ones) or write it by hand.

3. **Experiments / evaluations.** You re-run a dataset through a
   prompt/model/pipeline version and score each output with an
   **evaluator** — either:
   - **Heuristic evaluators**: exact match, regex match, JSON-schema
     validity, string length bounds — fast, free, deterministic.
   - **LLM-as-judge evaluators**: an LLM scores the output against a
     rubric ("is this response grounded in the source document? 1-5").

4. **Regression testing for prompts.** This is the big one for an
   interview: when you change a system prompt, add a tool, or upgrade a
   model, you re-run your golden dataset and diff the new scores against
   the baseline. A prompt change that improves 2 examples but silently
   breaks 5 others is a **regression**, and without this workflow you'd
   ship it and find out from angry users instead.

5. **Determinism checks.** At `temperature=0`, you generally *expect* the
   same input to produce the same (or near-identical) output across runs.
   If it doesn't, that's a signal worth investigating — could be
   non-determinism in the model provider itself, a caching bug, or a
   prompt template that's silently including something non-deterministic
   (a timestamp, an unordered dict, etc). At `temperature > 0` you
   *expect* variation, so "determinism check" becomes "does the output
   stay within acceptable bounds / pass the same evaluators," not
   "identical string."

## Why teams use it instead of just `assert output == expected`

LLM outputs are rarely byte-identical even at temperature=0 across model
versions, and are almost never byte-identical at temperature>0. So LLM
testing typically layers:

```
exact/heuristic checks  →  cheap, fast, catch obvious breakage
        +
LLM-as-judge checks     →  catch subtler regressions (tone, grounding,
                            completeness) that string matching can't see
        +
human review of flagged  →  the expensive fallback for anything the above
cases                       two disagree on or flag as borderline
```

## The exercise

You don't have a real LangSmith API key here, and setting one up is not a
good use of 3 days of prep time. Instead, `langsmith_stub.py` has you build
a **local mock** that captures the same shape of problem: log a run,
store a baseline, and detect drift from that baseline on a later run. This
is deliberately small — the goal is that you can walk into the interview
and say "I built a toy version of LangSmith's regression-testing idea and
here's exactly what determinism check I implemented and why," not that
you've memorized LangSmith's UI.

Open `langsmith_stub.py` and follow the task list at the top.

**Reality check:** actually wiring up real LangSmith (API key, `@traceable`
decorators, the hosted UI, dataset management) is very learnable in an
afternoon once you have the account — it is NOT something you need to have
hands-on experience with in 3 days. Being able to explain runs / datasets /
evaluators / regression testing fluently, and having built this toy version
of the underlying idea, is enough to hold your own in a conversation about
it.
