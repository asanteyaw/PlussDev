# `lymph::prep::LabelEncoder`

`LabelEncoder` is the standalone integer-coding encoder that produces a **dense
`0..K-1` code** — [`OrdinalEncoder`](OrdinalEncoder.md) with `one_based = false`
by default. It is the shape a downstream index-based step expects for a
label-style column. It is the object twin of
[`prep::stage::label_encoder`](../stage/label_encoder.md).

## Scope And Alias

The type is `lymph::prep::LabelEncoder`. It takes an
[`options::LabelEncoder`](LabelEncoderOptions.md) (family alias
`prep::LabelEncoderOptions`) at construction, default-constructed if omitted. It
shares its whole method surface with [`prep::OneHotEncoder`](OneHotEncoder.md) —
`features` / `learn` / `map` / `learn_map` / `learned` / `summary`, plus the
single-column `Thread` overloads and the `categories_` / `mapped_names_`
attributes.

## Construction

`explicit LabelEncoder(LabelEncoderOptions options = {});` — cheap, no data
touched.

## Transform

Fitting records the categories of the pinned column; replay swaps each value for
its `0`-based position. The historical name comes from encoding a target label
column; as a routed transform it simply integer-codes whatever column it is
given. Column count is unchanged; `mapped_names_` reads `<name>_label`.

The only default that differs from [`OrdinalEncoder`](OrdinalEncoder.md) is
`one_based`, which is `false` here.

## `summary()`

Returns a `TransformerSummary`; `state.back()` is the learned category list.
`categories_` / `mapped_names_` are the readable view.

## Methods

The method surface is shared verbatim with
[`prep::OneHotEncoder`](OneHotEncoder.md) — see that page for the full
signatures, semantics, and per-method examples.

| Method | Purpose |
| --- | --- |
| [`features({...})`](OneHotEncoder.md#features) | pin the categorical columns (required on the `DataFrame` path) |
| [`learn(frame)`](OneHotEncoder.md#learn) | record each column's category set; returns `*this` |
| [`map(frame)`](OneHotEncoder.md#map) | replace each value with its 0-based code; `const` |
| [`learn_map(frame)`](OneHotEncoder.md#learn_map) | `learn` then `map` in one call |
| [`learned()`](OneHotEncoder.md#learned) | has `learn` run yet? |
| [`summary()`](OneHotEncoder.md#summary) | fitted category list as a `TransformerSummary` |

Plus the single-column [`Thread` overloads](OneHotEncoder.md#single-column-overloads).

## Practical Examples

### Example 1

The single-column `Thread` overload on a text column: one `Thread` in, one
`Series` of `0..K-1` codes out.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::Thread;

  const Thread grade(pluss::make_note({"bronze", "silver", "gold", "silver", "bronze", "gold"}));

  lymph::prep::LabelEncoder encoder;
  const pluss::Series codes = encoder.learn_map(grade);
  std::cout << "codes (0-based):\n" << codes << "\n";

  std::cout << "map(new):\n"
            << encoder.map(Thread(pluss::make_note({"gold", "bronze"}))) << "\n";
  std::cout << "mapped_names_[0] = " << encoder.mapped_names_[0] << "\n";

  return 0;
}
```

Output

```text
codes (0-based):
pluss::Series(shape=[6,1], strides=[1,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2],
  [0],
  [1],
  [0],
  [2],
  [1]]
map(new):
pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [2]]
mapped_names_[0] = feature_0_label
```

The `Thread` overload names the pseudo-feature `feature_0`, so `mapped_names_`
reads `feature_0_label`; on the `DataFrame` path it would be `<column>_label`.

### Example 2

The `DataFrame` path: pin with `features()`, fit with `learn()`, replay with
`map()`. `learned()` guards it; `summary()` reports the learned category list.

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
  const DataFrame train({{"grade", Thread(pluss::make_series({10., 20., 30., 20., 10., 30.}, {6}, f64))}});
  const DataFrame later({{"grade", Thread(pluss::make_series({30., 30., 10.}, {3}, f64))}});

  lymph::prep::LabelEncoder encoder;
  std::cout << "learned before = " << std::boolalpha << encoder.learned() << "\n";

  encoder.features({"grade"}).learn(train);
  std::cout << "learned after  = " << encoder.learned() << "\n\n";

  std::cout << "map(train) -- dense 0..K-1 codes, column keeps its name:\n"
            << encoder.map(train) << "\n";
  std::cout << "map(later) -- same assignment replayed:\n" << encoder.map(later) << "\n";

  const lymph::prep::TransformerSummary s = encoder.summary();
  std::cout << "summary: " << s.feature_names[0] << ", "
            << s.state.back().numel() << " learned categories\n";

  return 0;
}
```

Output

```text
learned before = false
learned after  = true

map(train) -- dense 0..K-1 codes, column keeps its name:
============
     | grade
------------
0    | 0    
1    | 1    
2    | 2    
3    | 1    
4    | 0    
5    | 2    
------------
[6 cs x 1 feat]
map(later) -- same assignment replayed:
============
     | grade
------------
0    | 2    
1    | 2    
2    | 0    
------------
[3 cs x 1 feat]
summary: grade, 3 learned categories
```

### Example 3

`learn_map` in one call, and the shared unknown-category handling: the default
`Error` policy fails on an unseen value; `MissingBucket` routes it to a reserved
code just past the fitted range (3 categories are `0..2`, so unseen → `3`).

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
  const DataFrame train({{"grade", Thread(pluss::make_series({10., 20., 30., 20., 10., 30.}, {6}, f64))}});
  const DataFrame later({{"grade", Thread(pluss::make_series({20., 77.}, {2}, f64))}});

  lymph::prep::LabelEncoder encoder;
  std::cout << "learn_map(train):\n" << encoder.features({"grade"}).learn_map(train) << "\n";

  lymph::InferOptions to_bucket;
  to_bucket.unknown_categorical = lymph::UnknownCategoryPolicy::MissingBucket;
  std::cout << "map(later, MissingBucket) -- unseen 77 -> reserved code 3:\n"
            << encoder.map(later, to_bucket);

  return 0;
}
```

Output

```text
learn_map(train):
============
     | grade
------------
0    | 0    
1    | 1    
2    | 2    
3    | 1    
4    | 0    
5    | 2    
------------
[6 cs x 1 feat]
map(later, MissingBucket) -- unseen 77 -> reserved code 3:
============
     | grade
------------
0    | 1    
1    | 3    
------------
[2 cs x 1 feat]
```

## See Also

- [`prep::OrdinalEncoder`](OrdinalEncoder.md) — the same operation, 1-based by
  default, and the reference for the shared behaviour.
- [`prep::OneHotEncoder`](OneHotEncoder.md) — the shared method surface, in full.
- [`options::LabelEncoder`](LabelEncoderOptions.md) — the option fields.
- [`prep::stage::label_encoder`](../stage/label_encoder.md) — the factory twin.

## Changelog

- **0.1** — standalone `LabelEncoder` with the shared encoder surface, 0-based by
  default.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

LabelEncoderOptions
```
<!-- apiref:toctree:end -->
