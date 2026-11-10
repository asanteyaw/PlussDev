<!-- generated from MultiObjectiveScalarization.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::MultiObjectiveScalarization`

How several objectives are reduced to one value to minimise.

`WeightedSum` minimises `Σ wᵢ fᵢ` with `objective_weights`. `Chebyshev` minimises the largest weighted gap to a reference point, `max wᵢ (fᵢ − rᵢ)`, using `objective_weights` and `objective_reference`. On a non-convex trade-off a weighted sum can only reach the end points of the front, never a compromise in the middle; Chebyshev can. Whatever the scalarisation, the non-dominated points are kept in the Pareto archive.

## Fields

### `WeightedSum`

`enumerator`, default `0`.

`Σ wᵢ fᵢ`.

### `Chebyshev`

`enumerator`, default `1`.

`max wᵢ (fᵢ − rᵢ)` against a reference point.

## Guidance

Set as `MadsBlackboxAdvancedConfig::multiobjective_scalarization`.

## Practical Examples

### Example 1

A non-convex front: `f₁ = x`, `f₂ = 1 − x²` on `[0, 1]`. A weighted sum with weights `(0.6, 0.4)` is concave in `x`, so a local search ends at an end point (here `x = 1`); Chebyshev finds the balanced compromise `x = 0.618`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    const Series a = x.select(0, 0);
    e.objectives = a.reshape({1}).unite({(1.0 - a.square()).reshape({1})}, 0);
    return e;
  };
  for (const auto mode : {dfo::MultiObjectiveScalarization::WeightedSum, dfo::MultiObjectiveScalarization::Chebyshev}) {
    dfo::MadsBlackboxAdvancedConfig config;
    config.lower_bounds = pluss::series::make({0.0});
    config.upper_bounds = pluss::series::make({1.0});
    config.multiobjective_scalarization = mode;
    config.objective_weights = mode == dfo::MultiObjectiveScalarization::WeightedSum ? pluss::series::make({0.6, 0.4}) : pluss::series::make({1.0, 1.0});
    config.objective_reference = pluss::series::make({0.0, 0.0});
    config.max_evaluations = 3000;
    const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.8}), config);
    out.writeln(static_cast<int>(mode), ": x =", o.minimizer.items<double>(), ", objectives =", o.objective_vector.items<double>());
  }
}
```

Output

```text
0 : x = [1.0000] , objectives = [1.0000, 0.0000]
1 : x = [0.6180] , objectives = [0.6180, 0.6180]
```

### Example 2

Weights decide the compromise of a convex problem: `f₁ = (x − 1)²`, `f₂ = (x − 3)²`. The weighted-sum minimiser is `(w₁·1 + w₂·3)/(w₁ + w₂)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    const Series a = x.select(0, 0);
    e.objectives = (a - 1.0).square().reshape({1}).unite({(a - 3.0).square().reshape({1})}, 0);
    return e;
  };
  for (const double w : {0.2, 0.5, 0.8}) {
    dfo::MadsBlackboxAdvancedConfig config;
    config.objective_weights = pluss::series::make({w, 1.0 - w});
    config.max_evaluations = 1500;
    out.writeln("weight on f1", w, ": x =", dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.0}), config).minimizer.items<double>());
  }
}
```

Output

```text
weight on f1 0.200 : x = [2.600]
weight on f1 0.500 : x = [2.000]
weight on f1 0.800 : x = [1.400]
```

### Example 3

The default is the weighted sum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("default is WeightedSum =", dfo::MadsBlackboxAdvancedConfig{}.multiobjective_scalarization == dfo::MultiObjectiveScalarization::WeightedSum);
}
```

Output

```text
default is WeightedSum = true
```

## See Also

- [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MadsPollScheme.html">
    <span class="label">Previous</span>
    <span class="title">MadsPollScheme</span>
  </a>
  <a class="next-link" href="NelderMeadConfig.html">
    <span class="label">Next</span>
    <span class="title">NelderMeadConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
