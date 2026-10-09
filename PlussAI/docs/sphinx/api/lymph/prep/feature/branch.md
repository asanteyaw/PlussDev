# `lymph::prep::branch`

A **branch** is one piece of a [`FeatureTransformer`](FeatureTransformer.md): a
route (which columns) plus an ordered list of [stages](../stage.md) (what
to do with them). The `prep::branch` namespace holds the `Branch` type,
`compose` for the general case, and a family of one-stage builder shorthands.

## Signature

```cpp
// General: any stage list, routed by column names and/or positions.
lymph::prep::Branch lymph::prep::branch::compose(
    std::vector<lymph::prep::Stage> stages,
    std::vector<std::string>        feature_names = {},
    std::vector<std::int64_t>       feature_route = {});

// One-stage shorthand -- one per common transform:
lymph::prep::Branch lymph::prep::branch::standard_scaler(
    lymph::prep::StandardScalerOptions options = {},
    std::vector<std::string>           feature_names = {},
    std::vector<std::int64_t>          feature_route = {});
// ...and minmax_scaler, robust_scaler, maxabs_scaler, simple_imputer,
//    neighbor_imputer, iterative_imputer, one_hot_encoder, ordinal_encoder,
//    label_encoder, frequency_encoder, vector_normalizer, quantile_map,
//    power_map, threshold_binarizer, polynomial_lift.
```

A `Branch` is plain data — a list of `stages`, plus a route given as
`feature_names`, `feature_route` positions, or both. `lymph::prep::Branch` is
the family alias for `branch::Branch`.

## What It Does

A branch pairs a set of columns with the ordered stages to apply to them. `compose` builds a branch from any stage list, and the named builders create one-stage branches for the common transforms. The branch itself holds no fitted state.

## Parameters

`stages`
: The pipeline for this branch, run in order. Each element is a
  [`prep::Stage`](../stage.md) from a `prep::stage::*` factory. A builder
  like `branch::standard_scaler` fills this with a single stage for you.

`feature_names` / `feature_route`
: The branch's route into the transformer's ordered feature space — column
  **names**, or 0-based **positions**, or both. An empty route means the branch
  sees every feature.

## Returns

A `Branch` value — plain data, cheap to copy. Collect several into the
`branches` argument of
[`prep::feature_transformer(...)`](feature_transformer.md).

## Errors

- `compose` itself does not validate routes; a name or position outside the
  transformer's feature space is rejected when the `FeatureTransformer` is
  built.

## Which to reach for

| | |
| --- | --- |
| `branch::standard_scaler({}, {"x"})` and the other builders | the branch is exactly one stage — the common case |
| `branch::compose({s1, s2, ...}, {"x"})` | the branch is a multi-stage pipeline, **or** the stage has no dedicated builder (`stage::basen_encoder`, `stage::hashing_encoder`, `stage::target_encoder`, `stage::woe_encoder`, `stage::custom`) |

## Practical Examples

<!-- these examples establish: compose as a multi-stage pipeline, compose as the no-builder escape hatch, and routing by integer position. -->

### Example 1

`compose` with a two-stage pipeline — mean-impute, then standard-scale — on one
column. The `NaN` in row 2 is filled with the column mean (`30`) before scaling.

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
      {"x", Slate(pluss::make_series({10., 20., std::nan(""), 40., 50.}, {5}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"x"}).feature_batch();
  };

  lymph::prep::FeatureTransformer composed = lymph::prep::feature_transformer(
      raw,
      {lymph::prep::branch::compose(
          {
              lymph::prep::stage::simple_imputer(
                  {.strategy = lymph::prep::SimpleImputerStrategy::Mean}),
              lymph::prep::stage::standard_scaler(),
          },
          {"x"})},
      lymph::prep::FeatureRemainderPolicy::Drop);

  std::cout << "compose(impute -> scale):\n"
            << lymph::resolve(composed.learn_map(batch()), "composed") << "\n";

  return 0;
}
```

Output

```text
compose(impute -> scale):
pluss::Series(shape=[5,1], strides=[1,1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[[-1.4142136],
  [-0.70710678],
  [0],
  [0.70710678],
  [1.4142136]]
```

### Example 2

`compose` as the escape hatch for a stage with no dedicated builder —
`stage::basen_encoder` — alongside a builder branch. Output is the base-2 digit
columns of `code` followed by the min-max-scaled `spend`.

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
      {"code",  Slate(pluss::make_series({10., 20., 30., 40., 10., 30.}, {6}, f64))},
      {"spend", Slate(pluss::make_series({1., 2., 3., 4., 5., 6.}, {6}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"code", "spend"}).feature_batch();
  };

  lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::compose(
              {lymph::prep::stage::basen_encoder({.base = 2})}, {"code"}),
          lymph::prep::branch::minmax_scaler({}, {"spend"}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
  std::cout << "basen(code) ++ minmax(spend), shape [" << out.sizes()[0] << ", "
            << out.sizes()[1] << "]:\n" << out << "\n";

  return 0;
}
```

Output

```text
basen(code) ++ minmax(spend), shape [6, 4]:
pluss::Series(shape=[6,4], strides=[4,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data=[[1, 0, 0, 0],
  [0, 1, 0, 0.2],
  [1, 1, 0, 0.4],
  [0, 0, 1, 0.6],
  [1, 0, 0, 0.8],
  [1, 1, 0, 1]]
```

### Example 3

Routing by integer position — the third argument of `compose` — instead of
names. Positions index the transformer's ordered feature space (`a`, `b`, `c`).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
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
      {"b", Slate(pluss::make_series({10., 20., 10., 20.}, {4}, f64))},
      {"c", Slate(pluss::make_series({5., 5., 6., 6.}, {4}, f64))},
  });
  const auto batch = [&] {
    return lymph::workflow::payload::make(raw).features({"a", "b", "c"}).feature_batch();
  };

  lymph::prep::FeatureTransformer t = lymph::prep::feature_transformer(
      raw,
      {
          lymph::prep::branch::compose(
              {lymph::prep::stage::standard_scaler()}, {}, std::vector<std::int64_t>{0, 2}),
          lymph::prep::branch::compose(
              {lymph::prep::stage::one_hot_encoder()}, {}, std::vector<std::int64_t>{1}),
      },
      lymph::prep::FeatureRemainderPolicy::Drop);

  const Series out = lymph::resolve(t.learn_map(batch()), "learn_map");
  std::cout << "scale(a,c) ++ one_hot(b), shape [" << out.sizes()[0] << ", "
            << out.sizes()[1] << "]:\n" << out << "\n";

  return 0;
}
```

Output

```text
scale(a,c) ++ one_hot(b), shape [4, 5]:
pluss::Series(shape=[4,5], strides=[5,1], dtype=float64, platform=cpu, numel=20, contiguous=true)
data=[[-1.3416408, -1, 1, 0, 0],
  [-0.4472136, -1, 0, 1, 0],
  [0.4472136, 1, 1, 0, 0],
  [1.3416408, 1, 0, 1, 0]]
```

## See Also

- [`prep::feature_transformer`](feature_transformer.md) — consumes a list of
  branches.
- [`prep::FeatureTransformer`](FeatureTransformer.md) — the object they build.
- [`prep::stage`](../stage.md) — the stage factories `compose` takes.
