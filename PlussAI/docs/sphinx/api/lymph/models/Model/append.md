<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f6129865c64d -->

# `lymph::models::Model::append`

[Back to `lymph::models::Model`](../Model.md)

`append` attaches preparation steps to a model. Each step is a
[`prep::Stage`](../../prep/stage/Stage.md) — what to do — paired with a
[`prep::Route`](../../prep/route/Route.md) — which feature columns to do it to.

## Signature

```cpp
// One step.
lymph::models::Model& lymph::models::Model::append(
    lymph::prep::Stage stage,
    lymph::prep::Route route);

// Several steps in one call, applied in order.
lymph::models::Model& lymph::models::Model::append(
    std::initializer_list<std::pair<lymph::prep::Stage, lymph::prep::Route>> steps);

lymph::models::Model& lymph::models::Model::append(
    const std::vector<std::pair<lymph::prep::Stage, lymph::prep::Route>>& steps);

lymph::models::Model& lymph::models::Model::append(
    std::vector<std::pair<lymph::prep::Stage, lymph::prep::Route>>&& steps);
```

## What It Does

`append` records a `(stage, route)` pair — or a batch of them — on the model and
returns the model so calls chain. Nothing is fitted here; the steps are
intentions. [`learn`](learn.md) resolves each route against the training frame
and fits each stage in the order it was appended, transforming the features
before the next stage and finally the estimator sees them. `stage_count()` reads
`0` until `learn` has run.

### The `models` -> `prep` dependency

`append` is where the `models` family reaches into the `prep` family. It does not
define stages or routes — it consumes them:

- A **`prep::Stage`** describes a transform and carries its options. Build one
  with a `lymph::prep::stage::*` factory:
  `lymph::prep::stage::standard_scaler()`,
  `lymph::prep::stage::one_hot_encoder()`, and the rest. The
  [`prep::Stage`](../../prep/stage/Stage.md) page has the full list.
- A **`prep::Route`** describes a column selection. Build one with a
  `lymph::prep::route::*` factory: `lymph::prep::route::names({"a", "b"})`,
  `lymph::prep::route::where(predicate)`,
  `lymph::prep::route::remaining_of({...})`. See
  [`prep::Route`](../../prep/route/Route.md).

Those factories belong to `prep` and are documented there; `append` only needs
the `Stage` and `Route` values they return.

### `append` vs `prepare`

[`prepare()`](prepare.md) returns a fluent builder that wraps `append` with named
shorthands — `prepare().scale({"a"}).encode({"b"})` produces exactly the stages
that two `append` calls would. Reach for `append` when you want the stage and
route spelled out — an uncommon stage the builder has no shorthand for, or a
route computed rather than literal. Reach for `prepare` when a chain of familiar
stages reads cleaner (Example 3).

### One call for many steps

The batch overloads take a braced list (or a `std::vector`) of `{stage, route}`
pairs and apply them in order. It is equivalent to chaining `append` that many
times, but the copy-on-write detachment the model performs before a mutation
happens once for the whole batch instead of once per step (Example 4).

## Parameters

`lymph::prep::Stage stage`
: A transform descriptor from a `lymph::prep::stage::*` factory. Carries its own
  options; holds no fitted state, so the same `Stage` value may be appended to
  more than one model.

`lymph::prep::Route route`
: A column selection from a `lymph::prep::route::*` factory.

`std::initializer_list<std::pair<Stage, Route>> steps` (and the `std::vector` forms)
: A batch of `{stage, route}` pairs, applied front to back. The `std::vector&&`
  overload moves each pair's contents into the model.

## Returns

A reference to the model, so `append` calls chain and can be followed directly by
`learn_`.

## Errors

- A route naming a column that is not in the training frame fails when `learn`
  resolves it, not when `append` records it.
- A stage aimed at an incompatible column type — an encoder on a numeric column,
  a scaler on text — is reported at fit time, with the stage's position in the
  sequence.
- Appending after `learn` changes the plan but does not retrain; call `learn`
  again for the new steps to take effect.

## Practical Examples

### Example 1

One scaler step routed at two columns by name.

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // append is the explicit form: one stage descriptor, one route. The stage
  // says what to do; the route says which columns it applies to.
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}));
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count before learn = 0
stage_count after learn  = 1
trained                  = true

=======================================
     | amount  ratio  actual  predicted
---------------------------------------
0    | 1200    0.9    0       0        
1    | 1800    1.1    0       0        
2    | 3400    0.7    0       0        
3    | 4100    1.3    1       1        
4    | 5200    0.6    1       1        
5    | 6000    1.4    1       1        
---------------------------------------
[6 cs x 4 feat]
```

### Example 2

Two `append` calls route different stages to different columns: a scaler on the
numeric column, a one-hot encoder on the categorical one. They fit in call order.

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"region", Slate(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // Two append calls route different stages to different columns. They are
  // fitted in call order, each transforming the frame before the next sees it.
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount"}));
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));

  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Slate(amount)},
      {"region", Slate(region)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count = 2
trained     = true

========================================
     | amount  region  actual  predicted
----------------------------------------
0    | 1200    0       0       0        
1    | 1800    1       0       0        
2    | 3400    2       1       1        
3    | 4100    0       0       0        
4    | 5200    1       1       1        
5    | 6000    2       1       1        
----------------------------------------
[6 cs x 4 feat]
```

### Example 3

`append` and `prepare` produce the same stage. Two models built the two ways —
one `prepare().scale(...)`, one `append(standard_scaler, route::names(...))` —
train to identical predictions.

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}});

  // prepare().scale(names) is exactly append(standard_scaler, route::names).
  // Build the same one-stage model both ways and confirm they agree.
  lymph::models::Model via_prepare = lymph::models::logistic_regression();
  via_prepare.prepare().scale({"amount", "ratio"});
  via_prepare.learn_(x, y);

  lymph::models::Model via_append = lymph::models::logistic_regression();
  via_append.append(lymph::prep::stage::standard_scaler(),
                    lymph::prep::route::names({"amount", "ratio"}));
  via_append.learn_(x, y);

  const DataFrame compare({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"pred_via_prepare", Slate(via_prepare.infer(x))},
      {"pred_via_append", Slate(via_append.infer(x))},
  });
  std::cout << "stage_count: prepare=" << via_prepare.stage_count()
            << ", append=" << via_append.stage_count() << "\n\n";
  std::cout << compare;

  return 0;
}
```

Output

```text
stage_count: prepare=1, append=1

=======================================================
     | amount  ratio  pred_via_prepare  pred_via_append
-------------------------------------------------------
0    | 1200    0.9    0                 0              
1    | 1800    1.1    0                 0              
2    | 3400    0.7    0                 0              
3    | 4100    1.3    1                 1              
4    | 5200    0.6    1                 1              
5    | 6000    1.4    1                 1              
-------------------------------------------------------
[6 cs x 4 feat]
```

### Example 4

The batch overload: three different stages on three different columns, in one
`append` call.

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

  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"region", Slate(region)},
  });

  lymph::models::Model model = lymph::models::logistic_regression();

  // Bulk append: one call, a braced list of {stage, route} pairs, applied in
  // order. Equivalent to three chained append() calls but without the chain.
  model.append({
      {lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"})},
      {lymph::prep::stage::minmax_scaler(), lymph::prep::route::names({"ratio"})},
      {lymph::prep::stage::one_hot_encoder(), lymph::prep::route::names({"region"})},
  });
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Slate(amount)},
      {"ratio", Slate(ratio)},
      {"region", Slate(region)},
      {"actual", Slate(y)},
      {"predicted", Slate(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count before learn = 0
stage_count after learn  = 3
trained                  = true

===============================================
     | amount  ratio  region  actual  predicted
-----------------------------------------------
0    | 1200    0.9    0       0       0        
1    | 1800    1.1    1       0       0        
2    | 3400    0.7    2       0       0        
3    | 4100    1.3    0       1       1        
4    | 5200    0.6    1       1       1        
5    | 6000    1.4    2       1       1        
-----------------------------------------------
[6 cs x 5 feat]
```

## See Also

- [prepare](prepare.md) — the fluent builder over `append`.
- [learn](learn.md) — resolves the routes and fits the stages.
- [`prep::Stage`](../../prep/stage/Stage.md),
  [`prep::Route`](../../prep/route/Route.md) — the consumed types, and the
  factories that build them.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="assess.html">
    <span class="label">Next</span>
    <span class="title">assess</span>
  </a>
</div>
<!-- apiref:nav:end -->
