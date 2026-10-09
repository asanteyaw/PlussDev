# `pluss::ai::classic::prep::OrdinalEncoder`

Replace each category with its rank among the fitted vocabulary, sorted by
the category's own value — a single integer-valued column per input
feature. Reach for this when the categories already carry a natural order
you want preserved numerically (small/medium/large-style values already
encoded as, say, `1.0`/`2.0`/`3.0`); a model reading the output can treat
"rank 3" as meaningfully greater than "rank 1" the way it cannot for
[`OneHotEncoder`](OneHotEncoder.md)'s indicator columns.

## Signature

```cpp
explicit classic::prep::OrdinalEncoder(classic::prep::OrdinalEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`OrdinalEncoderOptions`](OrdinalEncoderOptions.md).

## State

The fitted per-feature category list lives inside the object, set by
`learn`, read back only through `map` / `dump_state`. `trained()` reports
whether `learn` has succeeded yet.

## Methods

`trained()` — `bool`
: `true` once `learn` has succeeded at least once.

`learn(batch, options = {})`
: Fits a vocabulary per feature the same way
  [`OneHotEncoder::learn`](OneHotEncoder.md) does (count, cap against
  `max_categories` per `overflow`, sort). The fitted rank of a category is
  its position in that sorted vocabulary — rank `0` is the smallest fitted
  value, not the most frequent one.

`learn(slate, options = {})`
: The single-column convenience overload.

`map(batch, options = {})`
: Replaces each fitted category with `base + rank`, where `base` is `1` if
  `one_based` (the default) else `0`. A value `map` has never seen follows
  `options.unknown_categorical`: `Error` fails the call, `MissingBucket`
  maps to `base + K` (one past the last fitted rank) **only if
  `include_unknown_bucket` is `true`** — otherwise the row is `NaN`, the
  same as `Ignore` produces. `Ignore` always maps to `NaN`.

`map(slate, options = {})`
: The single-column convenience overload.

`learn_map(batch_or_slate, learn_options = {}, infer_options = {})`
: `learn` then `map` on the same data, in one call.

`dump_state()` / `load_state(state)`
: `[metadata, key_sizes, flat_keys]`, following the pattern in
  [`scaling/Introduction`](../scaling/Introduction.md#persistence).

## Errors

- `max_categories < 0` — `Status::invalid_argument` from `learn`.
- `overflow = Error` and a feature's cardinality exceeds `max_categories` —
  `Status::invalid_argument` from `learn`.
- `map`/`learn_map` before a successful `learn` — `Status::precondition`.
- `unknown_categorical = Error` and `map` sees an unfitted category —
  `Status::invalid_argument`.

## Practical Examples

The default (one-based) rank fit, `one_based = false`, the
`MissingBucket`/`include_unknown_bucket` interaction that leaves a row
`NaN` when there is no reserved unknown rank to fall back to, and the
single-column `Slate` overload proven equivalent to a `FeatureBatch`.

### Example 1

Four fitted categories, sorted by value, ranked `1`-`4` by default.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // categories arrive out of order; fitted rank follows sorted value, not arrival order.
  cls::FeatureBatch train{};
  train.x = pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64).reshape({4, 1});

  cls::prep::OrdinalEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});
  std::cout << "map([10, 20, 30, 40]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
map([10, 20, 30, 40]) = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [2],
  [3],
  [4]]
```

### Example 2

`one_based = false` shifts every rank down by one, to a `0`-indexed scale.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});

  cls::prep::OrdinalEncoderOptions options{};
  options.one_based = false;
  cls::prep::OrdinalEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "one_based=false, map([10, 20, 30]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
one_based=false, map([10, 20, 30]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[0],
  [1],
  [2]]
```

### Example 3

An unseen value under `MissingBucket`: a reserved unknown rank when
`include_unknown_bucket` is `true`, `NaN` when it is `false` — the same
`NaN` `Ignore` always produces.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({99.0}, {1}, f64).reshape({1, 1});
  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;

  cls::prep::OrdinalEncoder with_bucket;  // include_unknown_bucket = true (default)
  with_bucket.learn(train);
  std::cout << "include_unknown_bucket=true,  MissingBucket -> "
            << with_bucket.map(probe, bucket_opts).value() << "\n";

  cls::prep::OrdinalEncoderOptions no_bucket_opts{};
  no_bucket_opts.include_unknown_bucket = false;
  cls::prep::OrdinalEncoder without_bucket(no_bucket_opts);
  without_bucket.learn(train);
  std::cout << "include_unknown_bucket=false, MissingBucket -> "
            << without_bucket.map(probe, bucket_opts).value() << "\n";

  return 0;
}
```

Output

```text
include_unknown_bucket=true,  MissingBucket -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[4]]
include_unknown_bucket=false, MissingBucket -> pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[nan]]
```

### Example 4

The single-column `Slate` overload — the same training/probe values as
Example 1, fit and mapped through `learn(slate, ...)`/`map(slate, ...)`
directly instead of a `FeatureBatch`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::Slate;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  const Slate train(pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64));
  const Slate probe(pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64));

  cls::prep::OrdinalEncoder via_slate;
  via_slate.learn(train);
  const pluss::Series slate_result = via_slate.map(probe).value();

  cls::FeatureBatch train_batch{};
  train_batch.x = pluss::make_series({30.0, 10.0, 40.0, 20.0}, {4}, f64).reshape({4, 1});
  cls::FeatureBatch probe_batch{};
  probe_batch.x = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {4}, f64).reshape({4, 1});

  cls::prep::OrdinalEncoder via_batch;
  via_batch.learn(train_batch);
  const pluss::Series batch_result = via_batch.map(probe_batch).value();

  const double max_abs_diff = (slate_result - batch_result).abs().amax(0, false).item<double>();
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
  overloads), `max_categories`/`overflow`/`include_unknown_bucket`/
  `one_based`, `dump_state`/`load_state`.

## See Also

- [`OrdinalEncoderOptions`](OrdinalEncoderOptions.md) — the options struct.
- [`LabelEncoder`](LabelEncoder.md) — the same rank encoding, `one_based`
  defaulting `false`, meant for a target column rather than a feature.
- [`OneHotEncoder`](OneHotEncoder.md) — the right choice when no order
  should be implied.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

OrdinalEncoderOptions
```
<!-- apiref:toctree:end -->
