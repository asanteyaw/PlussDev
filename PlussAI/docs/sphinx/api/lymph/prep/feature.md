# Lymph `prep::feature`

`feature` is the `prep` subfamily for **branch-structured preprocessing** — one
[`FeatureTransformer`](feature/FeatureTransformer.md) object that runs several
independent, column-routed stage pipelines in parallel and concatenates their
outputs into one feature matrix.

Where a [`prep::Stage`](stage.md) is *one* transform on *one* route, and a
standalone [`scaler`](scaling.md) or [`encoder`](encoding.md) is *one* transform
you hold, a `FeatureTransformer` is the whole preprocessing contract: "scale
these columns, one-hot those, impute-then-scale that group, drop the rest" —
learned once, replayed as a unit.

## The pieces

| Entity | Role |
| --- | --- |
| [`prep::branch::Branch`](feature/branch.md) | one route + one ordered list of stages. Built with [`branch::compose`](feature/branch.md) or a one-stage builder like `branch::standard_scaler` |
| [`prep::feature_transformer(...)`](feature/feature_transformer.md) | the factory — takes the ordered feature space, a list of branches, and a [remainder policy](feature/feature_transformer.md#the-remainder-policy) |
| [`prep::FeatureTransformer`](feature/FeatureTransformer.md) | the fitted composite object — `learn` / `map` / `learn_map` / `trained` / `stage_count`, move-only |

## Shape of use

Build the same two-branch transformer as the lifecycle page, with an explicit pass-through remainder. The program prints the stage count and whether it is trained.

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

The common home for a `FeatureTransformer` is a model, via
[`Model::feature_transformer`](../models/Model/append.md) — the model fits and
replays it as the first step of `learn` / `infer`. It can also be driven
directly on a `FeatureBatch` ([`FeatureTransformer` examples](feature/FeatureTransformer.md#practical-examples)).

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

feature/Introduction
feature/FeatureTransformer
feature/feature_transformer
feature/branch
```
<!-- apiref:toctree:end -->
