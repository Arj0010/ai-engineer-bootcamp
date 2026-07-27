"""Reference solution for langsmith_mock/langsmith_stub.py -- see solutions/README.md."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path


class MockLangSmithClient:
    def __init__(self, storage_path: str | Path = "runs.jsonl"):
        self._storage_path = Path(storage_path)

    def log_run(self, name: str, inputs: dict, output: str) -> None:
        record = {
            "name": name,
            "inputs": inputs,
            "output": output,
            "output_hash": hashlib.sha256(output.encode()).hexdigest(),
        }
        with self._storage_path.open("a") as f:
            f.write(json.dumps(record) + "\n")

    def get_baseline(self, name: str) -> dict | None:
        if not self._storage_path.exists():
            return None
        with self._storage_path.open() as f:
            for line in f:
                record = json.loads(line)
                if record["name"] == name:
                    return record
        return None


def compare_to_baseline(current_output: str, baseline_run: dict) -> dict:
    current_hash = hashlib.sha256(current_output.encode()).hexdigest()
    return {
        "exact_match": current_output == baseline_run["output"],
        "hash_match": current_hash == baseline_run["output_hash"],
    }


if __name__ == "__main__":
    client = MockLangSmithClient(storage_path=Path("/tmp/runs_demo.jsonl"))
    if Path("/tmp/runs_demo.jsonl").exists():
        Path("/tmp/runs_demo.jsonl").unlink()

    client.log_run("summarize_sky", {"text": "The sky is blue"}, "The sky is blue.")
    baseline = client.get_baseline("summarize_sky")

    new_output_after_prompt_change = "The sky appears blue due to Rayleigh scattering."
    diff = compare_to_baseline(new_output_after_prompt_change, baseline)
    assert diff["exact_match"] is False
    assert diff["hash_match"] is False
    print("Regression detected:", diff)

    # A determinism check: same output should always match itself.
    same_output_diff = compare_to_baseline(baseline["output"], baseline)
    assert same_output_diff["exact_match"] is True
    print("No drift on identical output:", same_output_diff)
