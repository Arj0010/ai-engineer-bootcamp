# LLM-as-a-judge, conceptually

Two complementary techniques are used together in production LLM eval
pipelines:

1. **Rule-based / heuristic checks.** Cheap, fast, fully deterministic.
   Good at catching clear-cut cases: banned words, PII patterns
   (emails/SSNs via regex), numbers in the output that don't appear
   anywhere in the source (a blunt hallucination proxy), JSON-schema
   validity, response length bounds. Bad at nuance — they can't tell you
   if a summary subtly misrepresents the source's *meaning* while reusing
   its exact words.

2. **LLM-as-a-judge.** Use an LLM (often a stronger/differently-prompted
   one) to grade another LLM's output against a rubric, e.g.: *"Does this
   summary contain any claim not supported by the source text? Answer
   with strict JSON: {"hallucinated": true/false, "reasoning": "..."}"*.
   More nuanced, but: non-deterministic, slower, costs money, has its own
   failure modes (judges can be fooled, biased toward longer answers,
   inconsistent across runs), and needs a fallback for malformed judge
   output.

**In practice**: rule-based checks run on every request as a fast/cheap
first pass (or a hard gate); LLM-judge runs are used for deeper eval —
often on a sample of production traffic, or on your full golden dataset
during CI/regression testing, not on every single live request, because
of cost and latency.

## The exercise

Open `judge_todo.py` and follow the task list. You'll implement:
- a rule-based hallucination heuristic (numbers not grounded in the source)
- a rule-based toxicity/banned-word check
- an LLM-judge function that prompts the (stub) LLM for a structured
  verdict and parses it

Then write a pytest test (put it in `../tests/`, following the same
pytest-httpx mocking pattern from Exercise B) that exercises both the
heuristic and the mocked-LLM judge path.

**Reality check:** real hallucination/toxicity detection in industry
often uses purpose-built classifier models (e.g. a fine-tuned toxicity
classifier, NLI-based faithfulness scoring) rather than regex or a single
judge prompt — that's a fair thing to *mention* you know exists, but not
something to attempt building in 3 days. What you're building here is the
right *shape* of the solution (heuristic + judge, combined), which is what
interviewers are usually actually probing for.
