"""
Day 1, Exercise D: Hallucination/toxicity checks -- rule-based + LLM-judge.

Read llm_judge/README.md first for the concept explanation.

--- TASK LIST ---

1. Implement rule_based_hallucination_check(source: str, output: str) -> dict
   Heuristic: extract standalone numbers from `output` via regex (r"\\b\\d+\\b")
   and flag any number that does NOT appear anywhere in `source`. Return:
     {"flagged": bool, "unsupported_numbers": [...]}

2. Implement rule_based_toxicity_check(output: str) -> dict
   Case-insensitive match against BANNED_WORDS below. Return:
     {"flagged": bool, "matched_terms": [...]}

3. Implement llm_judge(source: str, output: str, llm_client) -> dict
   Build a judge prompt instructing the LLM to respond with STRICT JSON:
     {"hallucinated": true/false, "reasoning": "..."}
   Call llm_client.complete(prompt), then json.loads() the response.
   Handle the case where the LLM doesn't return valid JSON -- decide what
   your function does then (see the "think about" question below) and
   implement that choice explicitly, don't let it silently blow up with an
   unhandled JSONDecodeError.

4. Write a pytest test (add it to ../tests/, e.g. test_llm_judge_todo.py)
   that:
   - Feeds a source/output pair with an invented number through
     rule_based_hallucination_check and asserts flagged=True.
   - Feeds a clean pair and asserts flagged=False.
   - Mocks the LLM call (pytest-httpx, same pattern as Exercise B) and
     asserts llm_judge() correctly parses a canned
     '{"hallucinated": true, "reasoning": "..."}' response.
   - Mocks a MALFORMED (non-JSON) LLM response and asserts your chosen
     failure behavior from task 3 actually happens.

Think about: if llm_judge() can't parse the LLM's response, should it
raise, fail safe to {"hallucinated": True} (block the output), or fail
open to {"hallucinated": False} (let it through)? There's no universally
right answer -- pick one, implement it, and be ready to defend the
tradeoff (a blocked-but-safe response vs. a false-negative slipping
through) in the interview.
"""
from __future__ import annotations

import json
import re

BANNED_WORDS = {"idiot", "stupid", "kill", "hate"}

JUDGE_PROMPT_TEMPLATE = (
    "You are grading an AI-generated summary for hallucination.\n"
    "Source: {source}\n"
    "Summary: {output}\n"
    'Respond with strict JSON only: {{"hallucinated": true or false, "reasoning": "..."}}'
)


def rule_based_hallucination_check(source: str, output: str) -> dict:
    raise NotImplementedError("TODO: see task 1")


def rule_based_toxicity_check(output: str) -> dict:
    raise NotImplementedError("TODO: see task 2")


def llm_judge(source: str, output: str, llm_client) -> dict:
    raise NotImplementedError("TODO: see task 3")
