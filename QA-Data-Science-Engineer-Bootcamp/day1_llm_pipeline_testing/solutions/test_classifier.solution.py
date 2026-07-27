"""Reference solution for tests/test_classifier_todo.py -- see solutions/README.md."""
import pytest

from pipeline.classifier import classify


def test_billing_keywords_classified_as_billing():
    result = classify("I need a refund on my last invoice")
    assert result["label"] == "billing"
    assert result["confidence"] > 0.5


@pytest.mark.parametrize("text", ["", "   ", "\n\t"])
def test_empty_string_returns_other_with_zero_confidence(text):
    result = classify(text)
    assert result["label"] == "other"
    assert result["confidence"] == 0.0


@pytest.mark.parametrize("bad_input", [123, None, ["a", "list"], {"not": "a string"}])
def test_non_string_input_raises_type_error(bad_input):
    with pytest.raises(TypeError):
        classify(bad_input)


def test_over_max_length_raises_value_error():
    from pipeline.classifier import MAX_INPUT_LENGTH

    too_long = "a" * (MAX_INPUT_LENGTH + 1)
    with pytest.raises(ValueError):
        classify(too_long)


def test_case_insensitivity():
    lower = classify("refund my invoice")
    upper = classify("REFUND MY INVOICE")
    assert lower["label"] == upper["label"]
    assert lower["confidence"] == upper["confidence"]


def test_no_keyword_match_returns_other():
    result = classify("What's the weather today?")
    assert result["label"] == "other"
    assert result["confidence"] == 0.0


@pytest.mark.parametrize(
    "text,expected_label",
    [
        ("I was charged twice for my subscription", "billing"),
        ("The app crashes with a 500 error", "technical"),
        ("I'm locked out, forgot my password", "account"),
    ],
)
def test_each_label_has_at_least_one_matching_example(text, expected_label):
    result = classify(text)
    assert result["label"] == expected_label


def test_matched_keywords_is_subset_of_label_keywords():
    from pipeline.classifier import KEYWORD_LABELS

    result = classify("I need a refund on my invoice")
    label = result["label"]
    assert set(result["matched_keywords"]).issubset(set(KEYWORD_LABELS[label]))
