# `pluss::ai::classic::prep::FrequencyEncoder`

Replace each category with the fraction of fitted rows that carried it —
`count(category) / total_fitted_count` — a single numeric column per input
feature, no matter the cardinality. Reach for this over
[`OneHotEncoder`](OneHotEncoder.md) when the category count is large enough
that one column per category is wasteful and a rough "how common is this
value" signal is enough on its own; it carries no information about *which*
category a row had beyond its popularity, so two different categories with
the same fitted frequency become indistinguishable after encoding.

## Signature

```cpp
explicit classic::prep::FrequencyEncoder(classic::prep::FrequencyEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`FrequencyEncoderOptions`](FrequencyEncoderOptions.md).

## State

The fitted per-feature category list and matching frequency list live inside
the object, set by `learn`, read back only through `map` / `dump_state`.
`trained()` reports whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: For each feature, fits a vocabulary the same way
  [`OneHotEncoder::learn`](OneHotEncoder.md) does (counts, caps against
  `max_categories` per `overflow`), then converts each surviving category's
  count into a frequency by dividing by the **total count of fitted rows
  for that feature after any `overflow`-driven drop** — a category dropped
  by `KeepMostFrequent` does not count toward the denominator either, so the
  remaining categories' frequencies still sum to `1.0` among themselves. A
  feature with zero non-`NaN` values fits an empty vocabulary (every value
  maps to the unknown-category fallback at `map` time).

`learn(thread, options = {})`
: The single-column convenience overload.

`map(batch, options = {})`
: Replaces each fitted category with its frequency. A value `map` has never
  seen follows `options.unknown_categorical`: `Error` fails the call,
  `MissingBucket` uses `unknown_frequency` from the options this encoder was
  constructed with, `Ignore` maps to `0.0` — the same value a category with
  zero fitted occurrences would carry, so `Ignore` is indistinguishable from
  "a valid but vanishingly rare category" by design.

`map(thread, options = {})`
: The single-column convenience overload.

`learn_map(batch_or_thread, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys, freq_sizes, flat_freqs]` — two parallel
  ragged arrays (categories and their frequencies) rather than the single
  flat array most other encoders use, since `FrequencyEncoder` is the only
  one whose per-category state is itself a floating-point value rather than
  an index or bit-cast key.

## Errors

- `max_categories < 0` — `Status::invalid_argument` from `learn`.
- `unknown_frequency` outside `[0, 1]` — `Status::invalid_argument` from
  `learn`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- `unknown_categorical = Error` and `map` sees an unfitted category —
  `Status::invalid_argument`.

## Practical Examples

The default fit's frequencies, the three `unknown_categorical` policies
(including how `Ignore` and a genuinely rare fitted category produce the
same output), a non-default `unknown_frequency`, and the single-column
`Thread` overload proven equivalent to a `FeatureBatch`.

### Example 1

Five rows, three categories — frequencies `2/5`, `2/5`, `1/5`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 1.0, 2.0, 3.0}, {5}, f64).reshape({5, 1});

  cls::prep::FrequencyEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  std::cout << "map([1, 2, 3]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([1, 2, 3]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0.4],
  [0.4],
  [0.2]]
```

### Example 2

The three `unknown_categorical` policies on a never-seen value, with
`unknown_frequency` left at its default `0.0` (so `MissingBucket` and
`Ignore` happen to agree here).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 1.0}, {3}, f64).reshape({3, 1});
  cls::prep::FrequencyEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({9.0}, {1}, f64).reshape({1, 1});

  cls::InferOptions err_opts{};
  err_opts.unknown_categorical = cls::UnknownCategoryPolicy::Error;
  std::cout << "Error:         ok = " << std::boolalpha << encoder.map(probe, err_opts).ok() << "\n";

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
MissingBucket: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
Ignore:        pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[0]]
```

### Example 3

`unknown_frequency` set away from its default — `MissingBucket` now
diverges from `Ignore`, giving an unseen value a deliberately distinctive
signal instead of blending in at `0.0`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({1.0, 2.0, 1.0}, {3}, f64).reshape({3, 1});

  cls::prep::FrequencyEncoderOptions options{};
  options.unknown_frequency = 1.0;  // "treat an unseen category as maximally common"
  cls::prep::FrequencyEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({9.0}, {1}, f64).reshape({1, 1});
  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "unknown_frequency=1.0, MissingBucket -> " << encoder.map(probe, bucket_opts).value() << "\n";

  return 0;
}
```

Output

```text
unknown_frequency=1.0, MissingBucket -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[1]]
```

### Example 4

The single-column `Thread` overload — the same training/probe values as
Example 1, fit and mapped through `learn(thread, ...)`/`map(thread, ...)`
directly instead of a `FeatureBatch`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Thread train(pluss::make_series({1.0, 2.0, 1.0, 2.0, 3.0}, {5}, f64));
  const Thread probe(pluss::make_series({1.0, 2.0, 3.0}, {3}, f64));

  cls::prep::FrequencyEncoder via_thread;
  via_thread.learn(train);
  const pluss::Series thread_result = via_thread.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({1.0, 2.0, 1.0, 2.0, 3.0}, {5}, f64).reshape({5, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});

  cls::prep::FrequencyEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (thread_result - batch_result).abs().amax(0, false).item<double>();
  std::cout << "max abs diff between Thread and FeatureBatch forms = " << max_abs_diff << "\n";

  return 0;
}
```

Output

```text
max abs diff between Thread and FeatureBatch forms = 0
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map` (`FeatureBatch` and `Thread`
  overloads), `max_categories`/`overflow`/`unknown_frequency`,
  `dump_state`/`load_state`.

## See Also

- [`FrequencyEncoderOptions`](FrequencyEncoderOptions.md) — the options
  struct.
- [`OneHotEncoder`](OneHotEncoder.md) — the wider-output alternative that
  preserves category identity, not just popularity.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

FrequencyEncoderOptions
```
<!-- apiref:toctree:end -->
