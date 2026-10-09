# `prep::feature` Introduction

## What This Subfamily Owns

`prep::feature` owns branch-structured preprocessing: the
[`prep::FeatureTransformer`](FeatureTransformer.md) object, the
[`prep::feature_transformer(...)`](feature_transformer.md) factory that builds
one, and the [`prep::branch`](branch.md) surface (`Branch`, `compose`, and the
one-stage builders) for describing each branch.

## Branch, Not Chain

A single [`prep::Stage`](../stage.md) attached with `Model::append` is one
transform on one route, and several of them chained run in sequence over an
evolving frame. A `FeatureTransformer` is different: each **branch** takes its
own slice of the *original* feature space and runs its own pipeline, and the
branch outputs are concatenated — left to right, in branch order — into one
matrix. Columns claimed by no branch are the **remainder**, kept or dropped by
the [remainder policy](feature_transformer.md#the-remainder-policy).

- **branch A** → `{median_income}` → impute, then scale
- **branch B** → `{ocean_proximity}` → one-hot
- **remainder** → everything else → pass through (or drop)

That whole contract is learned once and replayed as a unit.

## The Lifecycle In One Line

Build a feature transformer from two branches: a median-imputed and scaled numeric column, and a one-hot encoded categorical column. The program prints the stage count and whether it is trained. It stays untrained until a model or driver calls `learn`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame frame({
      {"median_income", Slate(pluss::make_series(std::vector<double>{2.0, 3.0, 4.0, 5.0}, {4}, cfg))},
      {"ocean_proximity", Slate(pluss::make_note({"near", "inland", "near", "bay"}))},
      {"target", Slate(pluss::make_series(std::vector<double>{0.0, 1.0, 0.0, 1.0}, {4}, cfg))},
  });

  lymph::prep::FeatureTransformer ft = lymph::prep::feature_transformer(
      frame,
      {
          lymph::prep::branch::compose(
              {lymph::prep::stage::simple_imputer({.strategy = lymph::prep::SimpleImputerStrategy::Mean}),
               lymph::prep::stage::standard_scaler()},
              {"median_income"}),
          lymph::prep::branch::one_hot_encoder({}, {"ocean_proximity"}),
      },
      lymph::prep::FeatureRemainderPolicy::PassThrough);

  std::cout << "stages = " << ft.stage_count() << "\n";
  std::cout << "trained = " << (ft.trained() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
stages = 2
trained = false
```

`feature_transformer(...)` assembles the object; the model fits it in `learn`
and replays it in `infer`. Driven directly, it is
`ft.learn_map(feature_batch)` — see the
[`FeatureTransformer` examples](FeatureTransformer.md#practical-examples).

## Related Pages

- [`prep::FeatureTransformer`](FeatureTransformer.md) — the object.
- [`prep::feature_transformer`](feature_transformer.md) — the factory.
- [`prep::branch`](branch.md) — how a branch is described.
- [`prep::stage`](../stage.md) — the stage factories a branch composes.
