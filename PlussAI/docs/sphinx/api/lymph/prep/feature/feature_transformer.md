# `lymph::prep::feature_transformer`

`feature_transformer` assembles a [`prep::FeatureTransformer`](FeatureTransformer.md)
from an ordered feature space, a list of [branches](branch.md), and a remainder
policy. It is the entry point for the [`prep::feature`](../feature.md) subfamily.

## Signature

```cpp
// (a) from an explicit ordered name list
lymph::prep::FeatureTransformer lymph::prep::feature_transformer(
    const std::vector<std::string>&            feature_order,
    const std::vector<lymph::prep::Branch>&    branches,
    lymph::prep::FeatureRemainderPolicy        remainder = lymph::prep::FeatureRemainderPolicy::PassThrough);

// (b) from a frame -- feature_order = frame.features()
lymph::prep::FeatureTransformer lymph::prep::feature_transformer(
    const pluss::table::DataFrame&            frame,
    const std::vector<lymph::prep::Branch>&   branches,
    lymph::prep::FeatureRemainderPolicy       remainder = lymph::prep::FeatureRemainderPolicy::PassThrough);

// (c) from a frame + explicit batch options -- feature_order resolved from data_options
lymph::prep::FeatureTransformer lymph::prep::feature_transformer(
    const pluss::table::DataFrame&           frame,
    const lymph::options::DFBatch&           data_options,
    const std::vector<lymph::prep::Branch>&  branches,
    lymph::prep::FeatureRemainderPolicy      remainder = lymph::prep::FeatureRemainderPolicy::PassThrough);
```

Each form also accepts `branches` as a braced `{...}` list or any range, and the
name-list form accepts a range of names.

## What It Does

`feature_transformer` builds a feature transformer from an ordered feature space and a list of branches. The remainder policy decides what happens to features no branch covers. The three forms differ only in where the feature order comes from: an explicit list, a frame's features, or a frame with batch options.

## Parameters

`feature_order` / `frame` / `frame` + `data_options`
: The ordered feature space the branch routes index into. From a bare frame it
  is `frame.features()`; with `data_options` it is the resolved
  `data_options.feature_names` (so you can restrict or reorder, and set
  `note_policy` for categorical columns).

`branches`
: One [`prep::Branch`](branch.md) per pipeline — from
  [`branch::compose`](branch.md) or a one-stage builder such as
  `branch::standard_scaler`. Applied in the given order; their outputs
  concatenate left to right.

`remainder`
: What to do with routed columns no branch claims. **Defaults to
  `PassThrough`** here — note the [`FeatureTransformer`](FeatureTransformer.md#construction)
  constructor defaults to `Drop`.

## Returns

A fitted-on-demand `lymph::prep::FeatureTransformer`. Move-only. Hand it to a
model with [`Model::feature_transformer`](../../models/Model/feature_transformer.md) or drive
it directly with `learn_map` on a `FeatureBatch`.

## Errors

- A branch route naming a column absent from `feature_order` is rejected when
  the transformer is built.
- Fitting errors (from a branch's own stages) surface through the `Status` /
  `StatusOr` returned by `learn` / `map`; wrap with `lymph::resolve(...)`.

## The remainder policy

### `FeatureRemainderPolicy` values

| Value | Numeric value | Description |
|---|---|---|
| `Drop` | `0` | — |
| `PassThrough` | `1` | — |

The **remainder** is every column in `feature_order` that no branch routes to.
`Drop` leaves it out of the output; `PassThrough` appends it unchanged, after
all branch outputs. See [`FeatureTransformer` Example 2](FeatureTransformer.md#practical-examples).

## Practical Examples

<!-- these examples establish: the frame short form, name-list / frame equivalence, and the data_options + model form. -->

### Example 1

The frame short form: `feature_order` is taken from the frame, two branches,
`Drop` remainder.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"income",  Slate(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"region",  Slate(pluss::make_series({0., 1., 0., 1.}, {4}, f64))},
      {"balance", Slate(pluss::make_series({10., 20., 30., 40.}, {4}, f64))},
  });

  lymph::prep::FeatureTransformer transformer = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::standard_scaler({}, {"income", "balance"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const lymph::FeatureBatch batch =
      lymph::workflow::payload::make(raw).features({"income", "region", "balance"}).feature_batch();
  const Series out = lymph::resolve(transformer.learn_map(batch), "learn_map");

  std::cout << "stage_count = " << transformer.stage_count()
            << ", output shape = [" << out.sizes()[0] << ", " << out.sizes()[1] << "]\n\n";
  std::cout << out << "\n";

  return 0;
}
```

Output

```text
stage_count = 2, output shape = [4, 5]

pluss::Series(shape=[4,5], strides=[5,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[-1.3416408, -1.3416408, 1, 0, 0],
  [-0.4472136, -0.4472136, 0, 1, 0],
  [0.4472136, 0.4472136, 1, 0, 0],
  [1.3416408, 1.3416408, 0, 1, 0]]
```

### Example 2

The name-list form (c) and the frame form (b) build the same transformer given
the same branches.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame raw({
      {"a", Slate(pluss::make_series({1., 2., 3., 4.}, {4}, f64))},
      {"b", Slate(pluss::make_series({40., 30., 20., 10.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"a", "b"}).feature_batch();
  };
  auto branches = [] {
    return std::vector<lymph::prep::branch::Branch>{
        lymph::prep::branch::minmax_scaler({}, {"a", "b"})};
  };

  lymph::prep::FeatureTransformer from_frame = lymph::prep::feature_transformer(raw, branches());
  lymph::prep::FeatureTransformer from_names = lymph::prep::feature_transformer(
      std::vector<std::string>{"a", "b"}, branches(),
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  const Series o1 = lymph::resolve(from_frame.learn_map(batch()), "f1");
  const Series o2 = lymph::resolve(from_names.learn_map(batch()), "f2");

  std::cout << "from frame:\n" << o1 << "\n";
  std::cout << "from name list:\n" << o2 << "\n";
  std::cout << "identical = " << std::boolalpha
            << (o1.sub(o2).abs().max().item<double>() == 0.0) << "\n";

  return 0;
}
```

Output

```text
from frame:
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0, 1],
  [0.33333333, 0.66666667],
  [0.66666667, 0.33333333],
  [1, 0]]
from name list:
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[0, 1],
  [0.33333333, 0.66666667],
  [0.66666667, 0.33333333],
  [1, 0]]
identical = true
```

### Example 3

Form (c) — frame plus `options::DFBatch` — handed to a model. `data_options`
also carries the label feature the model trains against.

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
  const DataFrame train({
      {"income",  Slate(pluss::make_series({1.0, 1.5, 2.0, 3.5, 4.0, 4.5}, {6}, f64))},
      {"region",  Slate(pluss::make_series({0., 1., 0., 1., 0., 1.}, {6}, f64))},
      {"target",  Slate(pluss::make_series({0., 0., 0., 1., 1., 1.}, {6}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "region"};
  data_options.label_feature = "target";

  lymph::prep::FeatureTransformer preprocessing = lymph::prep::feature_transformer(
      train, data_options,
      {
          lymph::prep::branch::standard_scaler({}, {"income"}),
          lymph::prep::branch::one_hot_encoder({}, {"region"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  lymph::models::Model model = lymph::models::logistic_regression();
  model.feature_transformer(std::move(preprocessing));
  model.learn_(train, data_options);

  std::cout << "trained = " << std::boolalpha << model.trained() << "\n";
  std::cout << DataFrame({
      {"income",    Slate(train.features("income"))},
      {"region",    Slate(train.features("region"))},
      {"actual",    Slate(train.features("target"))},
      {"predicted", Slate(model.infer(train, data_options))},
  });

  return 0;
}
```

Output

```text
trained = true
========================================
     | income  region  actual  predicted
----------------------------------------
0    | 1       0       0       0        
1    | 1.5     1       0       0        
2    | 2       0       0       0        
3    | 3.5     1       1       1        
4    | 4       0       1       1        
5    | 4.5     1       1       1        
----------------------------------------
[6 cs x 4 feat]
```

## See Also

- [`prep::FeatureTransformer`](FeatureTransformer.md) — the object this returns.
- [`prep::branch`](branch.md) — building the `branches` argument.
- [`prep::stage`](../stage.md) — the stage factories a branch composes.
- [`Model::feature_transformer`](../../models/Model/feature_transformer.md) — attaching one to
  a model.
