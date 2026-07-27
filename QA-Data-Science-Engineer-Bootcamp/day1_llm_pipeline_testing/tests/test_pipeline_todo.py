"""
Day 1, Exercise B: Testing the pipeline with a mocked LLM call.

pipeline.run_pipeline() calls out to LLMClient.complete(), which makes a
real HTTP POST (see pipeline/llm_client.py). In tests we NEVER want to hit
a real network / real API -- so you'll use the `httpx_mock` fixture from
pytest-httpx to intercept that call at the transport layer.

Setup: pytest-httpx is in requirements-qa.txt; installing it auto-registers
the `httpx_mock` fixture, no plugin config needed. The `llm_client` fixture
below comes from conftest.py.

--- TASK LIST ---

1. test_high_confidence_input_never_calls_llm
   - Give run_pipeline() a ticket the rule-based classifier is confident
     about (e.g. clear billing keywords).
   - Assert result["source"] == "rule_based".
   - Do NOT register any httpx_mock response. If the code path
     incorrectly tries to call the LLM anyway, httpx_mock will raise
     (no matching mock registered) and your test fails loudly -- proving
     the LLM was never invoked.

2. test_low_confidence_input_falls_back_to_llm
   - Give run_pipeline() an ambiguous ticket (no/few keyword matches).
   - Use httpx_mock.add_response(url=..., json={"completion": "technical"})
     to mock the LLM's reply.
   - Assert result["label"] == "technical" and
     result["source"] == "llm_fallback".

3. test_llm_error_propagates
   - Use httpx_mock.add_response(status_code=500) (or
     httpx_mock.add_exception(httpx.ConnectTimeout("boom"))) to simulate
     an LLM outage on a low-confidence ticket.
   - Assert run_pipeline() raises -- decide which exception type you
     expect and assert on it specifically (not a bare `except Exception`).

4. [YOUR CALL] test_llm_prompt_contains_the_original_ticket_text
   - Inspect the request httpx_mock captured (httpx_mock.get_requests())
     and assert the original ticket text appears in the JSON body sent to
     the LLM. This catches a real bug class: silently dropping or
     truncating user input before it reaches the model.

Think about (be ready to answer without looking anything up):
Why mock at the HTTP layer instead of unittest.mock.patch'ing
LLMClient.complete directly? What would a method-patch-based test fail to
catch if someone changed the request URL path or a JSON key name in
llm_client.py?
"""
import httpx
import pytest

from pipeline.pipeline import run_pipeline


def test_high_confidence_input_never_calls_llm(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_low_confidence_input_falls_back_to_llm(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_llm_error_propagates(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_llm_prompt_contains_the_original_ticket_text(llm_client, httpx_mock):
    raise NotImplementedError("TODO: implement this test (see task list above)")
