"""
Day 2, Exercise A+B combined: tests tying schemas/ and validation/ to the
sample data in ../data/.

Use `Path(__file__).parent.parent / "data" / "..."` to build CSV paths so
these tests work regardless of the directory pytest is invoked from.

--- TASK LIST ---

1. test_good_predictions_pass_schema
   - Load data/good_predictions.csv, validate against
     schemas.schema_todo.prediction_schema, assert it does not raise.

2. test_broken_predictions_fail_schema
   - Load data/broken_predictions.csv, call
     `prediction_schema.validate(df, lazy=True)` inside
     `pytest.raises(pa.errors.SchemaErrors) as exc_info`.
   - Assert `exc_info.value.failure_cases["column"]` contains at least
     "confidence" and "predicted_label" (i.e. prove the schema actually
     caught the specific broken columns, not just "something" failed).

3. test_confidence_within_bounds_flags_broken_data
   - Read the RAW confidence column out of broken_predictions.csv as
     floats where possible (skip the "high" string row, or coerce with
     errors="coerce" and drop NaNs -- your call, note your choice) and
     assert validation.statistical_checks_todo.confidence_within_bounds
     returns False.

4. test_class_balance_flags_imbalance
   - Build a pandas Series that's 95% one label, 5% another, run it
     through class_balance_check with min_fraction=0.1, assert
     balanced=False.

Think about: in task 2, why lazy=True instead of the default fail-fast
validate()? What would you lose in a CI log if you used fail-fast here?
"""
from pathlib import Path

import pandas as pd
import pandera.pandas as pa
import pytest

DATA_DIR = Path(__file__).parent.parent / "data"


def test_good_predictions_pass_schema():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_broken_predictions_fail_schema():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_confidence_within_bounds_flags_broken_data():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_class_balance_flags_imbalance():
    raise NotImplementedError("TODO: implement this test (see task list above)")
