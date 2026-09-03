# `lymph::options::OneHotEncoder`

`options::OneHotEncoder` (family alias `prep::OneHotEncoderOptions`) configures
the category budget and unknown handling for one-hot expansion. It is passed to
the [`prep::OneHotEncoder`](OneHotEncoder.md) constructor and to the
[`prep::stage::one_hot_encoder`](../stage/one_hot_encoder.md) factory.

```cpp
struct OneHotEncoderOptions {
  std::int64_t           max_categories = 20;
  CategoryOverflowPolicy overflow       = CategoryOverflowPolicy::KeepMostFrequent;
  bool                   include_unknown_bucket = true;
};
```

## Fields

### `max_categories`

`std::int64_t max_categories = 20`

The maximum number of distinct categories tracked per encoded column. Past this
count, `overflow` decides what happens. Raising it preserves more detail at the
cost of a wider output frame; lowering it caps feature-space growth.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

What to do when a column has more than `max_categories` distinct values:

| Value | Effect |
| --- | --- |
| `KeepMostFrequent` (default) | keep the most frequent `max_categories`; fold the rest into the unknown slot |
| `Error` | reject the column |
| `SkipFeature` | drop the column from the output |

`lymph::prep::CategoryOverflowPolicy` is the family spelling of the enum.

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

When `true`, `learn` reserves one extra output column — `<name>_unknown` — to
absorb categories not seen at fit time. Whether an unknown row actually lands
there is a replay-time choice on
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map); the bucket must
exist for `MissingBucket` to have somewhere to write.

## Related Pages

- [`prep::OneHotEncoder`](OneHotEncoder.md) — the object these options configure.
- [`prep::stage::one_hot_encoder`](../stage/one_hot_encoder.md) — the stage twin,
  with the transform arithmetic in full.
