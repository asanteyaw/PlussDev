# `pluss::ai::classic::prep::OneHotEncoderOptions`

The vocabulary cap, overflow policy, and unknown-bucket toggle consumed by
[`OneHotEncoder::learn`](OneHotEncoder.md) / `map`.

## Fields

### `max_categories`

`std::int64_t max_categories = 20`

The largest vocabulary `learn` will fit per feature before `overflow`
applies. `0` means unlimited (every distinct value fitted, however many
there are) — the numeric-value one-hot examples on this reference use small
synthetic vocabularies, but a real text-valued column with `max_categories =
0` can produce an enormous number of output columns; leave the default (or
set an explicit cap) for any column whose cardinality you have not checked.
Must be `>= 0`.

### `overflow`

`CategoryOverflowPolicy overflow = CategoryOverflowPolicy::KeepMostFrequent`

What happens when a feature's fitted cardinality exceeds `max_categories`.
This same three-value enum is shared by every category-vocabulary encoder in
`encoding` (`FrequencyEncoder`, `OrdinalEncoder`, `LabelEncoder`,
`BaseNEncoder`, and — via `SupervisedFitOptions` — `TargetEncoder` /
`WOEEncoder`); it is documented in full here and referenced, not repeated,
from the rest:

- `Error` — `learn` fails with `Status::invalid_argument` naming the
  encoder. The safest default for a pipeline where an unexpectedly
  high-cardinality column should stop training rather than silently degrade
  it.
- `KeepMostFrequent` (the default) — keeps the `max_categories` most common
  values (by count, ties broken by the value itself for a deterministic
  result) and drops the rest; a dropped value behaves exactly like an unseen
  category at `map` time, governed by `unknown_categorical`.
- `SkipFeature` — drops the *entire feature*, contributing zero output
  columns for it, rather than dropping just the rare categories within it.
  Useful when a handful of run-away-cardinality columns should be silently
  excluded from an otherwise-automatic pipeline instead of failing it.

### `include_unknown_bucket`

`bool include_unknown_bucket = true`

Whether the fitted output reserves one extra column per feature for
`unknown_categorical = MissingBucket` to route into. `true` (the default)
costs one extra output column per feature; set `false` when you know
`map` will only ever see fitted categories (or you are using `Error` /
`Ignore` instead of `MissingBucket`) and want the narrowest possible output.
Does not apply to a feature dropped by `overflow = SkipFeature` — see
[`OneHotEncoder`](OneHotEncoder.md)'s own `## Methods` section.

## Guidance

Leave `max_categories` at its default for a column you have not profiled;
lower it deliberately once you know the real cardinality and want to bound
memory. `overflow = KeepMostFrequent` is the right default for most
pipelines — reach for `Error` when an unexpected cardinality spike should
halt training loudly, and `SkipFeature` when a pipeline runs over many
columns automatically and a handful of pathological ones should simply drop
out rather than block the rest.

## See Also

- [`OneHotEncoder`](OneHotEncoder.md) — the consuming type.
- [`FrequencyEncoder`](FrequencyEncoder.md), [`OrdinalEncoder`](OrdinalEncoder.md),
  [`LabelEncoder`](LabelEncoder.md), [`BaseNEncoder`](BaseNEncoder.md),
  [`TargetEncoder`](TargetEncoder.md), [`WOEEncoder`](WOEEncoder.md) — every
  other encoder that reuses this same `CategoryOverflowPolicy`.
- [`encoding/Introduction`](Introduction.md) — unseen-category handling at
  `map` time.
