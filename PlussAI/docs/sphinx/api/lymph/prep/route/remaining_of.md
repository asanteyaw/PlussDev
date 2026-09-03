<!-- pluss-hand-authored -->

# `lymph::prep::route::remaining_of`

[Back to `prep::route`](../route.md)

`route::remaining_of` builds the **complement** of a selection: every column *not* claimed by a name list, or by another [`Route`](Route.md).

## Signature

```cpp
lymph::prep::Route lymph::prep::route::remaining_of(std::vector<std::string> names);
lymph::prep::Route lymph::prep::route::remaining_of(lymph::prep::Route other);
```

## What It Does

Both overloads resolve to "all columns, minus a claimed set", in frame order. They differ in how the claimed set is described:

- **`remaining_of(names)`** subtracts a fixed list. Use it when the claimed set is simple and static — typically "the target-like column" or "the one categorical column".
- **`remaining_of(other)`** subtracts whatever `other` selects, re-running `other`'s own logic at resolution time. Use it when the primary selection is predicate-driven or otherwise likely to change: the complement then cannot drift out of sync with it the way a duplicated name list would.

The two forms together let a plan say "scale *these*, and minmax *the rest*" with each column mentioned once (Example 4).

## Parameters

`std::vector<std::string> names`
: The claimed set to subtract (first overload).

`lymph::prep::Route other`
: A route whose selection is subtracted (second overload). It is stored and resolved lazily.

## Returns

A `lymph::prep::Route` of kind *remaining-of-names* or *remaining-of-route*. Cheap to copy.

## Errors

- The factory itself does not throw.
- If `other` selects every column, the complement resolves to an empty list (Example 3) — a stage routed at it then has nothing to do.

## Practical Examples

### Example 1

The name-list overload: everything except the listed columns.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // remaining_of(names) is the complement: every column NOT in the list.
  show("remaining_of({amount}):         ", lymph::prep::route::remaining_of(std::vector<std::string>{"amount"}).resolve(x));
  show("remaining_of({amount, region}): ", lymph::prep::route::remaining_of(std::vector<std::string>{"amount", "region"}).resolve(x));

  return 0;
}
```

Output

```text
remaining_of({amount}):          1 2
remaining_of({amount, region}):  1
```

### Example 2

The staple pattern: one-hot the categorical column, scale everything else.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // The common pattern: one-hot the categorical column, scale "everything else".
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}))
       .append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::remaining_of(std::vector<std::string>{"region"}));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | region  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       0       0        
2    | 2       0       0        
3    | 0       1       1        
4    | 1       1       1        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

### Example 3

The route overload keeps the complement tied to another route's logic. Every column here is `float64`, so the complement of a `where(float64)` route is empty.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // remaining_of(Route) keeps the complement tied to another route's logic, so a
  // predicate-driven selection and its remainder can never drift apart.
  const lymph::prep::Route numeric = lymph::prep::route::where(
      [](std::string_view, std::string_view dtype) { return dtype == "float64"; });
  show("numeric (where float64):", numeric.resolve(x));
  show("remaining_of(numeric):  ", lymph::prep::route::remaining_of(numeric).resolve(x));
  // Every column here is float64, so the remainder is empty -- exactly the
  // complement of whatever the predicate selects.

  return 0;
}
```

Output

```text
numeric (where float64): 0 1 2
remaining_of(numeric):
```

### Example 4

A route and its complement as a matched pair — `amount` and `ratio` are named once, `region` falls to the remainder automatically.

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
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series amount = pluss::make_series({1200., 1800., 3400., 4100., 5200., 6000.}, {6}, f64);
  const pluss::Series ratio  = pluss::make_series({0.9, 1.1, 0.7, 1.3, 0.6, 1.4}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"amount", Thread(amount)}, {"ratio", Thread(ratio)}, {"region", Thread(region)}});

  // A route and its remainder as a matched pair in one plan: scale the two named
  // columns, minmax the rest, with no column listed twice.
  const lymph::prep::Route scaled = lymph::prep::route::names({"amount", "ratio"});
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(), scaled)
       .append(lymph::prep::stage::minmax_scaler(), lymph::prep::route::remaining_of(scaled));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << "\n\n";
  std::cout << DataFrame({{"region", Thread(region)}, {"actual", Thread(y)},
                          {"predicted", Thread(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 2

================================
     | region  actual  predicted
--------------------------------
0    | 0       0       0        
1    | 1       0       0        
2    | 2       0       0        
3    | 0       1       1        
4    | 1       1       1        
5    | 2       1       1        
--------------------------------
[6 cs x 3 feat]
```

## See Also

- [`route::names`](names.md), [`route::where`](where.md) — the selections this complements.
- [`Route::resolve`](Route/resolve.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
