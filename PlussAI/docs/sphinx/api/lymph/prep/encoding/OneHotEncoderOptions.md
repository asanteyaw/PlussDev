# `lymph::prep::OneHotEncoderOptions`

`OneHotEncoderOptions` controls category budget and unknown handling for
one-hot expansion.

## Field Reference

### `std::int64_t max_categories = 20`

This is the maximum explicit category budget per encoded feature.

Larger values preserve more detail. Smaller values constrain feature-space
explosion more aggressively.

### `lymph::prep::CategoryOverflowPolicy overflow = KeepMostFrequent`

This controls what happens when observed category count exceeds the configured
budget.

Use `KeepMostFrequent` when you want a pragmatic cap. Use stricter policies
when feature growth must fail loudly instead of being compacted.

### `bool include_unknown_bucket = true`

If `true`, unseen categories encountered later can map into a learned unknown
bucket.

This is usually the more inference-robust choice.

## Related Pages

- [OneHotEncoder](OneHotEncoder.md)
- [../stage/one_hot_encoder.md](../stage/one_hot_encoder.md)
