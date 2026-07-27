import pytest

from pipeline.llm_client import LLMClient


@pytest.fixture
def llm_client() -> LLMClient:
    """An LLMClient pointed at a fake base_url -- never actually reaches the network.

    pytest-httpx intercepts httpx calls at the transport layer, so this
    base_url doesn't need to resolve to anything real.
    """
    return LLMClient(base_url="https://mock-llm.local", api_key="test-key")
