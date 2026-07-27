"""Reference solution for tests/test_data_contracts_todo.py -- see solutions/README.md."""
from pathlib import Path

import pandas as pd
import pandera.pandas as pa
import pytest

from schemas.schema_todo import prediction_schema  # noqa: F401  (replace with schema.solution import if testing solutions dir directly)
from validation.statistical_checks_todo import class_balance_check, confidence_within_bounds

DATA_DIR = Path(__file__).parent.parent / "data"


def test_good_predictions_pass_schema():
    df = pd.read_csv(DATA_DIR / "good_predictions.csv")
    prediction_schema.validate(df)  # should not raise


def test_broken_predictions_fail_schema():
    df = pd.read_csv(DATA_DIR / "broken_predictions.csv")
    with pytest.raises(pa.errors.SchemaErrors) as exc_info:
        prediction_schema.validate(df, lazy=True)

    failed_columns = set(exc_info.value.failure_cases["column"])
    assert "confidence" in failed_columns
    assert "predicted_label" in failed_columns


def test_confidence_within_bounds_flags_broken_data():
    df = pd.read_csv(DATA_DIR / "broken_predictions.csv")
    numeric_confidence = pd.to_numeric(df["confidence"], errors="coerce").dropna().to_numpy()
    assert confidence_within_bounds(numeric_confidence) is False


def test_class_balance_flags_imbalance():
    labels = pd.Series(["billing"] * 95 + ["technical"] * 5)
    result = class_balance_check(labels, min_fraction=0.1)
    assert result["balanced"] is False
