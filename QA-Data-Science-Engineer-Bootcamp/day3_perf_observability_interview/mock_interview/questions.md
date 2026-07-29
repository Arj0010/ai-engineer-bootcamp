# Mock interview: reframing your builder stories into QA answers

This file is prep material to read beforehand. The actual mock interview
happens live in the chat: describe 2-3 of your real past projects
(multi-agent systems, LLM orchestration, whatever you've built), and work
through questions like these with me. I will push back when an answer
stays in "here's what I built" mode instead of "here's how I'd verify it
works / here's how it could fail."

## The core reframe

Builder-brain answer: *"I built a RAG pipeline that retrieves chunks and
generates an answer."*

QA-brain answer: *"I built a RAG pipeline. Before I'd trust it in prod, I'd
want: unit tests on the retrieval scoring logic with adversarial edge
cases (empty index, query with no relevant chunks), a golden dataset of
~30 question/answer pairs run through an LLM-judge for faithfulness on
every prompt change, a schema check on the retrieved-chunk metadata so a
malformed document doesn't silently poison retrieval, and a load test on
the embedding endpoint since that's usually the latency bottleneck."*

Same project. Completely different signal to an interviewer hiring for a
QA-adjacent role.

## Warm-up

1. Walk me through one of your real projects in 60 seconds, builder-style,
   the way you'd normally describe it.
2. Now the same project — but every sentence has to answer "how would you
   know if this broke?" instead of "what does this do?"

## Testing strategy questions (bring YOUR project into these)

3. Where are the 3 riskiest points of silent failure in that system —
   places where something could be wrong and nothing would crash or log
   an error?
4. If you had one day to add tests to that project before shipping, what
   would you test first, and why that instead of something else?
5. What's the difference between a unit test and an integration test in
   your system, concretely — name one of each from your actual code.
6. What edge cases did you NOT handle when you built it? (This is
   deliberately uncomfortable — most builder-brain engineers haven't
   thought about this until asked directly.)

## LLM/agent-specific questions

7. Your multi-agent system has agent A call agent B call agent C. Where
   would you add tracing/logging so a failure 3 hops deep is debuggable,
   and what would you actually record at each hop?
8. How would you regression-test a prompt change across your whole agent
   pipeline, not just one LLM call in isolation?
9. Your agent has a tool-calling loop that could in principle run forever
   or call an expensive tool repeatedly. What test would catch that before
   a user does?
10. Suppose your LLM orchestration layer silently started returning valid
    JSON that fails your downstream schema 2% of the time. What's the
    fastest way you'd have caught this — logging? a canary? a synthetic
    test running on a schedule?

## CI/CD and process questions

11. If your team's CI takes 25 minutes and mostly re-runs the same slow
    integration suite, how do you decide what runs on every push vs.
    nightly vs. only pre-release?
12. What's a "flaky test" in the context of an LLM-backed system
    specifically (not a normal flaky test), and how would you handle one
    that fails 1 in 20 runs?

## Behavioral / reframing pressure-test

13. Tell me about a bug in one of your real projects that you found
    AFTER shipping. What test, if it had existed, would have caught it
    before?
14. What's a part of your own project you're least confident is correct,
    and why haven't you tested it yet?

## How the live session will actually run

- You describe a project in your own words.
- I'll ask 2-4 of the questions above, tailored to what you describe.
- When you give a builder-brained answer, I'll push back and ask you to
  redo it in QA framing before moving on — this is the point of the
  exercise, expect friction, don't expect me to let vague answers slide.
- We'll do this for 2-3 of your projects, time permitting.
