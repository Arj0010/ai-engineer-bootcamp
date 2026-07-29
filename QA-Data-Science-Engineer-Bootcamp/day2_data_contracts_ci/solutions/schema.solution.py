"""Reference solution for schemas/schema_todo.py -- see solutions/README.md."""
import pandera.pandas as pa
from pandera.pandas import Check, Column, DataFrameSchema

prediction_schema = DataFrameSchema(
    {
        "id": Column(int, unique=True, checks=Check.ge(0)),
        "text": Column(str, nullable=False, checks=Check.str_length(min_value=1)),
        "predicted_label": Column(
            str, checks=Check.isin(["billing", "technical", "account", "other"])
        ),
        "confidence": Column(float, checks=Check.in_range(0.0, 1.0)),
        "model_version": Column(str, nullable=False),
        "timestamp": Column(str, nullable=False),
    },
    strict=False,
)
