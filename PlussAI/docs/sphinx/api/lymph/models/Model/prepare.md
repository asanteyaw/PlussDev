<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 87824a58bc62 -->

# `lymph::models::Model::prepare`

[Back to `lymph::models::Model`](../Model.md)

`prepare` opens a fluent builder for attaching preparation stages to a model. It
is the readable front door to the same machinery [`append`](append.md) exposes
one stage at a time.

## Signature

```cpp
lymph::models::ModelPrep lymph::models::Model::prepare();

// Selected ModelPrep members (each returns *this, so calls chain):
lymph::models::ModelPrep& lymph::models::ModelPrep::scale(
    prep::Route route, prep::StandardScalerOptions options = {});
lymph::models::ModelPrep& lymph::models::ModelPrep::scale(
    std::initializer_list<std::string> names, prep::StandardScalerOptions options = {});
lymph::models::ModelPrep& lymph::models::ModelPrep::encode(
    prep::Route route, prep::OneHotEncoderOptions options = {});
lymph::models::ModelPrep& lymph::models::ModelPrep::fill(
    prep::Route route, prep::SimpleImputerOptions options = {});
lymph::models::ModelPrep& lymph::models::ModelPrep::add(
    prep::Stage stage, prep::Route route);

// Route constructors reachable from the builder:
prep::Route lymph::models::ModelPrep::names(std::vector<std::string> values) const;
prep::Route lymph::models::ModelPrep::where(
    std::function<bool(std::string_view name, std::string_view dtype)> predicate) const;
prep::Route lymph::models::ModelPrep::remaining(std::vector<std::string> already_named) const;
```

## What It Does

`prepare()` returns a `ModelPrep` — a thin accessor bound to the model. Every
call on it (`scale`, `encode`, `fill`, `add`, and the rest) records one
preparation stage plus the route it applies to, then returns the builder so the
next call can chain.

Nothing is fitted at this point. The stages are *intentions*: a route described
by column names is not yet checked against real columns, a scaler has no centre
or spread, an encoder has no vocabulary. That resolution and fitting happens
inside [`learn`](learn.md), stage by stage, in the order the calls were written —
which is why `stage_count()` reads `0` on a freshly configured model and only
becomes non-zero after training (Example 1).

The distinction between `prepare` and `append` is ergonomics, not capability.
`prepare().scale({"a"})` produces exactly the stage that
`append(prep::stage::standard_scaler(), prep::route::names({"a"}))` produces;
`prepare` just reads better when several stages are chained. Use whichever keeps
the call site clear.

Preparation attached with `prepare` becomes part of the model's contract. Once
trained, [`infer`](infer.md) and [`assess`](assess.md) replay the fitted stages
before the estimator, so callers always pass the raw, untransformed columns.

## Parameters

`prepare()` itself takes no arguments. The builder methods take two things.

### `route` — which columns a stage applies to

Every stage is aimed at a subset of the feature columns. A route is built one of
three ways from the builder:

`names({"a", "b", ...})`
: The explicit list. The most common and the easiest to review.

`where([](std::string_view name, std::string_view dtype) { ... })`
: A predicate over each column's name and dtype. Use it to target "every
  floating-point column" or "every column whose name ends in `_amount`" without
  listing them.

`remaining({"a", "b"})`
: Every feature column *except* the ones already named. Handy for a catch-all
  final stage: name the special columns, then apply a default transform to
  everything else (Example 3).

### `options` — how the stage behaves

Each builder method takes the options object for its stage kind
(`prep::StandardScalerOptions` for `scale`, `prep::OneHotEncoderOptions` for
`encode`, and so on). Those fields are documented on the corresponding options
pages; the defaults are the ordinary full-strength form of each transform.

## Returns

`prepare()` returns a `ModelPrep` bound to the model. Each builder method returns
a reference to that same `ModelPrep`, so calls chain. The stages are visible to
`learn` immediately; there is no separate "commit" step.

## Errors

- A route that names a column absent from the training frame fails when `learn`
  resolves it, not when `prepare` records it.
- Routing an encoder at a numeric column, or a scaler at a text column, is
  reported at fit time.
- Chaining on a `ModelPrep` whose model has gone out of scope is undefined; keep
  the model alive for as long as you hold its builder.

## Practical Examples

Each program builds its own frame, configures preparation, trains, and prints the
result. The output block under each is its exact captured run.

### Example 1

One scaling stage over two features on very different scales. `stage_count()`
shows the stage is only realised by `learn`, and inference takes the raw columns.

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

  // Two features on very different scales: amounts in the thousands, ratios
  // near one.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series ratio = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // prepare() opens a fluent builder that records preparation onto the model.
  // Nothing is fitted yet -- these are intentions.
  model.prepare().scale({"amount", "ratio"});
  std::cout << "stage_count before learn = " << model.stage_count() << "\n";

  // learn resolves and fits the recorded stages, then fits the estimator.
  model.learn_(x, y);
  std::cout << "stage_count after learn  = " << model.stage_count() << "\n";
  std::cout << "trained                  = " << std::boolalpha << model.trained() << "\n\n";

  // infer takes the raw, unscaled columns; the fitted scaler runs inside.
  const DataFrame review({
      {"amount", Thread(amount)},
      {"ratio", Thread(ratio)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
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

Chained stages of different kinds. `scale` the numeric column, then `encode` the
categorical one; they are fitted in written order, and `stage_count()` reads `2`.

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

  // A numeric column and a categorical one encoded as small integer codes:
  // region in {0, 1, 2}.
  const pluss::Series amount = pluss::make_series({1200.0, 1800.0, 3400.0, 4100.0, 5200.0, 6000.0}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 1, 0, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"region", Thread(region)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // Calls chain. Stages are recorded in the order written and later fitted in
  // that same order: scale the numeric column, one-hot expand the categorical
  // one.
  model.prepare()
      .scale({"amount"})
      .encode({"region"});

  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"amount", Thread(amount)},
      {"region", Thread(region)},
      {"actual", Thread(y)},
      {"predicted", Thread(model.infer(x))},
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

Route helpers. Name one column explicitly, then apply a second stage to
everything else with `remaining`. Holding the `ModelPrep` in a variable makes the
route-building calls readable.

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

  const pluss::Series a = pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0}, {6}, f64);
  const pluss::Series b = pluss::make_series({1.0, 3.0, 2.0, 5.0, 4.0, 6.0}, {6}, f64);
  const pluss::Series c = pluss::make_series({0.1, 0.2, 0.15, 0.3, 0.25, 0.35}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"a", Thread(a)}, {"b", Thread(b)}, {"c", Thread(c)}});

  lymph::models::Model model = lymph::models::logistic_regression();

  // A route can be given by explicit names, or built from a predicate over
  // (feature_name, dtype), or as "everything not already named". Here: scale
  // `a` by name, then scale every remaining column with one more stage.
  lymph::models::ModelPrep prep = model.prepare();
  prep.scale(prep.names({"a"}));
  prep.scale(prep.remaining({"a"}));

  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count()
            << "   (one for `a`, one for the rest)\n";
  std::cout << "trained     = " << std::boolalpha << model.trained() << "\n\n";

  const DataFrame review({
      {"a", Thread(a)},
      {"b", Thread(b)},
      {"c", Thread(c)},
      {"predicted", Thread(model.infer(x))},
  });
  std::cout << review;

  return 0;
}
```

Output

```text
stage_count = 2   (one for `a`, one for the rest)
trained     = true

==================================
     | a     b     c     predicted
----------------------------------
0    | 10    1     0.1   0        
1    | 20    3     0.2   0        
2    | 30    2     0.15  0        
3    | 40    5     0.3   1        
4    | 50    4     0.25  1        
5    | 60    6     0.35  1        
----------------------------------
[6 cs x 4 feat]
```

## See Also

- [append](append.md) — the one-stage-at-a-time form `prepare` wraps.
- [learn](learn.md) — resolves and fits the stages recorded here.
- [infer](infer.md) and [assess](assess.md) — replay the fitted stages.
- [`prep::StandardScaler`](../../prep/scaling/StandardScaler.md),
  [`prep::OneHotEncoder`](../../prep/encoding/OneHotEncoder.md) — the standalone
  transformers behind `scale` and `encode`.
- [Introduction](Introduction.md) — model lifecycle and cross-cutting notes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
