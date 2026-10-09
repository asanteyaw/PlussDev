<!-- pluss-hand-authored -->

# `lymph::prep::route::names`

[Back to `prep::route`](../route.md)

`route::names` builds a [`Route`](Route.md) that claims exactly the columns you name — the common, explicit case.

## Signature

```cpp
lymph::prep::Route lymph::prep::route::names(std::vector<std::string> names);
```

## What It Does

The returned route, when [resolved](Route/resolve.md) against a frame, produces one index per name, in the order the names were given, each being that name's position in the frame's feature order. A name that is not present in the resolution order simply contributes nothing. Reach for it whenever the feature subset is fixed and worth reading literally in review; use [`where`](where.md) when the subset should follow a rule instead.

## Parameters

`std::vector<std::string> names`
: The columns to claim, in the order they should appear in the resolved index list. Duplicates and absent names are tolerated (absent names drop out).

## Returns

A `lymph::prep::Route` of kind *names*. Cheap to copy; holds no frame reference.

## Errors

- The factory itself does not throw.
- Resolution against a frame that is missing a named column does not throw either — that name is skipped.

## Practical Examples

### Example 1

Resolving a route to concrete positions. `names` order is preserved in the result; each value is a position in the frame.

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

  // route::names claims exactly the listed columns, in the frame's feature order.
  // resolve turns the rule into concrete positions.
  const lymph::prep::Route r = lymph::prep::route::names({"amount", "region"});
  show("names({amount, region}) resolves to:", r.resolve(x));
  show("names({ratio}) resolves to:         ", lymph::prep::route::names({"ratio"}).resolve(x));

  return 0;
}
```

Output

```text
names({amount, region}) resolves to: 0 2
names({ratio}) resolves to:          1
```

### Example 2

The everyday use: name the columns each stage should touch.

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

  // The everyday use: name the columns a stage should touch.
  lymph::models::Model model = lymph::models::logistic_regression();
  model.append(lymph::prep::stage::standard_scaler(),
               lymph::prep::route::names({"amount", "ratio"}))
       .append(lymph::prep::stage::one_hot_encoder(),
               lymph::prep::route::names({"region"}));
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
stage_count = 2, trained = true

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

One `Route` value, two frames with the columns in different orders. The rule picks the same columns; only their positions change.

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

  // A Route is a value: build it once, resolve it against as many frames as you
  // like, in whatever column order each frame happens to have.
  const lymph::prep::Route r = lymph::prep::route::names({"region", "amount"});
  show("against x (amount, ratio, region): ", r.resolve(x));
  const DataFrame x2({{"region", Slate(region)}, {"amount", Slate(amount)}, {"ratio", Slate(ratio)}});
  show("against x2 (region, amount, ratio):", r.resolve(x2));
  std::cout << "the selected columns are the same; only their positions differ.\n";

  return 0;
}
```

Output

```text
against x (amount, ratio, region):  2 0
against x2 (region, amount, ratio): 0 1
the selected columns are the same; only their positions differ.
```

## See Also

- [`route::where`](where.md) — rule-driven selection.
- [`route::remaining_of`](remaining_of.md) — the complement.
- [`Route::resolve`](Route/resolve.md) — turn a route into positions.
- [`Model::append`](../../models/Model/append.md).

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
