"""Reference solution for validation/statistical_checks_todo.py -- see solutions/README.md."""
from __future__ import annotations

import numpy as np
import pandas as pd
from scipy import stats


def confidence_within_bounds(confidences: np.ndarray) -> bool:
    return bool(np.all((confidences >= 0.0) & (confidences <= 1.0)))


def class_balance_check(labels: pd.Series, min_fraction: float = 0.02) -> dict:
    fractions = (labels.value_counts(normalize=True)).to_dict()
    balanced = all(fraction >= min_fraction for fraction in fractions.values())
    return {"balanced": balanced, "fractions": fractions}


def detect_confidence_drift(baseline: np.ndarray, current: np.ndarray, alpha: float = 0.05) -> dict:
    result = stats.ks_2samp(baseline, current)
    return {
        "drifted": result.pvalue < alpha,
        "statistic": float(result.statistic),
        "p_value": float(result.pvalue),
    }
