"""Reference solution for genai_ops_monitoring/cost_monitor_todo.py -- see solutions/README.md."""
from __future__ import annotations

import time
from typing import Callable

from pipeline.llm_client import LLMClient

CHARS_PER_TOKEN = 4

DEFAULT_PRICE_PER_1K_PROMPT_TOKENS = 0.0015
DEFAULT_PRICE_PER_1K_COMPLETION_TOKENS = 0.002


class TimeoutBudgetExceeded(Exception):
    def __init__(self, metrics: dict):
        self.metrics = metrics
        super().__init__(f"Call exceeded timeout budget: {metrics.get('latency_s')}s")


def estimate_tokens(text: str) -> int:
    if not text:
        return 0
    return max(1, len(text) // CHARS_PER_TOKEN)


def calculate_cost(
    prompt_tokens: int,
    completion_tokens: int,
    price_per_1k_prompt: float = DEFAULT_PRICE_PER_1K_PROMPT_TOKENS,
    price_per_1k_completion: float = DEFAULT_PRICE_PER_1K_COMPLETION_TOKENS,
) -> float:
    return (prompt_tokens / 1000) * price_per_1k_prompt + (completion_tokens / 1000) * price_per_1k_completion


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
        start = self._clock()
        completion = self.llm_client.complete(prompt)
        elapsed = self._clock() - start

        prompt_tokens = estimate_tokens(prompt)
        completion_tokens = estimate_tokens(completion)
        cost_usd = calculate_cost(prompt_tokens, completion_tokens)

        metrics = {
            "completion": completion,
            "prompt_tokens": prompt_tokens,
            "completion_tokens": completion_tokens,
            "cost_usd": cost_usd,
            "latency_s": elapsed,
        }

        if elapsed > self.timeout_budget_s:
            raise TimeoutBudgetExceeded(metrics)

        return metrics
