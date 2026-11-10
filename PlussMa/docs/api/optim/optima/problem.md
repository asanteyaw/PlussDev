<!-- generated from problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::problem`

Starts a `ProblemBuilder` for describing an objective and constraints as one reusable value.

## Signature

```cpp
optim::ProblemBuilder optim::problem();
```

## What It Does

`problem()` returns an empty [`ProblemBuilder`](ProblemBuilder/Introduction.md). Chain the objective and constraint calls on it, then pass the builder (or its `build()`) to `optima`. The builder converts to a `ConstrainedProblem` implicitly, so no explicit `build()` is needed. Prefer it over the free-form `optima(objective, start, options, constraints)` when the same problem is solved more than once or the constraints are assembled in several places.

## Returns

An empty `ProblemBuilder`.

## Practical Examples

### Example 1

The minimum: an objective only.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("x") - 3.0).square().sum(); }), pluss::series::make({0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [3.000000]
```

### Example 2

An objective with bounds and a linear row, chained in any order. Minimise `|x − (2,2)|²` with `x₀ + x₁ ≤ 3` and `x ≥ 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series A = pluss::series::make({{1.0, 1.0}});
  const optim::OptimaState result = optim::optima(
      optim::problem()
          .objective([](const Pack &p) { return (p.at("x") - 2.0).square().sum(); })
          .lbs(pluss::series::make({0.0, 0.0}))
          .ln_ineq(A, pluss::series::make({3.0})),
      pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.500000, 1.500000]
```

### Example 3

A problem is a value: copy it, change one part and compare. The second solve adds an upper bound.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  auto base = optim::problem().objective([](const Pack &p) { return (p.at("x") - 5.0).square().sum(); });
  const optim::ConstrainedProblem free_problem = base.build();
  const optim::ConstrainedProblem capped = base.ubs(pluss::series::make({2.0})).build();
  out.writeln("free   x =", optim::optima(free_problem, pluss::series::make({0.0})).params.at("x").items<double>());
  out.writeln("capped x =", optim::optima(capped, pluss::series::make({0.0})).params.at("x").items<double>());
}
```

Output

```text
free   x = [4.999949]
capped x = [2.000000]
```

## See Also

- [`ProblemBuilder`](ProblemBuilder/Introduction.md): its methods
- [`optima`](optima.md): solves a problem
- [`constraints`](constraints.md): constraints without an objective

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="print_summary.html">
    <span class="label">Previous</span>
    <span class="title">print_summary</span>
  </a>
  <a class="next-link" href="reports_compact_summary.html">
    <span class="label">Next</span>
    <span class="title">compact_summary</span>
  </a>
</div>
<!-- apiref:nav:end -->
