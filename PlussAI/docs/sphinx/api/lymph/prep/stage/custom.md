<!-- pluss-hand-authored -->

# `lymph::prep::stage::custom`

[Back to `prep::stage`](../stage_family.md)

`custom` wraps any `Series -> Series` callable as a stateless [`CustomStage`](Stage.md). It is the escape hatch for a one-off derived feature the built-in factories do not cover.

## Signature

```cpp
lymph::prep::CustomStage lymph::prep::stage::custom(
    std::string name,
    Fn map_fn,                               // any callable: Series -> Series
    lymph::prep::TransformContract contract = { /* stateless defaults */ },
    lymph::options::FunctionTransformer options = {});
```

## What It Does

The callable is applied to the feature frame at both fit and inference, with no learned state in between. A custom stage carries no route, so it is attached with [`Model::add`](../../models/Model.md) — not `append` — and it maps the whole feature frame it receives; compose it with routed built-in stages in the same plan. Keep the callable pure and deterministic: it runs unchanged on every later frame, and anything it derives from the data it sees is invisible to persistence.

## Parameters

`std::string name`
: A label for the stage, used in diagnostics. Must be non-empty.

`Fn map_fn`
: The transform. Anything invocable as `Series(const Series&)` (returning `Series` or `StatusOr<Series>`).

`lymph::prep::TransformContract contract`
: Behavioural metadata (see [`Stage::contract`](Stage.md#contract)); the default describes a stateless, lossy, non-reversible transform.

`lymph::options::FunctionTransformer options`
: Rarely needed; passed to the underlying function transformer.

## Returns

`lymph::prep::CustomStage` — a distinct type from `prep::Stage`, attached via `Model::add`.

## Errors

- The factory throws if `name` is empty.
- `Model::add` throws if the stage has no callable.
- An error raised inside the callable surfaces when the owning plan is fitted.

## Practical Examples

### Example 1

A `log1p` compressor for a positive, skewed column, attached with `Model::add`.

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

  // prep::stage::custom wraps any Series -> Series callable as a stateless stage.
  // It is attached with Model::add (no route -- it maps the whole feature frame).
  // Here: a log1p compressor for a positive, skewed column.
  const pluss::Series amount = pluss::make_series({0., 1., 9., 99., 999., 9999.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("log1p", [](const pluss::Series& s) { return s.log1p(); }));
  model.learn_(x, y);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"amount", Thread(amount)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
trained = true

================================
     | amount  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       0       0        
2    | 9       0       0        
3    | 99      1       1        
4    | 999     1       1        
5    | 9999    1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 2

The callable closes over configuration — here the clip bounds — so one factory call yields a fully configured stage.

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

  // The callable can close over configuration -- here the clip bounds -- so one
  // factory call produces a fully configured stage.
  const pluss::Series v = pluss::make_series({-100., -3., 0., 4., 50., 900.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"v", Thread(v)}});

  const double lo = -5.0, hi = 10.0;
  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("clip", [lo, hi](const pluss::Series& s) {
    return s.clamp_min(lo).clamp_max(hi);
  }));
  model.learn_(x, y);
  std::cout << "trained = " << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"v", Thread(v)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
trained = true

==============================
     | v     actual  predicted
------------------------------
0    | -100  0       0        
1    | -3    0       0        
2    |  0    0       0        
3    |  4    1       1        
4    |  50   1       1        
5    |  900  1       1        
------------------------------
[6 cs x 3 feat]
```

### Example 3

A custom `sqrt` compressor runs first (whole frame), then a routed `standard_scaler` — custom and built-in stages in one plan.

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

  // A custom sqrt compressor combined with a built-in stage. Route the scaler at
  // the column; the custom stage runs first over the whole (one-column) frame.
  const pluss::Series amount = pluss::make_series({1., 4., 16., 64., 256., 1024.}, {6}, f64);
  const pluss::Series y = pluss::make_series({0,0,0,1,1,1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}});

  lymph::models::Model model = lymph::models::logistic_regression();
  model.add(lymph::prep::stage::custom("sqrt", [](const pluss::Series& s) { return s.sqrt(); }))
       .append(lymph::prep::stage::standard_scaler(), lymph::prep::route::names({"amount"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"amount", Thread(amount)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | amount  actual  predicted
--------------------------------
0    | 1       0       0        
1    | 4       0       0        
2    | 16      0       0        
3    | 64      1       1        
4    | 256     1       1        
5    | 1024    1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`Model::add`](../../models/Model.md) — attaches a `CustomStage`.
- [`prep::stage::threshold_binarizer`](threshold_binarizer.md), [`prep::stage::polynomial_lift`](polynomial_lift.md) — built-in stateless shapers.
- `prep::FunctionTransformer` — the object the callable becomes.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
