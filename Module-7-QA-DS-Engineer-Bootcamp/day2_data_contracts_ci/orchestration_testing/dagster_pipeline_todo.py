"""
Day 2, Exercise C: Testing an orchestrated pipeline with Dagster's test utilities.

Read orchestration_testing/README.md first. This wires
schemas.schema_todo.prediction_schema and
validation.statistical_checks_todo.class_balance_check (from Exercises A
and B -- implement those first if you haven't) into a 3-stage Dagster
asset pipeline, and tests it with `dagster.materialize`.

--- TASK LIST ---

1. Define a resource for the CSV path:

     class CSVPathResource(dg.ConfigurableResource):
         path: str

2. Implement `raw_predictions(csv_path: CSVPathResource) -> pd.DataFrame`
   as a `@dg.asset`, returning `pd.read_csv(csv_path.path)`.

3. Implement `validated_predictions(raw_predictions: pd.DataFrame) -> pd.DataFrame`
   as a `@dg.asset` depending on `raw_predictions` (Dagster wires the
   dependency automatically from the parameter name matching the upstream
   asset's function name). Call `prediction_schema.validate(raw_predictions)`
   and return the result -- let a `pandera.errors.SchemaError` propagate
   and fail the step; don't catch it here.

4. Implement `drift_report(validated_predictions: pd.DataFrame) -> dict`
   as a `@dg.asset`, calling `class_balance_check` on the
   `predicted_label` column and returning its result dict.

5. Write a pytest test (in ../tests/, e.g. test_orchestration_todo.py) that:
   a. Calls
        dg.materialize(
            [raw_predictions, validated_predictions, drift_report],
            resources={"csv_path": CSVPathResource(path=str(DATA_DIR / "good_predictions.csv"))},
        )
      and asserts `result.success is True`, then inspects
      `result.output_for_node("drift_report")`.
   b. Calls the same materialize with the broken CSV path AND
      `raise_on_error=False`, and asserts `result.success is False`.

Think about (bring to the debrief): what's the tradeoff of using
`dg.materialize` in your test suite (fast, in-process, no scheduler)
versus actually running this through Dagster's dev webserver / a
real schedule? When would each be the right tool?
"""
from __future__ import annotations

from pathlib import Path

import dagster as dg
import pandas as pd

from schemas.schema_todo import prediction_schema
from validation.statistical_checks_todo import class_balance_check

DATA_DIR = Path(__file__).parent.parent / "data"


# TODO: task 1 -- CSVPathResource


# TODO: task 2 -- raw_predictions asset


# TODO: task 3 -- validated_predictions asset


# TODO: task 4 -- drift_report asset
