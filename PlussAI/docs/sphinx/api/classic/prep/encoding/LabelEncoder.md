# `pluss::ai::classic::prep::LabelEncoder`

Mechanically identical to [`OrdinalEncoder`](OrdinalEncoder.md) — replace
each category with `base + rank`, rank taken from a sorted fitted
vocabulary — with one difference: `one_based` defaults `false` here, so the
fitted classes read `0, 1, 2, …` rather than `1, 2, 3, …`. This is the
0-indexed convention a **classification target** column is expected to
carry through `classic` (label `0` is the first class an
[`EstimatorBase`](../../estimators/EstimatorBase.md) reports), which is what
this type is for: encoding the *target*, not a feature. Encoding an ordered
*feature* column is what [`OrdinalEncoder`](OrdinalEncoder.md) — the same
mechanics, `one_based = true` by default — is for.

## Signature

```cpp
explicit classic::prep::LabelEncoder(classic::prep::LabelEncoderOptions options = {});
```

## Construction

A concrete value type, constructed directly with an optional
[`LabelEncoderOptions`](LabelEncoderOptions.md).

## State

The fitted per-feature category list lives inside the object, set by
`learn`, read back only through `map` / `dump_state`. `trained()` reports
whether `learn` has succeeded yet.

## Methods

Identical in every particular to [`OrdinalEncoder`](OrdinalEncoder.md#methods)
— `learn`/`map`/`learn_map` (`FeatureBatch` and `Thread` overloads),
`dump_state`/`load_state` — down to the shared internal fit/map helpers.
Only the type name in error messages and the `one_based` default differ.

## Errors

As [`OrdinalEncoder`](OrdinalEncoder.md#errors), with `LabelEncoder` in the
message strings in place of `OrdinalEncoder`.

## Practical Examples

The default `one_based = false` fit (0-indexed class labels), and the same
`MissingBucket`/`include_unknown_bucket` interaction
[`OrdinalEncoder`](OrdinalEncoder.md#example-3) demonstrates — shown here on
a target column to make the "this is the target, not a feature" framing
concrete.

### Example 1

Three fitted classes, ranked `0`-`2` by default — the labels an
`EstimatorBase::infer` call on a classifier trained against this target
would itself return.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  using pluss::table::DataFrame;
  using pluss::table::Thread;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // a raw text-free numeric label column, arriving as 10 / 20 / 30 rather than 0 / 1 / 2.
  const Thread raw_labels(pluss::make_series({20.0, 10.0, 30.0, 20.0}, {4}, f64));

  cls::prep::LabelEncoder encoder;
  const pluss::Series labels = encoder.learn_map(raw_labels).value();
  std::cout << "encoded labels = " << labels << "\n";

  return 0;
}
```

Output

```text
encoded labels = pluss::Series(shape=[4,1], strides=[1,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1],
  [0],
  [2],
  [1]]
```

### Example 2

`one_based = true` recovers `OrdinalEncoder`'s own default convention, for
the rare case a 1-indexed label is actually what a downstream step expects.

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

  cls::prep::LabelEncoderOptions options{};
  options.one_based = true;
  cls::prep::LabelEncoder encoder(options);
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({10.0, 20.0, 30.0}, {3}, f64).reshape({3, 1});
  std::cout << "one_based=true, map([10, 20, 30]) = " << encoder.map(probe).value() << "\n";

  return 0;
}
```

Output

```text
one_based=true, map([10, 20, 30]) = pluss::Series(shape=[3,1], strides=[1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[[1],
  [2],
  [3]]
```

### Example 3

An unseen class at `map` time under each `unknown_categorical` policy —
relevant when scoring against a held-out set that happens to introduce a
class the training labels never had.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::FeatureBatch train{};
  train.x = pluss::make_series({10.0, 20.0}, {2}, f64).reshape({2, 1});
  cls::prep::LabelEncoder encoder;
  encoder.learn(train);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({30.0}, {1}, f64).reshape({1, 1});

  cls::InferOptions err_opts{};
  err_opts.unknown_categorical = cls::UnknownCategoryPolicy::Error;
  std::cout << "Error:         ok = " << std::boolalpha << encoder.map(probe, err_opts).ok() << "\n";

  cls::InferOptions bucket_opts{};
  bucket_opts.unknown_categorical = cls::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket: " << encoder.map(probe, bucket_opts).value() << "\n";

  return 0;
}
```

Output

```text
Error:         ok = false
MissingBucket: pluss::Series(shape=[1,1], strides=[1,1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[[2]]
```

## Changelog

- **0.1** — `learn`/`map`/`learn_map` (`FeatureBatch` and `Thread`
  overloads), `max_categories`/`overflow`/`include_unknown_bucket`/
  `one_based`, `dump_state`/`load_state`.

## See Also

- [`LabelEncoderOptions`](LabelEncoderOptions.md) — the options struct.
- [`OrdinalEncoder`](OrdinalEncoder.md) — the identical mechanics, meant for
  an ordered feature column rather than a target.
- [`encoding/Introduction`](Introduction.md) — the contract shared across
  this whole sub-family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LabelEncoderOptions
```
<!-- apiref:toctree:end -->
