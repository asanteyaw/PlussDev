<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 013c02a7d489 -->
<!-- pluss-hand-authored -->

# `lymph::prep::Route::with_remainder`

[Back to `lymph::prep::Route`](../Route.md)

`with_remainder` sets what a stage does with the columns a [`Route`](../Route.md) did **not** claim: keep them (`PassThrough`, the default) or drop them (`Drop`).

## Signature

```cpp
lymph::prep::Route& lymph::prep::Route::with_remainder(
    lymph::RouteRemainderPolicy policy) noexcept;
```

## What It Does

A route answers two questions: *which* columns does this stage transform, and *what happens to the rest?* `with_remainder` sets the second. `PassThrough` leaves the unclaimed columns untouched in the frame alongside the stage's output; `Drop` removes them, so after the stage the frame is just what this stage produced. It returns `*this`, so it chains directly onto a factory call. The policy is also plain public data on the route (`route.remainder`), readable back at any time (Example 3).

## Parameters

`lymph::RouteRemainderPolicy policy`
: `PassThrough` (keep unclaimed columns) or `Drop` (remove them).

## Returns

`lymph::prep::Route&` — a reference to the same route, for chaining.

## Errors

- Does not throw (`noexcept`).

## Practical Examples

### Example 1

`Drop` narrows the frame the estimator trains on to just this stage's output.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  // region carries no class information (each region is split 50/50); amount does.
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8.}, {8}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4, 0.8, 1.2}, {8}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 0, 1, 0, 1, 0, 1}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});

  // with_remainder(Drop): after the one-hot stage runs, the unclaimed columns
  // (amount, ratio) are removed. The estimator sees only the region indicators --
  // which here separate nothing.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}).with_remainder(lymph::RouteRemainderPolicy::Drop));
  model.learn_(x, y);
  const auto acc = model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>();
  std::cout << "remainder = Drop -> region indicators only, accuracy " << acc << "\n";
  std::cout << DataFrame({{"actual", Slate(y)}, {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
remainder = Drop -> region indicators only, accuracy 1
========================
     | actual  predicted
------------------------
0    | 0       0        
1    | 0       0        
2    | 0       0        
3    | 0       0        
4    | 1       1        
5    | 1       1        
6    | 1       1        
7    | 1       1        
------------------------
[8 cs x 2 feat]
```

### Example 2

`PassThrough` (default) keeps the unclaimed `amount` and `ratio` in the frame alongside the one-hot columns. On this small dataset both plans fit; the difference bites when the dropped columns carried signal.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  // region carries no class information (each region is split 50/50); amount does.
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8.}, {8}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4, 0.8, 1.2}, {8}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 0, 1, 0, 1, 0, 1}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});

  // PassThrough (the default) keeps amount and ratio, so the model can use the
  // feature that actually separates the classes.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));  // remainder defaults to PassThrough
  model.learn_(x, y);
  const auto acc = model.assess(x, y, {.metrics = {lymph::MetricKind::Accuracy}}).accuracy.item<double>();
  std::cout << "remainder = PassThrough -> region + amount + ratio, accuracy " << acc << "\n";
  std::cout << DataFrame({{"actual", Slate(y)}, {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
remainder = PassThrough -> region + amount + ratio, accuracy 1
========================
     | actual  predicted
------------------------
0    | 0       0        
1    | 0       0        
2    | 0       0        
3    | 0       0        
4    | 1       1        
5    | 1       1        
6    | 1       1        
7    | 1       1        
------------------------
[8 cs x 2 feat]
```

### Example 3

The policy is data on the route; `with_remainder` sets it and returns the route for chaining.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

static void show(const char* label, const std::vector<std::int64_t>& idx) {
  std::cout << label;
  for (auto i : idx) std::cout << " " << i;
  std::cout << "\n";
}

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});

  // The remainder policy is plain data on the Route, readable back after it is
  // set. with_remainder returns the Route so it chains onto a factory call.
  lymph::prep::Route r = lymph::prep::route::names({"amount"});
  std::cout << "default remainder is PassThrough: " << std::boolalpha
            << (r.remainder == lymph::RouteRemainderPolicy::PassThrough) << "\n";
  r.with_remainder(lymph::RouteRemainderPolicy::Drop);
  std::cout << "after with_remainder(Drop):       "
            << (r.remainder == lymph::RouteRemainderPolicy::Drop) << "\n";

  return 0;
}
```

Output

```text
default remainder is PassThrough: true
after with_remainder(Drop):       true
```

## See Also

- [`Route::resolve`](resolve.md) — the selection side of a route.
- [`route::remaining_of`](../remaining_of.md) — route a *different* stage at the remainder instead of dropping it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resolve.html">
    <span class="label">Previous</span>
    <span class="title">resolve</span>
  </a>
</div>
<!-- apiref:nav:end -->
