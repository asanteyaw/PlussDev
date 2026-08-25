# `lymph::ColumnRouter`

## Summary

`ColumnRouter` describes how feature columns are routed into stages or workflow
branches.

Use it when different feature subsets should follow different preprocessing paths.

## What It Solves

Real tabular preprocessing is rarely "apply one thing to every column."

It is usually closer to:

- standardize the numeric columns
- one-hot encode the categoricals
- pass some columns through untouched
- define a remainder branch for "everything else"

If that routing logic lives only as loose name lists near fit calls, it becomes
hard to audit and easy to drift. `ColumnRouter` turns routing itself into an
explicit reusable object.

## Main Construction Styles

`ColumnRouter` is usually built through:

- `lymph::columnrouter::names(...)`
- `lymph::columnrouter::where(...)`
- `lymph::columnrouter::remaining_of(...)`

The important split is:

- `names(...)` when the route is explicit and stable
- `where(...)` when the route is derived from column metadata
- `remaining_of(...)` when one branch should consume everything not already
  claimed

## Signature

```cpp
lymph::ColumnRouter lymph::columnrouter::names(
    std::vector<std::string> names);
lymph::ColumnRouter lymph::columnrouter::where(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);
lymph::ColumnRouter lymph::columnrouter::remaining_of(
    std::vector<std::string> names);
std::vector<std::int64_t> lymph::ColumnRouter::resolve(
    const pluss::table::DataFrame& frame,
    const std::vector<std::string>& ordered_names) const;
```

## Practical Examples

This page uses the exact verifier program
`lymph_prep_column_router_docverify_demo.cpp`.

### Example 1

This example shows both the explicit-name route and the predicate-plus-remainder
route in one executable so you can compare them directly.

```cpp
#include <pluss/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string join_i64(const std::vector<std::int64_t>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) out << ", ";
    out << values[i];
  }
  out << "]";
  return out.str();
}

} // namespace

int main() {
  namespace lymph = pluss::ai::lymph;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.4, 1.8, 2.2}, {4}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.6, 0.9}, {4}, f64))},
      {"late", Thread(pluss::make_series({0, 0, 0, 0}, {4}, f64))},
      {"region", Thread(pluss::make_note({"north", "west", "north", "south"}))},
  });

  const std::vector<std::string> ordered_names =
      {"income", "balance", "late", "region"};

  const lymph::ColumnRouter named =
      lymph::columnrouter::names({"income", "balance"});
  const lymph::ColumnRouter numeric = lymph::columnrouter::where(
      [](std::string_view, std::string_view dtype) {
        return dtype == "float64";
      });
  const lymph::ColumnRouter categorical =
      lymph::columnrouter::names({"region"});
  const lymph::ColumnRouter remainder =
      lymph::columnrouter::remaining_of(categorical);

  std::cout << "== names ==\n";
  std::cout << "named=" << join_i64(named.resolve(raw, ordered_names)) << "\n";

  std::cout << "== where_and_remainder ==\n";
  std::cout << "numeric=" << join_i64(numeric.resolve(raw, ordered_names)) << "\n";
  std::cout << "categorical="
            << join_i64(categorical.resolve(raw, ordered_names)) << "\n";
  std::cout << "remainder="
            << join_i64(remainder.resolve(raw, ordered_names)) << "\n";

  return 0;
}
```

Output

```text
== names ==
named=[0, 1]
== where_and_remainder ==
numeric=[0, 1, 2]
categorical=[3]
remainder=[0, 1, 2]
```

Read that output carefully:

- `names(...)` is exact positional routing of the requested features
- `where(...)` resolved all `float64` columns in the ordered feature list
- `remaining_of(categorical)` captured the complement of the categorical route

That is the normal `ColumnRouter` mental model:

- construction describes intent
- `resolve(...)` turns that intent into concrete feature positions against a
  real frame

## Related Pages

- [Stage](Stage.md)
- [FeatureTransformer](FeatureTransformer.md)
- [Workflow](../model/Workflow.md)
