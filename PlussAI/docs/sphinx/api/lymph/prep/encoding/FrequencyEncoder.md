# `lymph::prep::FrequencyEncoder`

`FrequencyEncoder` is the standalone, fittable count encoder of the `prep`
family: it replaces each category with **how often it occurred at fit time**, as
a proportion in `[0, 1]`. One numeric column in, one out, same name. It is the
object twin of [`prep::stage::frequency_encoder`](../stage/frequency_encoder.md).

## Scope And Alias

The type is `lymph::prep::FrequencyEncoder`. It takes an
[`options::FrequencyEncoder`](FrequencyEncoderOptions.md) (family alias
`prep::FrequencyEncoderOptions`) at construction, default-constructed if
omitted. It shares its whole method surface with
[`prep::OneHotEncoder`](OneHotEncoder.md) — `features` / `learn` / `map` /
`learn_map` / `learned` / `summary`, plus the single-column `Thread` overloads
and the `categories_` / `frequencies_` attributes.

## Construction

`explicit FrequencyEncoder(FrequencyEncoderOptions options = {});` — cheap, no
data touched.

## Transform

Fitting counts each category in the pinned column; replay swaps each value for
its observed relative frequency. It is a cheap way to give a model a rough
popularity signal for a high-cardinality column without the width of one-hot —
but two different categories that happen to be equally common collapse to the
same value.

An unseen category at `map` time follows
[`InferOptions::unknown_categorical`](OneHotEncoder.md#map): `Error` (default),
`MissingBucket` (which yields `unknown_frequency`), or `Ignore` (which yields
`0`).

## `summary()`

Returns a `TransformerSummary`. `frequencies_` (public attribute) holds the
fit-time count of each category, aligned with `categories_`.

## Methods

The method surface is shared verbatim with
[`prep::OneHotEncoder`](OneHotEncoder.md) — see that page for the full
signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](OneHotEncoder.md#features) | pin the categorical columns (required on the `DataFrame` path) |
| [`learn(frame)`](OneHotEncoder.md#learn) | count each column's categories; returns `*this` |
| [`map(frame)`](OneHotEncoder.md#map) | replace each value with its relative frequency; `const` |
| [`learn_map(frame)`](OneHotEncoder.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](OneHotEncoder.md#learned) | has `learn` run yet? |
| [`summary()`](OneHotEncoder.md#summary) | fitted state as a `TransformerSummary` |

Plus the single-column [`Thread` overloads](OneHotEncoder.md#single-column-overloads).

## Practical Examples

### Example 1

Each category becomes its share of the training rows.

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
  const DataFrame train({{"city", Thread(pluss::make_series({1., 1., 1., 1., 2., 2., 2., 3.}, {8}, f64))}});
  const DataFrame later({{"city", Thread(pluss::make_series({3., 2., 1.}, {3}, f64))}});

  lymph::prep::FrequencyEncoder encoder;
  std::cout << "learn_map(train):\n"
            << encoder.features({"city"}).learn_map(train) << "\n";
  std::cout << "map(later) -- fitted frequencies replayed:\n"
            << encoder.map(later) << "\n";

  return 0;
}
```

Output

```text
learn_map(train):
============
     | city 
------------
0    | 0.5  
1    | 0.5  
2    | 0.5  
3    | 0.5  
4    | 0.375
5    | 0.375
6    | 0.375
7    | 0.125
------------
[8 cs x 1 feat]
map(later) -- fitted frequencies replayed:
============
     | city 
------------
0    | 0.125
1    | 0.375
2    | 0.5  
------------
[3 cs x 1 feat]
```

### Example 2

`unknown_frequency` is the value an unseen category receives — but only when the
replay-time policy asks for the bucket.

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
  const DataFrame train({{"city", Thread(pluss::make_series({1., 1., 1., 2., 2., 3.}, {6}, f64))}});
  const DataFrame later({{"city", Thread(pluss::make_series({1., 7.}, {2}, f64))}});

  lymph::prep::FrequencyEncoder encoder(
      lymph::prep::FrequencyEncoderOptions{.unknown_frequency = 0.01});
  encoder.features({"city"}).learn(train);

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- city 7 -> unknown_frequency 0.01:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
```

Output

```text
map(later, MissingBucket) -- city 7 -> unknown_frequency 0.01:
===========
     | city
-----------
0    | 0.5 
1    | 0.01
-----------
[2 cs x 1 feat]
```

### Example 3

The caveat: two distinct categories that occur equally often collapse to the
same value, and the model can no longer tell them apart. Cities 2 and 3 each
appear twice, so both map to `0.25`.

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
  //   city 1 x4,  city 2 x2,  city 3 x2   (8 rows)
  const DataFrame train({{"city", Thread(pluss::make_series(
      {1., 1., 1., 1., 2., 2., 3., 3.}, {8}, f64))}});

  lymph::prep::FrequencyEncoder encoder;
  std::cout << "learn_map(train) -- cities 2 and 3 both -> 0.25:\n"
            << encoder.features({"city"}).learn_map(train);

  return 0;
}
```

Output

```text
learn_map(train) -- cities 2 and 3 both -> 0.25:
===========
     | city
-----------
0    | 0.5 
1    | 0.5 
2    | 0.5 
3    | 0.5 
4    | 0.25
5    | 0.25
6    | 0.25
7    | 0.25
-----------
[8 cs x 1 feat]
```

## See Also

- [`prep::OneHotEncoder`](OneHotEncoder.md) — the shared method surface, in full.
- [`options::FrequencyEncoder`](FrequencyEncoderOptions.md) — the option fields.
- [`prep::stage::frequency_encoder`](../stage/frequency_encoder.md) — the factory
  twin.

## Changelog

- **0.1** — standalone `FrequencyEncoder` with the shared encoder surface and the
  `unknown_frequency` fallback.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

FrequencyEncoderOptions
```
<!-- apiref:toctree:end -->
