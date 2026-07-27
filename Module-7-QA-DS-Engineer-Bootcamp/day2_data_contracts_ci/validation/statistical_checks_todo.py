"""
Day 2, Exercise B: Statistical / invariant checks with NumPy & SciPy.

Schemas (Pandera) check STRUCTURE row by row: types, ranges, categories.
Statistical checks look at the DATA AS A WHOLE -- this is where you catch
things a per-row schema physically cannot: distribution drift, class
imbalance, a model quietly collapsing to one label, confidence scores
trending down over time.

--- TASK LIST ---

1. Implement confidence_within_bounds(confidences: np.ndarray) -> bool
   Basic invariant: every value in [0.0, 1.0]. (Yes, Pandera could check
   this too on the raw dataframe -- do it again here with plain NumPy so
   you understand the primitive the schema Check is built on.)

2. Implement class_balance_check(labels: pd.Series, min_fraction: float = 0.02) -> dict
   Real classifiers can silently collapse to one dominant label. Compute
   the fraction of rows per label that ACTUALLY APPEAR in the data.
   Return {"balanced": bool, "fractions": {label: fraction, ...}}, where
   balanced=False if any label present in the data falls below
   min_fraction.

3. Implement detect_confidence_drift(baseline, current, alpha: float = 0.05) -> dict
   Use `scipy.stats.ks_2samp` (two-sample Kolmogorov-Smirnov test) to
   compare a baseline confidence distribution against a new batch's
   distribution. Return
   {"drifted": bool, "statistic": float, "p_value": float}, where
   drifted=True if p_value < alpha (i.e. we reject the null hypothesis
   that both samples come from the same distribution).

4. Write a pytest test (../tests/test_data_contracts_todo.py or a new
   file) that:
   - Feeds two near-identical distributions (same np.random.seed, small
     noise) and asserts detect_confidence_drift reports NOT drifted.
   - Feeds two clearly different distributions (e.g. np.random.beta(8, 2,
     500) vs np.random.beta(2, 8, 500)) and asserts it DOES report
     drifted.

Think about: alpha=0.05 means roughly a 1-in-20 false-positive rate on
drift alerts even when nothing actually changed. In a CI pipeline running
on every commit, is that acceptable? What would you do instead (smaller
alpha? require the check to fail N times in a row? only run it on a
schedule, not per-commit)?
"""
from __future__ import annotations

import numpy as np
import pandas as pd
from scipy import stats


def confidence_within_bounds(confidences: np.ndarray) -> bool:
    raise NotImplementedError("TODO: see task 1")


def class_balance_check(labels: pd.Series, min_fraction: float = 0.02) -> dict:
    raise NotImplementedError("TODO: see task 2")


def detect_confidence_drift(baseline: np.ndarray, current: np.ndarray, alpha: float = 0.05) -> dict:
    raise NotImplementedError("TODO: see task 3")
