"""
Day 1, Exercise C: A local mock of LangSmith's core ideas.

Read langsmith_mock/README.md first for the concept explanation.

--- TASK LIST ---

1. Implement MockLangSmithClient.log_run(name, inputs, output) to append
   one JSON record per line (JSONL) to self._storage_path:
     {"name": name, "inputs": inputs, "output": output, "output_hash": ...}
   output_hash = a stable hash of `output`, e.g.
   hashlib.sha256(output.encode()).hexdigest()

2. Implement MockLangSmithClient.get_baseline(name) to read the storage
   file line by line and return the FIRST record matching that name (your
   "golden" reference run), or None if there isn't one yet.

3. Implement compare_to_baseline(current_output, baseline_run) -> dict
   returning at least:
     - "exact_match": bool   (current_output == baseline_run["output"])
     - "hash_match": bool    (do the hashes match?)
   This is your determinism check.

4. Write a short demo in the `if __name__ == "__main__":` block below (or
   a pytest test in tests/, your choice) that:
   a. Logs a baseline run for name="summarize_sky", input
      {"text": "The sky is blue"}, output "The sky is blue."
   b. Simulates a prompt change producing a DIFFERENT output for the same
      logical input.
   c. Calls compare_to_baseline() and asserts/prints that it correctly
      flags the mismatch.

Think about: what would you do differently if the underlying pipeline
used temperature > 0? Is hash_match=False automatically a bug in that
case? What would you check INSTEAD of exact/hash match?
"""
from __future__ import annotations

import hashlib
import json
from pathlib import Path


class MockLangSmithClient:
    def __init__(self, storage_path: str | Path = "runs.jsonl"):
        self._storage_path = Path(storage_path)

    def log_run(self, name: str, inputs: dict, output: str) -> None:
        raise NotImplementedError("TODO: see task 1")

    def get_baseline(self, name: str) -> dict | None:
        raise NotImplementedError("TODO: see task 2")


def compare_to_baseline(current_output: str, baseline_run: dict) -> dict:
    raise NotImplementedError("TODO: see task 3")


if __name__ == "__main__":
    # TODO: task 4 -- write your demo here
    pass
