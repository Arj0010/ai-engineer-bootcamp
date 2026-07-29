"""
Day 2, Exercise A: Define a Pandera schema for model prediction output.

Pandera lets you describe a DataFrame's contract (columns, dtypes, value
ranges, nullability, uniqueness) and validate real data against it --
think "pytest for DataFrames," and the closest thing ML pipelines have to
an API contract test.

Your target schema, based on ../data/good_predictions.csv:

| column          | dtype | constraints                                                |
|-----------------|-------|-------------------------------------------------------------|
| id              | int   | unique, >= 0                                                 |
| text            | str   | non-null, non-empty (str length >= 1)                        |
| predicted_label | str   | one of: billing, technical, account, other                   |
| confidence      | float | 0.0 <= confidence <= 1.0                                     |
| model_version   | str   | non-null                                                     |
| timestamp       | str   | non-null (not parsed to datetime for this exercise)          |

--- TASK LIST ---

1. Build a `pandera.DataFrameSchema` (or a `pandera.DataFrameModel` class --
   try DataFrameModel if you want the more pydantic-like API) named
   `prediction_schema` that captures the table above. Look up
   `pandera.Check` for range/isin/str_length checks, and the `unique=True`
   / `nullable=False` Column kwargs.

2. Write a small script or pytest test (put pytest version in
   ../tests/test_data_contracts_todo.py) that:
   a. Loads ../data/good_predictions.csv with pandas and validates it
      against your schema with `.validate(df)` -- should pass silently.
   b. Loads ../data/broken_predictions.csv and validates it with
      `.validate(df, lazy=True)`, catches the raised
      `pandera.errors.SchemaErrors`, and inspects
      `err.failure_cases` (a DataFrame) to see every row that failed and
      why. Assert that failure_cases contains failures for specific
      columns you expect (e.g. "confidence", "predicted_label", "id").

3. Answer (comment it, or bring it to the debrief): what's the practical
   difference between `schema.validate(df)` (fail-fast, raises on the
   FIRST error) and `schema.validate(df, lazy=True)` (collects ALL
   failures before raising)? When would you want fail-fast vs collect-all
   in a real CI pipeline that gates a merge?
"""
import pandera.pandas as pa

# TODO: define prediction_schema here (DataFrameSchema or DataFrameModel)
prediction_schema = None
