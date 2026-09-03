# `lymph::prep::OrdinalEncoder`

`OrdinalEncoder` is the standalone, fittable integer-coding encoder of the
`prep` family: it learns the category set of a column and replaces each value
with a small integer code — **1-based by default**, with `0` reserved for a
category unseen at fit time. One column in, one column out, same name. It is the
object twin of [`prep::stage::ordinal_encoder`](../stage/ordinal_encoder.md).

## Scope And Alias

The type is `lymph::prep::OrdinalEncoder`. It takes an
[`options::OrdinalEncoder`](OrdinalEncoderOptions.md) (family alias
`prep::OrdinalEncoderOptions`) at construction, default-constructed if omitted.
It shares its whole method surface with
[`prep::OneHotEncoder`](OneHotEncoder.md) — `features` / `learn` / `map` /
`learn_map` / `learned` / `summary`, plus the single-column `Thread` overloads
and the `categories_` / `mapped_names_` attributes — so that page is the
reference for the mechanics. What follows is what is specific to
`OrdinalEncoder`.

## Construction

`explicit OrdinalEncoder(OrdinalEncoderOptions options = {});` — cheap, no data
touched. The option fields are on the
[`options::OrdinalEncoder`](OrdinalEncoderOptions.md) page.

## Transform

Fitting records the categories of each pinned column. Replay swaps each value
for that category's position: `1..K` by default (`0` = unseen), or `0..K-1` when
`one_based` is `false`. The column count does not change. `mapped_names_` reads
`<name>_ordinal`, but the replaced column keeps its original name.

Reach for it when a downstream step wants a compact integer code and the
categories have no order to preserve — a tree split handles the codes fine; a
linear model reads them as a magnitude, which usually calls for
[`OneHotEncoder`](OneHotEncoder.md) or [`BaseNEncoder`](BaseNEncoder.md)
instead.

## `summary()`

`OrdinalEncoder` returns a `TransformerSummary` (`feature_names` plus a `state`
tensor vector). For a single fitted column the `state` holds three tensors;
`state.back()` is the learned category list, so `state.back().numel()` is `K`.
`categories_` / `mapped_names_` are the readable view.

## Methods

The method surface is shared verbatim with
[`prep::OneHotEncoder`](OneHotEncoder.md) — see that page for the full
signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](OneHotEncoder.md#features) | pin the categorical columns (required on the `DataFrame` path) |
| [`learn(frame)`](OneHotEncoder.md#learn) | record each column's category set; returns `*this` |
| [`map(frame)`](OneHotEncoder.md#map) | replace each value with its integer code; `const` |
| [`learn_map(frame)`](OneHotEncoder.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](OneHotEncoder.md#learned) | has `learn` run yet? |
| [`summary()`](OneHotEncoder.md#summary) | fitted category list as a `TransformerSummary` |

Plus the single-column [`Thread` overloads](OneHotEncoder.md#single-column-overloads).

## Practical Examples

### Example 1

The shared lifecycle: `features().learn_map()` codes the training frame, `map()`
replays the same assignment.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Thread(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});
  const DataFrame later({{"region", Thread(pluss::make_series({30., 10., 20.}, {3}, f64))}});

  lymph::prep::OrdinalEncoder encoder;
  const DataFrame coded = encoder.features({"region"}).learn_map(train);
  std::cout << "learn_map(train):\n" << coded << "\n";
  std::cout << "map(later) -- same code assignment replayed:\n" << encoder.map(later) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.state tensors = " << s.state.size()
            << ", learned categories = " << s.state.back().numel() << "\n";
  std::cout << "mapped_names_[0] = " << encoder.mapped_names_[0] << "\n";

  return 0;
}
```

Output

```text
learn_map(train):
=============
     | region
-------------
0    | 1     
1    | 2     
2    | 3     
3    | 1     
4    | 2     
5    | 3     
-------------
[6 cs x 1 feat]
map(later) -- same code assignment replayed:
=============
     | region
-------------
0    | 3     
1    | 1     
2    | 2     
-------------
[3 cs x 1 feat]
summary.state tensors = 3, learned categories = 3
mapped_names_[0] = region_ordinal
```

### Example 2

`one_based = false` for a dense `0..K-1` coding, and the single-column `Thread`
overload.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Thread(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});

  lymph::prep::OrdinalEncoder zero_based(
      lymph::prep::OrdinalEncoderOptions{.one_based = false});
  std::cout << "one_based = false:\n"
            << zero_based.features({"region"}).learn_map(train) << "\n";

  lymph::prep::OrdinalEncoder e;
  const pluss::Series codes =
      e.learn_map(Thread(pluss::make_note({"low", "mid", "high", "mid", "low"})));
  std::cout << "Thread overload:\n" << codes << "\n";

  return 0;
}
```

Output

```text
one_based = false:
=============
     | region
-------------
0    | 0     
1    | 1     
2    | 2     
3    | 0     
4    | 1     
5    | 2     
-------------
[6 cs x 1 feat]
Thread overload:
pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[2],
  [1],
  [3],
  [1],
  [2]]
```

### Example 3

An unseen category at `map` time. `include_unknown_bucket` (default) reserves a
slot for it; the replay-time policy is
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map) — `Error` by default,
`MissingBucket` to route it to a reserved code just past the fitted range.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"region", Thread(pluss::make_series({10., 20., 30., 10., 20., 30.}, {6}, f64))}});
  const DataFrame later({{"region", Thread(pluss::make_series({30., 10., 99.}, {3}, f64))}});

  lymph::prep::OrdinalEncoder encoder;
  encoder.features({"region"}).learn(train);

  try {
    (void)encoder.map(later);
  } catch (const std::exception& ex) {
    std::cout << "default (Error) threw: " << ex.what() << "\n\n";
  }

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "MissingBucket -- unseen 99 maps to the reserved code 4:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
```

Output

```text
default (Error) threw: prep::OrdinalEncoder.map: OrdinalEncoder::map saw unknown category during map().
Triggered at: /.../lymph/common.h:122 (T pluss::ai::lymph::resolve(StatusOr<T>, std::string_view) [T = pluss::codem::Series])

MissingBucket -- unseen 99 maps to the reserved code 4:
=============
     | region
-------------
0    | 3     
1    | 1     
2    | 4     
-------------
[3 cs x 1 feat]
```

## See Also

- [`prep::OneHotEncoder`](OneHotEncoder.md) — the shared method surface, in full.
- [`options::OrdinalEncoder`](OrdinalEncoderOptions.md) — the option fields.
- [`prep::LabelEncoder`](LabelEncoder.md) — the same operation, 0-based by
  default.
- [`prep::stage::ordinal_encoder`](../stage/ordinal_encoder.md) — the factory
  twin.

## Changelog

- **0.1** — standalone `OrdinalEncoder` with the shared encoder surface and the
  `one_based` toggle.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

OrdinalEncoderOptions
```
<!-- apiref:toctree:end -->
