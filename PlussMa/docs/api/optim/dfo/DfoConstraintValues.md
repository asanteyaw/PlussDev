<!-- generated from DfoConstraintValues.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DfoConstraintValues`

The two kinds of nonlinear constraint values a constraint callback returns.

Returned by the callback in `DfoProblem::constraints_values`. `inequality` holds `g(x)` and the point is feasible when every element is `≤ 0`; `equality` holds `h(x)` and the point is feasible when every element is `0`. Either may be left undefined when there are none of that kind. Both may be scalars or vectors of residuals. Equality constraints are supported by pattern search only; the mesh-adaptive routes accept inequalities.

## Fields

### `inequality`

`pluss::Series`, default `undefined`.

Values `g(x)` that must be `≤ 0`.

### `equality`

`pluss::Series`, default `undefined`.

Values `h(x)` that must be `0` (pattern search only).

## Guidance

Returned by `DfoProblem::constraints_values`.

## Practical Examples

### Example 1

Inequality and equality together. Minimise `Σ (xᵢ − 3)²` subject to `x₀ + x₁ ≤ 4` and `x₀ − x₁ = 0`: the answer is `(2, 2)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.0, 0.0});
  problem.objective = f;
  problem.constraints_values = [](const Series &x) {
    dfo::DfoConstraintValues c;
    c.inequality = x.sum() - 4.0;
    c.equality = x.select(0, 0) - x.select(0, 1);
    return c;
  };
  const dfo::DfoResult result = dfo::DFO::solve(problem);
  out.writeln("minimizer =", result.minimizer.items<double>(), ", solver =", dfo::to_string(result.selected_solver));
}
```

Output

```text
minimizer = [2.0000, 2.0000] , solver = PatternSearch
```

### Example 2

Only inequalities. The equality member stays undefined.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.0, 0.0});
  problem.objective = f;
  problem.constraints_values = [](const Series &x) { dfo::DfoConstraintValues c; c.inequality = x.sum() - 2.0; return c; };
  const dfo::DfoResult result = dfo::DFO::solve(problem);
  out.writeln("minimizer =", result.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.0007, 0.9993]
```

### Example 3

A vector of inequalities: the box `0 ≤ xᵢ ≤ 2` written as four rows `[−x₀, −x₁, x₀ − 2, x₁ − 2] ≤ 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.0, 0.0});
  problem.objective = f;
  problem.constraints_values = [](const Series &x) {
    dfo::DfoConstraintValues c;
    c.inequality = (-x).unite({x - 2.0}, 0);
    return c;
  };
  const dfo::DfoResult result = dfo::DFO::solve(problem);
  out.writeln("minimizer =", result.minimizer.items<double>());
}
```

Output

```text
minimizer = [2.0000, 2.0000]
```

## See Also

- [`DfoProblem`](DfoProblem.md): holds the callback

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DfoBuilder.html">
    <span class="label">Previous</span>
    <span class="title">DfoBuilder</span>
  </a>
  <a class="next-link" href="DfoDiagnostics.html">
    <span class="label">Next</span>
    <span class="title">DfoDiagnostics</span>
  </a>
</div>
<!-- apiref:nav:end -->
