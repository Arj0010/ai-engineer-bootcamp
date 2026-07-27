"""
Day 2, Exercise C tests: see orchestration_testing/dagster_pipeline_todo.py task 5.
"""
from pathlib import Path

import dagster as dg

from orchestration_testing.dagster_pipeline_todo import (
    CSVPathResource,
    drift_report,
    raw_predictions,
    validated_predictions,
)

DATA_DIR = Path(__file__).parent.parent / "data"


def test_good_csv_pipeline_run_succeeds():
    raise NotImplementedError("TODO: implement this test (see task 5a)")


def test_broken_csv_pipeline_run_fails():
    raise NotImplementedError("TODO: implement this test (see task 5b)")
