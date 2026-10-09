<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af55bed2debb -->
<!-- pluss-hand-authored -->

# `lymph::prep::Route::resolve`

[Back to `lymph::prep::Route`](../Route.md)

`resolve` turns a declarative [`Route`](../Route.md) into concrete column positions for a given frame.

## Signature

```cpp
std::vector<std::int64_t> lymph::prep::Route::resolve(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& ordered_names) const;

std::vector<std::int64_t> lymph::prep::Route::resolve(
    const pluss::table::DataFrame& frame) const;
```

## What It Does

The result is a list of `0`-based indices into a feature order. For a [`names`](../names.md) route it holds one index per name, in the route's list order; for [`where`](../where.md) and [`remaining_of`](../remaining_of.md) it holds the matching indices in frame order.

The two overloads differ only in which feature order the indices refer to:

- **`resolve(frame, ordered_names)`** — indices into the explicit `ordered_names` list. This is the form to use whenever Note-typed columns or a non-default `DataFrameBatchOptions::feature_names` order are in play, because the meaning of "position 2" is then exactly what you passed.
- **`resolve(frame)`** — indices into the frame's own visible (Series-only) feature order. Correct for the ordinary case where that order is what a stage will see.

## Parameters

`const pluss::table::DataFrame& frame`
: The frame the route is being applied to. Its dtypes are what a [`where`](../where.md) predicate reads.

`const std::vector<std::string>& ordered_names`
: The feature order the returned indices refer to (first overload).

## Returns

`std::vector<std::int64_t>` — the resolved column positions.

## Errors

- A [`names`](../names.md) route whose names are all absent resolves to an empty vector; it does not throw.
- A [`where`](../where.md) predicate that throws propagates the exception.

## Practical Examples

### Example 1

The two overloads agree when the explicit list matches the frame's own order.

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

  // resolve(frame) uses the frame's own column order; resolve(frame, ordered)
  // uses the list you pass. They agree when the list matches the frame.
  const lymph::prep::Route r = lymph::prep::route::names({"ratio", "region"});
  show("resolve(x):                          ", r.resolve(x));
  show("resolve(x, {amount, ratio, region}): ", r.resolve(x, {"amount", "ratio", "region"}));

  return 0;
}
```

Output

```text
resolve(x):                           1 2
resolve(x, {amount, ratio, region}):  1 2
```

### Example 2

The vector's order follows the route's own name list; each value is that name's position in the order you resolve against.

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
  const pluss::Series amount = pluss::make_series({1., 2., 3., 4., 5., 6., 7., 8.}, {8}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4, 0.8, 1.2}, {8}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2, 0, 1}, {8}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64);
  const DataFrame x({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});

  // The vector has one entry per route name, in the route's own list order; each
  // entry is that name's position in the ordered space you resolve against.
  const lymph::prep::Route r = lymph::prep::route::names({"region", "amount"});
  show("ordered = amount, ratio, region -> [pos(region), pos(amount)]:", r.resolve(x, {"amount", "ratio", "region"}));
  show("ordered = region, ratio, amount -> [pos(region), pos(amount)]:", r.resolve(x, {"region", "ratio", "amount"}));

  return 0;
}
```

Output

```text
ordered = amount, ratio, region -> [pos(region), pos(amount)]: 2 0
ordered = region, ratio, amount -> [pos(region), pos(amount)]: 0 2
```

### Example 3

One route, resolved anew for two frames whose columns sit in different positions — the reuse point.

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

  // A Route is resolved anew for every frame -- what makes it reusable across
  // folds or repeated calls.
  const lymph::prep::Route r = lymph::prep::route::names({"amount", "region"});
  const DataFrame fold_a({{"amount", Slate(amount)}, {"ratio", Slate(ratio)}, {"region", Slate(region)}});
  const DataFrame fold_b({{"ratio", Slate(ratio)}, {"region", Slate(region)}, {"amount", Slate(amount)}});
  show("fold_a:                    ", r.resolve(fold_a));
  show("fold_b (columns reordered):", r.resolve(fold_b));

  return 0;
}
```

Output

```text
fold_a:                     0 2
fold_b (columns reordered): 2 1
```

## See Also

- [`Route::with_remainder`](with_remainder.md) — the other thing a route carries.
- [`route::names`](../names.md), [`route::where`](../where.md), [`route::remaining_of`](../remaining_of.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="with_remainder.html">
    <span class="label">Next</span>
    <span class="title">with_remainder</span>
  </a>
</div>
<!-- apiref:nav:end -->
