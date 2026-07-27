"""A deterministic, rule-based classifier.

This stands in for "a simple ML classifier" -- no model weights, no
randomness, so its behavior is fully specified and testable. Real
classifiers hide behind a similar interface: input in, label + confidence
out. Testing this teaches the same muscles as testing a real model's
inference wrapper, without needing GPUs or training data.
"""
from __future__ import annotations

KEYWORD_LABELS: dict[str, tuple[str, ...]] = {
    "billing": ("invoice", "charge", "refund", "payment", "subscription", "price"),
    "technical": ("error", "bug", "crash", "exception", "not working", "broken", "500", "timeout"),
    "account": ("password", "login", "locked out", "email address", "username", "2fa"),
}

DEFAULT_LABEL = "other"
MAX_INPUT_LENGTH = 5_000


def classify(text: str) -> dict:
    if not isinstance(text, str):
        raise TypeError(f"text must be a str, got {type(text).__name__}")
    if len(text) > MAX_INPUT_LENGTH:
        raise ValueError(f"text exceeds max length of {MAX_INPUT_LENGTH} characters")

    stripped = text.strip()
    if not stripped:
        return {"label": DEFAULT_LABEL, "confidence": 0.0, "matched_keywords": []}

    lowered = stripped.lower()
    scores: dict[str, list[str]] = {}
    for label, keywords in KEYWORD_LABELS.items():
        matched = [kw for kw in keywords if kw in lowered]
        if matched:
            scores[label] = matched

    if not scores:
        return {"label": DEFAULT_LABEL, "confidence": 0.0, "matched_keywords": []}

    best_label = max(scores, key=lambda label: len(scores[label]))
    matched = scores[best_label]
    total_possible = len(KEYWORD_LABELS[best_label])
    confidence = min(1.0, len(matched) / total_possible + 0.5)

    return {"label": best_label, "confidence": round(confidence, 2), "matched_keywords": matched}
