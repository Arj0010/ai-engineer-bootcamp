# Cost, token usage, and timeout monitoring for GenAI workflows

This is a distinct QA concern from Exercise D's hallucination/toxicity
checks: those verify the response is *correct*; this exercise verifies
the call was *affordable and fast enough*, which is a separate failure
mode a purely functional test suite won't catch.

## Why this matters as its own testing category

- **Token usage regressions are invisible to functional tests.** A prompt
  template change that accidentally includes an entire retrieved document
  instead of a snippet will still produce a *correct-looking* answer — it
  just costs 50x more per call. Nothing about correctness testing catches
  that; you need an explicit token/cost assertion.
- **A slow-but-successful call is still a production failure.** If your
  SLA is "respond in under 2 seconds" and a call takes 6 seconds but
  eventually returns a valid 200, a purely functional test (did it return
  200? is the JSON valid?) says pass. A latency-budget check says fail —
  and it should, because real users already gave up waiting.
- **These checks belong in monitoring, not just CI**, but you write and
  unit-test the underlying logic the same way either way — this exercise
  builds the logic; Day 3's observability section covers where it plugs
  into a live dashboard.

## The exercise

`cost_monitor_todo.py` wraps the Day 1 `LLMClient` with an
`InstrumentedLLMClient` that measures latency, estimates token usage,
computes a dollar cost, and enforces a timeout budget — raising even on a
technically-successful call if it ran too slow.

Note the constructor takes a `clock` parameter instead of calling
`time.perf_counter()` directly inside the class. This is a deliberate
**dependency injection** pattern: instead of monkeypatching Python's
global `time` module (which works, but silently affects everything else
using `time` during that test), you inject a fake, fully-controllable
clock function in your tests. This is the same idea as injecting a fake
random-number generator to test code that depends on `random` — a
reusable technique worth having in your back pocket for any time-dependent
or randomness-dependent code you need to test deterministically.

Open `cost_monitor_todo.py` and follow the task list.

## Checkpoint question

Your `estimate_tokens()` uses a chars-per-token heuristic instead of a
real tokenizer (`tiktoken` or similar). What's the practical risk of
shipping a cost dashboard built entirely on this approximation instead of
exact token counts from the provider's API response? When would the
approximation be good enough vs. not?
