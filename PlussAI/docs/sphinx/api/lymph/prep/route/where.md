<!-- pluss-hand-authored -->

# `lymph::prep::route::where`

[Back to `prep::route`](../route.md)

`route::where` builds a [`Route`](Route.md) from a predicate over each column's `(name, dtype)` — a selection that follows a rule rather than a fixed list.

## Signature

```cpp
lymph::prep::Route lymph::prep::route::where(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);
```

## What It Does

At [resolution](Route/resolve.md) time the predicate is called once per column in the frame's feature order; every column it returns `true` for is claimed, in that order. `dtype` is the column's element type as a lowercase string (`"float64"`, `"int64"`, ...). Use it for "every numeric column", "every name with this prefix", and similar rules that would otherwise be a hand-maintained list. The cost is that a reader must read the predicate to know what is claimed — prefer [`names`](names.md) when the set is small and stable.

## Parameters

`std::function<bool(std::string_view name, std::string_view dtype)> predicate`
: Returns `true` for a column to claim. Called per column at resolution time; keep it pure and cheap.

## Returns

A `lymph::prep::Route` of kind *where*. Cheap to copy; holds the predicate.

## Errors

- The factory itself does not throw.
- An exception thrown from the predicate propagates out of [`resolve`](Route/resolve.md).

## Practical Examples

### Example 1

A name rule and a dtype rule. Every column here is `float64`, so the type predicate claims all three.

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

  // route::where takes a predicate over (feature name, dtype). dtype is the
  // column element type as a string, e.g. "float64".
  const lymph::prep::Route by_name = lymph::prep::route::where(
      [](std::string_view name, std::string_view) { return name.rfind("r", 0) == 0; });
  show("where(name starts with r):", by_name.resolve(x));
  const lymph::prep::Route by_type = lymph::prep::route::where(
      [](std::string_view, std::string_view dtype) { return dtype == "float64"; });
  show("where(dtype == float64):  ", by_type.resolve(x));

  return 0;
}
```

Output

```text
where(name starts with r): 1 2
where(dtype == float64):   0 1 2
```

### Example 2

Route a scaler at every `price_` column by prefix — the plan does not have to name them.

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

  const pluss::Series price_usd = pluss::make_series({12., 18., 34., 41., 52., 60.}, {6}, f64);
  const pluss::Series price_eur = pluss::make_series({11., 17., 31., 39., 48., 55.}, {6}, f64);
  const pluss::Series region = pluss::make_series({0, 1, 2, 0, 1, 2}, {6}, f64);
  const pluss::Series y = pluss::make_series({0, 0, 0, 1, 1, 1}, {6}, f64);
  const DataFrame x({{"price_usd", Slate(price_usd)}, {"price_eur", Slate(price_eur)}, {"region", Slate(region)}});

  // Route a scaler at every "price_" column by prefix -- no need to list them.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::where([](std::string_view n, std::string_view) {
                 return n.rfind("price_", 0) == 0;
               }));
  model.learn_(x, y);
  std::cout << "stage_count = " << model.stage_count() << ", trained = "
            << std::boolalpha << model.trained() << "\n\n";
  std::cout << DataFrame({{"region", Slate(region)}, {"actual", Slate(y)},
                          {"predicted", Slate(model.infer(x))}});

  return 0;
}
```

Output

```text
stage_count = 1, trained = true

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

`where` and `names` can express the same selection; `names` is the one to reach for when it is this short.

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

  // where and names can describe the same selection. names reads better when the
  // set is small and stable; where earns its keep when the set follows a rule.
  const auto a = lymph::prep::route::names({"amount", "ratio"}).resolve(x);
  const auto b = lymph::prep::route::where(
      [](std::string_view n, std::string_view) { return n == "amount" || n == "ratio"; }).resolve(x);
  show("names({amount, ratio}):", a);
  show("equivalent where:      ", b);
  std::cout << "same? " << std::boolalpha << (a == b) << "\n";

  return 0;
}
```

Output

```text
names({amount, ratio}): 0 1
equivalent where:       0 1
same? true
```

## See Also

- [`route::names`](names.md) — explicit list.
- [`route::remaining_of`](remaining_of.md) — pass a `where` route to keep its complement in sync.
- [`Route::resolve`](Route/resolve.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
