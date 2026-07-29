"""Reference solution for tests/test_orchestration_todo.py -- see solutions/README.md."""
from pathlib import Path

import dagster as dg

# NOTE: this import path matches tests/test_orchestration_todo.py -- when
# checking this solution, dagster_pipeline.solution.py is copied over
# orchestration_testing/dagster_pipeline_todo.py, so the module path below
# is what actually resolves.
from orchestration_testing.dagster_pipeline_todo import (
    CSVPathResource,
    drift_report,
    raw_predictions,
    validated_predictions,
)

DATA_DIR = Path(__file__).parent.parent / "data"


def test_good_csv_pipeline_run_succeeds():
    result = dg.materialize(
        [raw_predictions, validated_predictions, drift_report],
        resources={"csv_path": CSVPathResource(path=str(DATA_DIR / "good_predictions.csv"))},
    )
    assert result.success is True
    assert "fractions" in result.output_for_node("drift_report")


def test_broken_csv_pipeline_run_fails():
    result = dg.materialize(
        [raw_predictions, validated_predictions, drift_report],
        resources={"csv_path": CSVPathResource(path=str(DATA_DIR / "broken_predictions.csv"))},
        raise_on_error=False,
    )
    assert result.success is False
