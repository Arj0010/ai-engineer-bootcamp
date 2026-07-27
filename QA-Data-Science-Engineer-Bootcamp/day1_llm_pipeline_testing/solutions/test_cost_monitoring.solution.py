"""Reference solution for tests/test_cost_monitoring_todo.py -- see solutions/README.md."""
import pytest

from genai_ops_monitoring.cost_monitor_todo import InstrumentedLLMClient, TimeoutBudgetExceeded


class FakeClock:
    def __init__(self, values):
        self._values = iter(values)

    def __call__(self):
        return next(self._values)


def test_complete_with_metrics_returns_accurate_token_and_cost_estimates(llm_client, httpx_mock):
    httpx_mock.add_response(
        url="https://mock-llm.local/v1/complete",
        json={"completion": "ok"},  # 2 chars -> estimate_tokens gives max(1, 2//4) = 1
    )
    instrumented = InstrumentedLLMClient(llm_client, timeout_budget_s=5.0)

    result = instrumented.complete_with_metrics("hi there")  # 8 chars -> 2 tokens

    assert result["completion"] == "ok"
    assert result["prompt_tokens"] == 2
    assert result["completion_tokens"] == 1
    assert result["cost_usd"] > 0
    assert result["latency_s"] >= 0


def test_slow_call_raises_timeout_budget_exceeded(llm_client, httpx_mock):
    httpx_mock.add_response(
        url="https://mock-llm.local/v1/complete",
        json={"completion": "ok"},
    )
    fake_clock = FakeClock([0.0, 5.0])  # start=0.0, end=5.0 -> elapsed=5.0s
    instrumented = InstrumentedLLMClient(llm_client, timeout_budget_s=2.0, clock=fake_clock)

    with pytest.raises(TimeoutBudgetExceeded) as exc_info:
        instrumented.complete_with_metrics("hi there")

    assert exc_info.value.metrics["latency_s"] == 5.0
