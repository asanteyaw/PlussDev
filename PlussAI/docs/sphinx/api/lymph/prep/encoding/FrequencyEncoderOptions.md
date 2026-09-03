# `lymph::options::FrequencyEncoder`

`options::FrequencyEncoder` (family alias `prep::FrequencyEncoderOptions`)
configures count encoding. It is passed to the
[`prep::FrequencyEncoder`](FrequencyEncoder.md) constructor and to the
[`prep::stage::frequency_encoder`](../stage/frequency_encoder.md) factory.

```cpp
struct FrequencyEncoderOptions {
  std::int64_t           max_categories    = 50;
  CategoryOverflowPolicy overflow          = CategoryOverflowPolicy::KeepMostFrequent;
  double                 unknown_frequency = 0.0;
};
```

## Fields

### `max_categories`

`std::int64_t max_categories = 50`

Cap on distinct categories tracked per column. Past this count, `overflow`
applies.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

`KeepMostFrequent` keeps the top `max_categories` and folds the rest into the
unseen slot; `Error` rejects the column; `SkipFeature` drops it.
`lymph::prep::CategoryOverflowPolicy` is the family spelling.

### `unknown_frequency`

`double unknown_frequency = 0.0`

The value an unseen category receives at `map` time — but only when
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map) is `MissingBucket`.
Under the default `Error` policy an unseen category fails; under `Ignore` it
maps to `0`. Set a small prior here if a downstream model should read an unseen
category as "very rare" rather than "impossible".

## Related Pages

- [`prep::FrequencyEncoder`](FrequencyEncoder.md) — the object these options
  configure.
- [`prep::stage::frequency_encoder`](../stage/frequency_encoder.md) — the stage
  twin.
