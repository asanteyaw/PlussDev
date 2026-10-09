# `lymph::prep::BaseNEncoder`

`BaseNEncoder` is the standalone, fittable radix encoder of the `prep` family:
it ordinal-codes each category (`1..K`, with `0` reserved for unseen or missing)
and then writes that code in radix `base` across a fixed set of digit columns.
`base = 2` is the popular **binary encoder**. `K` categories cost
`ceil(log_base(K + 1))` columns instead of one-hot's `K`. It is the object twin
of [`prep::stage::basen_encoder`](../stage/basen_encoder.md).

## Scope And Alias

The type is `lymph::prep::BaseNEncoder`. It takes an
[`options::BaseNEncoder`](BaseNEncoderOptions.md) (family alias
`prep::BaseNEncoderOptions`) at construction, default-constructed if omitted. It
shares its whole method surface with [`prep::OneHotEncoder`](OneHotEncoder.md) —
`features` / `learn` / `map` / `learn_map` / `learned` / `summary`. It has **no**
single-column `Slate` overload; use the `DataFrame` path.

## Construction

`explicit BaseNEncoder(BaseNEncoderOptions options = {});` — cheap, no data
touched.

## Transform

Fitting records the categories (codes `1..K`) and how many radix-`base` digits
`K` needs. Replay emits that many `0..base-1` columns per pinned column, named
`<name>_0 .. <name>_{d-1}`, least-significant digit first. It is the middle
ground between [`OrdinalEncoder`](OrdinalEncoder.md) (one column, but a linear
model reads the code as a magnitude) and [`OneHotEncoder`](OneHotEncoder.md)
(faithful, but `K` columns wide): a compact multi-column code a linear model can
use without the full width.

A text (note-backed) column is accepted on the `DataFrame` path — its category
identity is taken as a hash.

## `summary()`

Returns a `TransformerSummary`; for one fitted column the `state` holds four
tensors (category keys, digit layout, and bookkeeping).

## Methods

The method surface is shared verbatim with
[`prep::OneHotEncoder`](OneHotEncoder.md) — see that page for the full
signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](OneHotEncoder.md#features) | pin the categorical columns (required) |
| [`learn(frame)`](OneHotEncoder.md#learn) | record categories and digit width; returns `*this` |
| [`map(frame)`](OneHotEncoder.md#map) | emit the radix-`base` digit columns; `const` |
| [`learn_map(frame)`](OneHotEncoder.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](OneHotEncoder.md#learned) | has `learn` run yet? |
| [`summary()`](OneHotEncoder.md#summary) | fitted state as a `TransformerSummary` |

## Practical Examples

### Example 1

Binary encoding (`base = 2`): five categories in three digit columns.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"code", Slate(pluss::make_series({10., 20., 30., 40., 50., 10., 30.}, {7}, f64))}});
  const DataFrame later({{"code", Slate(pluss::make_series({50., 20., 10.}, {3}, f64))}});

  lymph::prep::BaseNEncoder encoder;  // base = 2
  std::cout << "learn_map(train) -- code_0..code_2 are the binary digits:\n"
            << encoder.features({"code"}).learn_map(train) << "\n";
  std::cout << "map(later):\n" << encoder.map(later) << "\n";

  return 0;
}
```

Output

```text
learn_map(train) -- code_0..code_2 are the binary digits:
=============================
     | code_0  code_1  code_2
-----------------------------
0    | 1       0       0     
1    | 0       1       0     
2    | 1       1       0     
3    | 0       0       1     
4    | 1       0       1     
5    | 1       0       0     
6    | 1       1       0     
-----------------------------
[7 cs x 3 feat]
map(later):
=============================
     | code_0  code_1  code_2
-----------------------------
0    | 1       0       1     
1    | 0       1       0     
2    | 1       0       0     
-----------------------------
[3 cs x 3 feat]
```

Row 0 (`code 10`) is category 1 → `001` → digits `1 0 0`; row 3 (`code 40`) is
category 4 → `100` → digits `0 0 1`.

### Example 2

A larger `base` uses fewer, wider digit columns.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train(
      {{"code", Slate(pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8., 9.}, {9}, f64))}});

  lymph::prep::BaseNEncoder encoder(lymph::prep::BaseNEncoderOptions{.base = 4});
  std::cout << "base = 4:\n"
            << encoder.features({"code"}).learn_map(train) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary.state tensors = " << s.state.size() << "\n";

  return 0;
}
```

Output

```text
base = 4:
=====================
     | code_0  code_1
---------------------
0    | 1       0     
1    | 2       0     
2    | 3       0     
3    | 0       1     
4    | 1       1     
5    | 2       1     
6    | 3       1     
7    | 0       2     
8    | 1       2     
---------------------
[9 cs x 2 feat]
summary.state tensors = 4
```

### Example 3

`map` on a later frame, including an unseen category. Codes `1..K` are the
fitted categories; `0` is reserved for unseen / missing, so an unseen value
under `MissingBucket` writes the all-zero digit row.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame train({{"code", Slate(pluss::make_series({10., 20., 30., 40., 50., 60., 70.}, {7}, f64))}});
  // 7 categories -> codes 1..7 -> ceil(log2(8)) = 3 binary digit columns.
  const DataFrame later({{"code", Slate(pluss::make_series({70., 10., 999.}, {3}, f64))}});

  lymph::prep::BaseNEncoder encoder;  // base = 2
  encoder.features({"code"}).learn(train);

  std::cout << "map(train) -- 7 categories in 3 binary digit columns:\n"
            << encoder.map(train) << "\n";

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- unseen 999 -> code 0 -> all-zero row:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
```

Output

```text
map(train) -- 7 categories in 3 binary digit columns:
=============================
     | code_0  code_1  code_2
-----------------------------
0    | 1       0       0     
1    | 0       1       0     
2    | 1       1       0     
3    | 0       0       1     
4    | 1       0       1     
5    | 0       1       1     
6    | 1       1       1     
-----------------------------
[7 cs x 3 feat]
map(later, MissingBucket) -- unseen 999 -> code 0 -> all-zero row:
=============================
     | code_0  code_1  code_2
-----------------------------
0    | 1       1       1     
1    | 1       0       0     
2    | 0       0       0     
-----------------------------
[3 cs x 3 feat]
```

## See Also

- [`prep::OneHotEncoder`](OneHotEncoder.md) — the shared method surface, in full.
- [`options::BaseNEncoder`](BaseNEncoderOptions.md) — the option fields.
- [`prep::stage::basen_encoder`](../stage/basen_encoder.md) — the factory twin.

## Changelog

- **0.1** — standalone `BaseNEncoder` with the shared encoder surface and the
  `base` radix control.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

BaseNEncoderOptions
```
<!-- apiref:toctree:end -->
