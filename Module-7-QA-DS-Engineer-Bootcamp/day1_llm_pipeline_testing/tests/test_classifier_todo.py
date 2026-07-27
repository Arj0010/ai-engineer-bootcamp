"""
Day 1, Exercise A: Unit tests for pipeline.classifier

Read pipeline/classifier.py FIRST. Then write pytest tests for it below.

One example is filled in so you can see the expected style. Everything
else is your job -- replace each `raise NotImplementedError(...)` body
with a real test as you go.

Rules for this exercise:
- Don't just test the happy path. For each test, be ready to say out loud
  what real-world bug it would catch.
- Use plain `assert` (pytest rewrites it for good failure messages, no
  need for self.assertEqual-style APIs).
- Use `pytest.raises` for the input-validation cases.
- Use `@pytest.mark.parametrize` at least once.

--- TASK LIST (implement each as a test function) ---

1. test_billing_keywords_classified_as_billing  [DONE - see below, for reference]

2. test_empty_string_returns_other_with_zero_confidence
   - classify("") and classify("   ") should both return label "other",
     confidence 0.0. (Edge case: whitespace-only input.)

3. test_non_string_input_raises_type_error
   - classify(123) and classify(None) should both raise TypeError.
     (Edge case: wrong type from an upstream caller -- e.g. someone
     passes a whole request dict by mistake.)

4. test_over_max_length_raises_value_error
   - A string longer than classifier.MAX_INPUT_LENGTH should raise
     ValueError. (Edge case: pathological/adversarial input size.)

5. test_case_insensitivity
   - "REFUND MY INVOICE" should classify identically to
     "refund my invoice".

6. test_no_keyword_match_returns_other
   - A message with no relevant keywords ("What's the weather today?")
     should return label "other", confidence 0.0.

7. [PARAMETRIZE] test_each_label_has_at_least_one_matching_example
   - Parametrize over ("billing", "technical", "account") with one
     example ticket per label, asserting classify() returns that label.

Stretch (optional, only if you have time left):
8. test_matched_keywords_is_subset_of_label_keywords
   - The returned matched_keywords should all actually belong to
     KEYWORD_LABELS[label]. This is an "the function isn't lying about
     its own output" invariant check, not a specific-example check.
"""
from pipeline.classifier import classify


def test_billing_keywords_classified_as_billing():
    result = classify("I need a refund on my last invoice")
    assert result["label"] == "billing"
    assert result["confidence"] > 0.5


def test_empty_string_returns_other_with_zero_confidence():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_non_string_input_raises_type_error():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_over_max_length_raises_value_error():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_case_insensitivity():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_no_keyword_match_returns_other():
    raise NotImplementedError("TODO: implement this test (see task list above)")


def test_each_label_has_at_least_one_matching_example():
    raise NotImplementedError("TODO: implement this test (see task list above, use parametrize)")


# STRETCH (optional): test_matched_keywords_is_subset_of_label_keywords
