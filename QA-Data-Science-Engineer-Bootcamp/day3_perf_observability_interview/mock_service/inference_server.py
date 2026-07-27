"""
Minimal mock inference endpoint to load-test against.

Run it with (from day3_perf_observability_interview/):
    uvicorn mock_service.inference_server:app --port 8000

Then hit it manually first to sanity check:
    curl -X POST http://localhost:8000/predict -H "Content-Type: application/json" \
         -d '{"text": "I need a refund"}'
    curl http://localhost:8000/health
"""
import random
import time

from fastapi import FastAPI
from pydantic import BaseModel

app = FastAPI(title="Mock Inference Service")

LABELS = ["billing", "technical", "account", "other"]


class PredictRequest(BaseModel):
    text: str


class PredictResponse(BaseModel):
    label: str
    confidence: float
    latency_ms: float


@app.post("/predict", response_model=PredictResponse)
def predict(request: PredictRequest) -> PredictResponse:
    start = time.perf_counter()
    # Simulate variable inference latency -- real models don't respond in
    # constant time, and load tests that assume they do miss real tail
    # latency behavior.
    time.sleep(random.uniform(0.05, 0.35))
    label = random.choice(LABELS)
    confidence = round(random.uniform(0.5, 0.99), 2)
    latency_ms = (time.perf_counter() - start) * 1000
    return PredictResponse(label=label, confidence=confidence, latency_ms=latency_ms)


@app.get("/health")
def health() -> dict:
    return {"status": "ok"}
