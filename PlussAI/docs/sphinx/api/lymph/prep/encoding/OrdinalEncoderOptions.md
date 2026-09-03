# `lymph::options::OrdinalEncoder`

`options::OrdinalEncoder` (family alias `prep::OrdinalEncoderOptions`) configures
integer coding. It is passed to the
[`prep::OrdinalEncoder`](OrdinalEncoder.md) constructor and to the
[`prep::stage::ordinal_encoder`](../stage/ordinal_encoder.md) factory.

```cpp
struct OrdinalEncoderOptions {
  std::int64_t           max_categories = 256;
  CategoryOverflowPolicy overflow       = CategoryOverflowPolicy::KeepMostFrequent;
  bool                   include_unknown_bucket = true;
  bool                   one_based      = true;
};
```

## Fields

### `max_categories`

`std::int64_t max_categories = 256`

Cap on distinct categories tracked per column. Past this count, `overflow`
applies.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

What to do when a column has more than `max_categories` distinct values:
`KeepMostFrequent` keeps the top `max_categories` and folds the rest into the
unseen slot; `Error` rejects the column; `SkipFeature` drops it.
`lymph::prep::CategoryOverflowPolicy` is the family spelling.

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

Reserve code `0` for categories not seen at fit time. What `map` does when it
meets one is a replay-time choice on
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map).

### `one_based`

`bool one_based = true`

Codes start at `1` (with `0` = unseen). Set `false` for a dense `0..K-1` coding
— which is exactly [`options::LabelEncoder`](LabelEncoderOptions.md)'s default.

## Related Pages

- [`prep::OrdinalEncoder`](OrdinalEncoder.md) — the object these options
  configure.
- [`prep::stage::ordinal_encoder`](../stage/ordinal_encoder.md) — the stage twin.
