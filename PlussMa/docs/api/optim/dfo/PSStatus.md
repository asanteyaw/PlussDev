<!-- generated from PSStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSStatus`

Why a pattern-search run stopped.

`Converged` means the mesh fell below `mesh_tolerance` (or the step and value tests were met). `ReachedIterationBudget`, `ReachedEvaluationBudget` and `ReachedTimeBudget` mean `max_iterations`, `max_evaluations` or `time_limit_seconds` passed first. `InfeasibleStationary` means the run stopped at a point that violates the constraints and from which no feasible direction improved the violation: the problem may have no feasible point.

## Fields

### `Converged`

`enumerator`, default `0`.

The mesh converged.

### `ReachedIterationBudget`

`enumerator`, default `1`.

`max_iterations` ran out.

### `ReachedEvaluationBudget`

`enumerator`, default `2`.

`max_evaluations` ran out.

### `ReachedTimeBudget`

`enumerator`, default `3`.

`time_limit_seconds` passed.

### `InfeasibleStationary`

`enumerator`, default `4`.

Stopped at an infeasible point with no feasible improving direction.

## Guidance

Read from `PSOutcome::status`.

## Practical Examples

### Example 1

Converged and out of iterations. The default limit is 100 iterations per variable, which the Rosenbrock function exhausts; with a larger limit the mesh converges.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::PSConfig few, many;
  few.max_iterations = 10;
  many.max_iterations = 5000;
  out.writeln("5000 iterations:", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), many).status));
  out.writeln("default        :", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).status));
  out.writeln("10 iterations  :", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), few).status));
}
```

Output

```text
5000 iterations: Converged
default        : ReachedIterationBudget
10 iterations  : ReachedIterationBudget
```

### Example 2

Out of evaluations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::PSConfig few;
  few.max_evaluations = 50;
  out.writeln(dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), few).status));
}
```

Output

```text
ReachedEvaluationBudget
```

### Example 3

Infeasible. The two constraints `x ≥ 2` and `x ≤ 1` contradict each other.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  dfo::PSConfig config;
  config.max_iterations = 4000;
  config.nonlinear_constraints = [](const Series &x) {
    dfo::PSConstraintValues v;
    v.inequality = (x * -1.0 + 2.0).reshape({1}).unite({(x - 1.0).reshape({1})}, 0);
    return v;
  };
  out.writeln(dfo::to_string(dfo::pattern_search_minimizer([](const Series &x) { return x.square().sum(); }, pluss::series::make({0.0}), config).status));
}
```

Output

```text
InfeasibleStationary
```

## See Also

- [`PSOutcome`](PSOutcome.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSSearchStrategy.html">
    <span class="label">Previous</span>
    <span class="title">PSSearchStrategy</span>
  </a>
  <a class="next-link" href="ParticleSwarmConfig.html">
    <span class="label">Next</span>
    <span class="title">ParticleSwarmConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
