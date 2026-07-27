"""Reference solution for orchestration_testing/dagster_pipeline_todo.py -- see solutions/README.md."""
from __future__ import annotations

from pathlib import Path

import dagster as dg
import pandas as pd

from schemas.schema_todo import prediction_schema
from validation.statistical_checks_todo import class_balance_check

DATA_DIR = Path(__file__).parent.parent / "data"


class CSVPathResource(dg.ConfigurableResource):
    path: str


@dg.asset
def raw_predictions(csv_path: CSVPathResource) -> pd.DataFrame:
    return pd.read_csv(csv_path.path)


@dg.asset
def validated_predictions(raw_predictions: pd.DataFrame) -> pd.DataFrame:
    return prediction_schema.validate(raw_predictions)


@dg.asset
def drift_report(validated_predictions: pd.DataFrame) -> dict:
    return class_balance_check(validated_predictions["predicted_label"])
