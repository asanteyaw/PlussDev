<!-- pluss-hand-authored -->

# `lymph::selection::GridAxis`

[Back to `selection`](../selection.md)

`GridAxis` and `GridPoint` are the **search-space coordinate types**, shared by
[`selection::search`](search.md) and the `ModelSpec`-grid tuning entry points.
An axis names one hyperparameter and lists the discrete values to sweep; a point
is one combination, with name-keyed access for the `make_model` closure.

## Signature

```cpp
std::int64_t           lymph::selection::grid_size(const std::vector<GridAxis>& axes);
GridPoint              lymph::selection::grid_point_at(std::int64_t index,
                                                      const std::vector<GridAxis>& axes);
std::vector<GridPoint> lymph::selection::grid_points(const std::vector<GridAxis>& axes);
```

## The `GridAxis` and `GridPoint` types

`GridAxis` is a plain aggregate — a `std::string name` and a
`std::vector<double> values`, the discrete points to sweep on that axis. The
`name` is what `GridPoint` lookups and trial labels key on.

`GridPoint` is one combination, returned by `grid_point_at` / `grid_points` and
handed to a `make_model` closure. It offers `double at(name)` (value on the
named axis), `double operator[](i)` (value on the i-th axis), `std::size_t
size()` (axis count), and `std::string label()` — the auto-generated
`"name=0.010000, other=1.000000"` string used in ranked boards.

## What It Does

`grid_size` is the product of every axis's value count — the total number of
points. `grid_point_at(i, axes)` decodes one index into a `GridPoint` without
materializing the rest: it is a mixed-radix decode where the **last axis varies
fastest** as `i` counts up, matching an equivalent nested-loop expansion.
`grid_points` materializes all of them in that order — reach for it only when
every point will actually be evaluated.

Values are always `double`. For a boolean or enum hyperparameter, sweep `{0.0,
1.0}` (or `{0, 1, 2}`) and convert in the closure:
`point.at("fit_intercept") != 0.0`.

An axis with an empty `values` list makes `grid_size` return `0`; a search over
such a space raises at run time.

## Parameters

`axes` — `std::vector<GridAxis>`
: The search space. Order matters only for the decode order and the label
  string.

`index` — `std::int64_t` *(`grid_point_at`)*
: `0 <= index < grid_size(axes)`.

## Returns

`grid_size` → `std::int64_t`. `grid_point_at` → one `GridPoint`. `grid_points`
→ `std::vector<GridPoint>`, length `grid_size(axes)`.

## Practical Examples

<!-- these examples establish: grid_size + grid_point_at + at()/label(), grid_points ordering, and use in a make_model closure. -->

### Example 1

`grid_size`, then `grid_point_at` to read one point by name.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 0.1, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };

  std::cout << "grid_size = " << lymph::selection::grid_size(axes) << "\n";

  const lymph::selection::GridPoint p = lymph::selection::grid_point_at(2, axes);
  std::cout << "point 2: reg=" << p.at("regularization_strength")
            << ", intercept=" << p.at("fit_intercept")
            << ", size=" << p.size() << "\n";
  std::cout << "point 2 label: " << p.label() << "\n";

  return 0;
}
```

Output

```text
grid_size = 6
point 2: reg=1, intercept=0, size=2
point 2 label: regularization_strength=1.000000, fit_intercept=0.000000
```

### Example 2

`grid_points` materializes the whole space. The last axis (`leaf`) cycles
fastest.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "depth", .values = {1.0, 2.0}},
      {.name = "leaf", .values = {5.0, 10.0, 20.0}},
  };

  const std::vector<lymph::selection::GridPoint> points =
      lymph::selection::grid_points(axes);

  std::cout << "points = " << points.size() << "\n";
  for (const auto& p : points) {
    std::cout << "  " << p.label() << "\n";
  }

  return 0;
}
```

Output

```text
points = 6
  depth=1.000000, leaf=5.000000
  depth=2.000000, leaf=5.000000
  depth=1.000000, leaf=10.000000
  depth=2.000000, leaf=10.000000
  depth=1.000000, leaf=20.000000
  depth=2.000000, leaf=20.000000
```

### Example 3

A `GridPoint` drives a `make_model` closure — the same shape
[`search::Search::run`](search/Search/run.md) uses, here iterated directly.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>
#include <vector>

int main() {
  namespace lymph = pluss::ai::lymph;

  const std::vector<lymph::selection::GridAxis> axes = {
      {.name = "regularization_strength", .values = {0.01, 1.0}},
      {.name = "fit_intercept", .values = {0.0, 1.0}},
  };

  const auto make_model = [](const lymph::selection::GridPoint& point) {
    return lymph::models::logistic_regression({
        .regularization_strength = point.at("regularization_strength"),
        .fit_intercept = point.at("fit_intercept") != 0.0,
    });
  };

  const std::int64_t total = lymph::selection::grid_size(axes);
  for (std::int64_t i = 0; i < total; ++i) {
    const lymph::selection::GridPoint point = lymph::selection::grid_point_at(i, axes);
    const lymph::models::Model m = make_model(point);
    std::cout << "cell " << i << " [" << point.label()
              << "] -> task " << static_cast<int>(m.task_kind()) << "\n";
  }

  return 0;
}
```

Output

```text
cell 0 [regularization_strength=0.010000, fit_intercept=0.000000] -> task 0
cell 1 [regularization_strength=1.000000, fit_intercept=0.000000] -> task 0
cell 2 [regularization_strength=0.010000, fit_intercept=1.000000] -> task 0
cell 3 [regularization_strength=1.000000, fit_intercept=1.000000] -> task 0
```

## See Also

- [`search::grid` / `random` / `build`](search/grid.md) — consume a
  `std::vector<GridAxis>` to build a `Search`.
- [`search::Search::run`](search/Search/run.md) — passes each `GridPoint` to
  `make_model`.
