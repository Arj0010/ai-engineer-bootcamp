"""Orchestrates the classifier + LLM fallback.

This is the thing your test suite is really protecting: business logic
that branches on a model's confidence and calls out to an LLM for the
uncertain cases. This branch-on-confidence pattern (cheap model first,
expensive model as fallback) is extremely common in production LLM
systems, and it's exactly the kind of logic that's easy to break silently.
"""
from __future__ import annotations

from .classifier import classify
from .llm_client import LLMClient

LOW_CONFIDENCE_THRESHOLD = 0.6

FALLBACK_PROMPT_TEMPLATE = (
    "Classify the following support ticket into one of: billing, technical, "
    "account, other. Respond with just the label.\n\nTicket: {text}"
)


def run_pipeline(text: str, llm_client: LLMClient | None = None) -> dict:
    client = llm_client or LLMClient()
    result = classify(text)

    if result["confidence"] < LOW_CONFIDENCE_THRESHOLD:
        prompt = FALLBACK_PROMPT_TEMPLATE.format(text=text)
        llm_label = client.complete(prompt).strip().lower()
        result = {**result, "label": llm_label, "source": "llm_fallback"}
    else:
        result = {**result, "source": "rule_based"}

    return result
