# `pluss::ai::classic::prep::OneHotEncoder`

Expand each feature column into one indicator column per fitted category,
plus (by default) one more for values `map` has never seen — the classic
one-hot / dummy encoding. Output width grows with cardinality (`K` fitted
categories become `K` or `K + 1` columns per input feature), so this is the
right choice for low-to-moderate cardinality where no category is
numerically "more" than another; reach for [`OrdinalEncoder`](OrdinalEncoder.md)
instead when the categories do have a natural order, or
[`BaseNEncoder`](BaseNEncoder.md) / [`HashingEncoder`](HashingEncoder.md)
when the cardinality is too high for one column per category to be
practical.

## Signature

```cpp
explicit classic::prep::OneHotEncoder(classic::prep::OneHotEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`OneHotEncoderOptions`](OneHotEncoderOptions.md).

## State

The fitted per-feature category list, output offsets, and output widths live
inside the object, set by `learn`, read back only through `map` /
`dump_state`. `trained()` reports whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature column, counts occurrences of every non-`NaN` value, caps
  the vocabulary against `max_categories` per `overflow` (see
  [`OneHotEncoderOptions`](OneHotEncoderOptions.md#overflow)), and sorts the
  surviving values for a deterministic column order. `SkipFeature` fits an
  empty vocabulary for that feature and contributes exactly `0` output
  columns for it — `include_unknown_bucket` does not apply to a skipped
  feature, since there is no vocabulary at all to reserve an unknown slot
  next to. Fails with `Status::invalid_argument` if
  **every** feature ends up with zero output columns (nothing left to encode
  at all), or if `max_categories < 0`.

`learn(slate, options = {})`
: The single-column convenience overload — `slate` is treated as a
  one-feature batch. Equivalent to wrapping it in a
  [`FeatureBatch`](../../data/FeatureBatch.md) yourself and calling the
  overload above.

`map(batch, options = {})`
: Writes a `1.0` into the column for each row's fitted category, `0.0`
  everywhere else in that feature's block — every input feature's block is
  independent, so a row's output is the concatenation of one one-hot vector
  per input feature, in feature order. A value `map` has never seen follows
  `options.unknown_categorical` (see [`encoding/Introduction`](Introduction.md#unseen-categories-at-map-time)):
  `Error` fails the call, `MissingBucket` sets that feature's dedicated
  unknown column (only present when `include_unknown_bucket` was `true` at
  `learn` time — if it was `false`, the row is left all-zero for that
  feature instead, since there is no unknown column to set), `Ignore` always
  leaves the row all-zero for that feature. `Status::precondition` if not
  yet trained.

`map(slate, options = {})`
: The single-column convenience overload, paired with `learn(slate, ...)`.

`learn_map(batch_or_slate, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys, feature_offsets, feature_widths]`,
  following the pattern in [`scaling/Introduction`](../scaling/Introduction.md#persistence)
  — metadata now also carries the total encoded output width.

## Errors

- `max_categories < 0` — `Status::invalid_argument` from `learn`.
- Every feature reduced to zero output columns — `Status::invalid_argument`
  from `learn`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- `unknown_categorical = Error` and `map` sees an unfitted category —
  `Status::invalid_argument`.
- A `map` batch whose layout or feature count does not match what `learn`
  was fit on — `Status::shape_mismatch`.

## Practical Examples

The default fit and its one-hot layout, the three `unknown_categorical`
policies on a value `map` was never fit on, `include_unknown_bucket = false`
narrowing the output width, the `SkipFeature` overflow policy alongside
a second, unaffected feature, and the single-column `Slate` overload
proven equivalent to wrapping the same data in a `FeatureBatch`.

### Example 1

The default fit: three categories become three columns, with a fourth
unknown-bucket column appended by default.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // one feature column: categories encoded as small integers 1, 2, 3.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0, 1.0, 2.0}, {5}, f64).reshape({5, 1});

  cls::prep::OneHotEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  std::cout << "map([1, 2, 3]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([1, 2, 3]) = pluss::Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[[1, 0, 0, 0],
  [0, 1, 0, 0],
  [0, 0, 1, 0]]
```

### Example 2

The three `unknown_categorical` policies applied to the same never-seen
value (`4.0`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  cls::prep::OneHotEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({4.0}, {1}, f64).reshape({1, 1});

  cls::InferOptions err_opts{};
  err_opts.unknown_categorical = cls::UnknownCategoryPolicy::Error;
  const cls::StatusOr<pluss::Series> errored = encoder.map(probe, err_opts);
  std::cout << "Error:         ok = " << std::boolalpha << errored.ok() << "\n";

  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket: " << encoder.map(probe, bucket_opts).value() << "\n";

  cls::InferOptions ignore_opts{};
  ignore_opts.unknown_categorical = cls::UnknownCategoryPolicy::Ignore;
  std::cout << "Ignore:        " << encoder.map(probe, ignore_opts).value() << "\n";

  return 0;
}
```

Output

```text
Error:         ok = false
MissingBucket: pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0, 0, 1]]
Ignore:        pluss::Series(shape=[1,4], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 0, 0, 0]]
```

### Example 3

`include_unknown_bucket = false`: exactly `K` output columns, no reserved
unknown slot — `MissingBucket` degrades to an all-zero row since there is
nowhere to route it.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::OneHotEncoderOptions options{};
  options.include_unknown_bucket = false;
  cls::prep::OneHotEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch known{};
  known.x = pluss::make_series({2.0}, {1}, f64).reshape({1, 1});
  std::cout << "known value 2   -> " << encoder.map(known).value() << "\n";

  cls::FeatureBatch unknown{};
  unknown.x = pluss::make_series({9.0}, {1}, f64).reshape({1, 1});
  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "unknown (no bucket to route to) -> " << encoder.map(unknown, bucket_opts).value() << "\n";

  return 0;
}
```

Output

```text
known value 2   -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 1, 0]]
unknown (no bucket to route to) -> pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0, 0, 0]]
```

### Example 4

`overflow = SkipFeature` drops an over-cardinality feature's output
entirely, while a second, well-behaved feature in the same batch encodes
normally.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // feature 0: 5 distinct categories (over the cap of 2); feature 1: 2 categories.
  cls::FeatureBatch train{};
  train.x = pluss::make_series(
      {1.0, 10.0, 2.0, 20.0, 3.0, 10.0, 4.0, 20.0, 5.0, 10.0}, {10}, f64).reshape({5, 2});

  cls::prep::OneHotEncoderOptions options{};
  options.max_categories = 2;
  options.overflow = cls::prep::CategoryOverflowPolicy::SkipFeature;
  cls::prep::OneHotEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 10.0}, {2}, f64).reshape({1, 2});
  std::cout << "map([1, 10]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([1, 10]) = pluss::Series(shape=[1,3], strides=[3,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1, 0, 0]]
```

### Example 5

The single-column `Slate` overload — the same training/probe values as
Example 1, fit and mapped through `learn(slate, ...)`/`map(slate, ...)`
directly instead of wrapping them in a `FeatureBatch` first. Identical
output either way, confirming the "equivalent to wrapping it in a
`FeatureBatch` yourself" claim above.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Slate train(pluss::make_series({1.0, 2.0, 3.0, 1.0, 2.0}, {5}, f64));
  const Slate probe(pluss::make_series({1.0, 2.0, 3.0}, {3}, f64));

  cls::prep::OneHotEncoder via_slate;
  via_slate.learn(train);
  const pluss::Series slate_result = via_slate.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({1.0, 2.0, 3.0, 1.0, 2.0}, {5}, f64).reshape({5, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::OneHotEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (slate_result - batch_result).abs().amax(0, false).amax(0, false).item<double>();
  std::cout << "max abs diff between Slate and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between Slate and FeatureBatch forms = 0
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map` (`FeatureBatch` and `Slate`
  overloads), `max_categories`/`overflow`/`include_unknown_bucket`,
  `dump_state`/`load_state`.

## See Also

- [`OneHotEncoderOptions`](OneHotEncoderOptions.md) — the options struct,
  including the shared `CategoryOverflowPolicy` enum.
- [`OrdinalEncoder`](OrdinalEncoder.md) / [`BaseNEncoder`](BaseNEncoder.md) —
  narrower-output alternatives for ordered or high-cardinality categories.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.
- [`Flow::add_one_hot_encoder`](../../compose/Flow/add_stage.md) — routing
  this into a `compose::Flow`.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

OneHotEncoderOptions
```
<!-- apiref:toctree:end -->
