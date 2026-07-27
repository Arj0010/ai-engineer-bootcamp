"""
Day 1, Exercise E: Cost, token usage, and timeout monitoring for GenAI calls.

Read genai_ops_monitoring/README.md first.

--- TASK LIST ---

1. Implement estimate_tokens(text: str) -> int
   Use the common approximation: 1 token ~= 4 characters
   (len(text) // CHARS_PER_TOKEN). Return 0 for an empty string. This is
   a rough heuristic, not what a real tokenizer would give you -- note
   that explicitly, don't pretend otherwise.

2. Implement calculate_cost(prompt_tokens, completion_tokens,
   price_per_1k_prompt=DEFAULT_PRICE_PER_1K_PROMPT_TOKENS,
   price_per_1k_completion=DEFAULT_PRICE_PER_1K_COMPLETION_TOKENS) -> float
   cost = (prompt_tokens / 1000) * price_per_1k_prompt
        + (completion_tokens / 1000) * price_per_1k_completion

3. Implement InstrumentedLLMClient.complete_with_metrics(prompt: str) -> dict
   - Record a start time via self._clock().
   - Call self.llm_client.complete(prompt).
   - Record an end time via self._clock(); compute elapsed = end - start.
   - Estimate prompt_tokens and completion_tokens, compute cost_usd.
   - Build metrics = {"completion": ..., "prompt_tokens": ...,
     "completion_tokens": ..., "cost_usd": ..., "latency_s": elapsed}.
   - If elapsed > self.timeout_budget_s: raise
     TimeoutBudgetExceeded(metrics) -- note this raises even though the
     underlying HTTP call succeeded; a technically-200 response that
     blew the latency budget should still count as a test/monitoring
     failure.
   - Otherwise return metrics.

4. Write pytest tests (../tests/test_cost_monitoring_todo.py) that:
   a. Mock a fast LLM response with httpx_mock (see Exercise B's
      pattern), use the REAL default clock (time.perf_counter, the
      default), and assert complete_with_metrics returns correct
      prompt_tokens/completion_tokens/cost_usd for known input/output
      text, and does NOT raise (a real mocked HTTP call is near-instant).
   b. Mock the same fast LLM response, but construct the
      InstrumentedLLMClient with a FAKE clock -- a small callable that
      returns a fixed sequence of values simulating 5 elapsed seconds
      (e.g. an iterator over [0.0, 5.0], one value per self._clock()
      call) -- and assert TimeoutBudgetExceeded is raised, with the
      exception's metrics showing latency_s == 5.0.
      (This tests the timeout-budget LOGIC without needing an actually
      slow network call -- the dependency-injected clock is what makes
      that possible.)

Think about: why inject a clock callable instead of monkeypatching
`time.perf_counter` globally in the test? (Hint: what else in the test
process reads `time.perf_counter` while your test is running, and what
happens to it if you patch the global?)
"""
from __future__ import annotations

import time
from typing import Callable

from pipeline.llm_client import LLMClient

CHARS_PER_TOKEN = 4  # rough approximation; a real system would use tiktoken or the provider's tokenizer

DEFAULT_PRICE_PER_1K_PROMPT_TOKENS = 0.0015
DEFAULT_PRICE_PER_1K_COMPLETION_TOKENS = 0.002


class TimeoutBudgetExceeded(Exception):
    """Raised when a call exceeds its latency budget, even if it returned successfully."""

    def __init__(self, metrics: dict):
        self.metrics = metrics
        super().__init__(f"Call exceeded timeout budget: {metrics.get('latency_s')}s")


def estimate_tokens(text: str) -> int:
    raise NotImplementedError("TODO: see task 1")


def calculate_cost(
    prompt_tokens: int,
    completion_tokens: int,
    price_per_1k_prompt: float = DEFAULT_PRICE_PER_1K_PROMPT_TOKENS,
    price_per_1k_completion: float = DEFAULT_PRICE_PER_1K_COMPLETION_TOKENS,
) -> float:
    raise NotImplementedError("TODO: see task 2")


class InstrumentedLLMClient:
    def __init__(
        self,
        llm_client: LLMClient,
        timeout_budget_s: float = 2.0,
        clock: Callable[[], float] = time.perf_counter,
    ):
        self.llm_client = llm_client
        self.timeout_budget_s = timeout_budget_s
        self._clock = clock

    def complete_with_metrics(self, prompt: str) -> dict:
        raise NotImplementedError("TODO: see task 3")
