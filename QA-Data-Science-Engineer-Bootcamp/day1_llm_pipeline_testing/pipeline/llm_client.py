"""Stub LLM client used throughout Day 1.

In a real system this would call OpenAI/Anthropic. Here it makes an HTTP
POST to a configurable base_url so it behaves exactly like a real API
client in tests -- you mock the HTTP layer (pytest-httpx) instead of the
Python function itself, which is closer to how you'd test a production
LLM integration than patching the method directly.
"""
from __future__ import annotations

import httpx


class LLMClient:
    def __init__(
        self,
        base_url: str = "https://mock-llm.local",
        api_key: str = "test-key",
        timeout: float = 10.0,
    ):
        self.base_url = base_url.rstrip("/")
        self.api_key = api_key
        self.timeout = timeout

    def complete(self, prompt: str, *, temperature: float = 0.0) -> str:
        if not isinstance(prompt, str):
            raise TypeError(f"prompt must be a str, got {type(prompt).__name__}")
        if not prompt.strip():
            raise ValueError("prompt must not be empty")

        response = httpx.post(
            f"{self.base_url}/v1/complete",
            json={"prompt": prompt, "temperature": temperature},
            headers={"Authorization": f"Bearer {self.api_key}"},
            timeout=self.timeout,
        )
        response.raise_for_status()
        data = response.json()
        return data["completion"]
