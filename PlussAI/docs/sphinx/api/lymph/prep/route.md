# Lymph `prep::route`

`route` is the `prep` subfamily for **declarative feature selection** — the
"which columns" half of a preprocessing step, the piece that pairs with a
[`prep::Stage`](stage.md) "what".

A route is a rule, written once, that turns into concrete column positions only
when it is [`resolve`](route/Route/resolve.md)d against a real frame. Making the
selection a reusable object rather than a name list threaded through every stage
call means the intent is stated in one place, survives across cross-validation
folds and repeated calls, and is easy to compare in review.

## The three builders

All three live in the inner `lymph::prep::route` namespace and return a
[`prep::Route`](route/Route.md):

- [`route::names({...})`](route/names.md) — claim exactly the listed columns. The
  common case: explicit and stable.
- [`route::where(predicate)`](route/where.md) — claim every column whose
  `(name, dtype)` satisfies a predicate. Dynamic; follows a rule rather than a
  list.
- [`route::remaining_of({...})`](route/remaining_of.md) /
  [`route::remaining_of(other_route)`](route/remaining_of.md) — the complement:
  everything *not* claimed by a name list, or by another route.

The `Route` they produce carries two things a caller applies at the stage: the
selection itself (read with [`resolve`](route/Route/resolve.md)) and a
[remainder policy](route/Route/with_remainder.md) that says what happens to the
unclaimed columns — pass them through (the default) or drop them.

## Where it fits

The stage says what to do and the route says where to do it. This program attaches the scaler to two named columns, then trains from a dataframe that carries those columns. It prints the stage count before and after training.

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
      {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0, 60.0}, {6}, cfg))},
      {"ratio", Slate(pluss::make_series(std::vector<double>{0.1, 0.4, 0.2, 0.5, 0.3, 0.6}, {6}, cfg))},
      {"label", Slate(pluss::make_series(std::vector<double>{0, 0, 0, 1, 1, 1}, {6}, cfg))},
  });

  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  std::cout << "stages before learn = " << model.stage_count() << "\n";

  lymph::DataFrameBatchOptions options{};
  options.feature_names = {"amount", "ratio"};
  options.label_feature = "label";
  model.learn_(frame, options);
  std::cout << "stages after learn = " << model.stage_count() << "\n";
  std::cout << "trained = " << (model.trained() ? "true" : "false") << "\n";
  return 0;
}
```

Output

```text
stages before learn = 0
stages after learn = 1
trained = true
```

[`Model::append`](../models/Model/append.md) and
[`Model::prepare`](../models/Model/prepare.md) take a `Stage` and a `Route`
together; `prep::FeatureTransformer` uses a `Route` per branch. The route is
resolved lazily, when a frame is finally in hand.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 3
:hidden:

route/Introduction
route/Route
route/names
route/where
route/remaining_of
```
<!-- apiref:toctree:end -->
