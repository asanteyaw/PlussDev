# `lymph::options::BaseNEncoder`

`options::BaseNEncoder` (family alias `prep::BaseNEncoderOptions`) configures
radix encoding. It is passed to the
[`prep::BaseNEncoder`](BaseNEncoder.md) constructor and to the
[`prep::stage::basen_encoder`](../stage/basen_encoder.md) factory.

```cpp
struct BaseNEncoderOptions {
  std::int64_t           base           = 2;
  std::int64_t           max_categories = 1024;
  CategoryOverflowPolicy overflow       = CategoryOverflowPolicy::KeepMostFrequent;
};
```

## Fields

### `base`

`std::int64_t base = 2`

The radix each ordinal code is written in. `2` gives a binary code —
`ceil(log2(K + 1))` columns, each `0/1`. A larger base uses fewer, wider digit
columns (`ceil(log_base(K + 1))` columns, each `0..base-1`).

### `max_categories`

`std::int64_t max_categories = 1024`

Cap on distinct categories tracked per column. Past this count, `overflow`
applies.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

`KeepMostFrequent` keeps the top `max_categories` and folds the rest into the
unseen code (`0`); `Error` rejects the column; `SkipFeature` drops it.
`lymph::prep::CategoryOverflowPolicy` is the family spelling.

## Related Pages

- [`prep::BaseNEncoder`](BaseNEncoder.md) — the object these options configure.
- [`prep::stage::basen_encoder`](../stage/basen_encoder.md) — the stage twin.
