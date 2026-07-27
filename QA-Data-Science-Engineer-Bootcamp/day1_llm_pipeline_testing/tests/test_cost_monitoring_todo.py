"""
Day 1, Exercise E tests: see genai_ops_monitoring/cost_monitor_todo.py task 4.
"""
import pytest

from genai_ops_monitoring.cost_monitor_todo import InstrumentedLLMClient, TimeoutBudgetExceeded


class FakeClock:
    """Returns a fixed sequence of values, one per call -- lets a test claim
    "5 seconds elapsed" without actually waiting 5 seconds."""

    def __init__(self, values):
        self._values = iter(values)

    def __call__(self):
        return next(self._values)


def test_complete_with_metrics_returns_accurate_token_and_cost_estimates(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task 4a)")


def test_slow_call_raises_timeout_budget_exceeded(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task 4b)")
