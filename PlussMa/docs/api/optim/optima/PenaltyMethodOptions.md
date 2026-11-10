<!-- generated from PenaltyMethodOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::PenaltyMethodOptions`

Settings of the penalty (merit) algorithm.

Used when `OptimaOptions::algorithm` is `MeritMethod`, through `OptimaOptions::penalty`. The method minimises the objective plus `penalty` times a measure of the constraint violation by gradient steps with a backtracking line search, and raises the weight until the constraints hold to `constraint_tolerance`. It is the simplest algorithm and a reasonable first try on small problems; it needs more iterations than the others for tight accuracy.

## Fields

### `max_iterations`

`std::int64_t`, default `100`.

The iteration budget.

### `max_line_search_iterations`

`std::int64_t`, default `20`.

The most step reductions per iteration.

### `initial_step_size`

`double`, default `0.1`.

The first trial step length.

### `backtracking`

`double`, default `0.5`.

The factor by which a rejected step is multiplied.

### `armijo`

`double`, default `1e-4`.

The sufficient-decrease constant.

### `penalty`

`double`, default `10.0`.

The starting weight of the violation measure; increased as needed.

### `constraint_tolerance`

`double`, default `1e-6`.

The largest violation accepted as feasible.

### `optimality_tolerance`

`double`, default `1e-6`.

The stationarity accepted as optimal.

### `project_bounds`

`bool`, default `true`.

Clip iterates back inside the bounds.

### `progress_hook`

`std::function<bool(const optim::OptimaProgressSnapshot &)>`, default `empty`.

Called each iteration; return `true` to stop.

## Guidance

Set through `OptimaOptions::penalty`.

## Practical Examples

### Example 1

A larger starting penalty reaches feasibility sooner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  for (const double penalty : {1.0, 100.0}) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::MeritMethod;
    options.penalty.penalty = penalty;
    const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
    out.writeln("penalty", penalty, ": iterations =", result.iteration, ", violation =", result.evaluation.max_constraint_violation);
  }
}
```

Output

```text
penalty 1.000000 : iterations = 28 , violation = 0.000000
penalty 100.000000 : iterations = 12 , violation = 0.000000
```

### Example 2

An iteration budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::MeritMethod;
  options.penalty.max_iterations = 5;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = MaxIterations , iterations = 5 , evaluations = 12
```

### Example 3

Tolerances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::MeritMethod;
  options.penalty.constraint_tolerance = 1e-9;
  options.penalty.optimality_tolerance = 1e-9;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
exit = Converged , iterations = 24 , evaluations = 25
x = [0.447214, 0.894427]
```

## See Also

- [`OptimaOptions`](OptimaOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaState.html">
    <span class="label">Previous</span>
    <span class="title">OptimaState</span>
  </a>
  <a class="next-link" href="ProblemBuilder.html">
    <span class="label">Next</span>
    <span class="title">ProblemBuilder</span>
  </a>
</div>
<!-- apiref:nav:end -->
