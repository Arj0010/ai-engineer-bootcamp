"""Reference solution for tests/test_pipeline_todo.py -- see solutions/README.md."""
import httpx
import pytest

from pipeline.pipeline import run_pipeline


def test_high_confidence_input_never_calls_llm(llm_client, httpx_mock):
    # No httpx_mock.add_response() registered -- if run_pipeline tried to
    # call the LLM, httpx_mock would raise for the unmatched request.
    result = run_pipeline("I was charged twice for my subscription, need a refund", llm_client)
    assert result["source"] == "rule_based"
    assert result["label"] == "billing"


def test_low_confidence_input_falls_back_to_llm(llm_client, httpx_mock):
    httpx_mock.add_response(
        url="https://mock-llm.local/v1/complete",
        json={"completion": "technical"},
    )
    result = run_pipeline("something is wrong but I can't describe it", llm_client)
    assert result["label"] == "technical"
    assert result["source"] == "llm_fallback"


def test_llm_error_propagates(llm_client, httpx_mock):
    httpx_mock.add_response(url="https://mock-llm.local/v1/complete", status_code=500)
    with pytest.raises(httpx.HTTPStatusError):
        run_pipeline("something is wrong but I can't describe it", llm_client)


def test_llm_prompt_contains_the_original_ticket_text(llm_client, httpx_mock):
    ticket_text = "the thingamajig broke somehow"
    httpx_mock.add_response(
        url="https://mock-llm.local/v1/complete",
        json={"completion": "technical"},
    )
    run_pipeline(ticket_text, llm_client)

    requests = httpx_mock.get_requests()
    assert len(requests) == 1
    body = httpx.Request.read(requests[0]) and requests[0].content
    assert ticket_text.encode() in body
