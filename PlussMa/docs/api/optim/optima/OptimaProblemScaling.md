<!-- generated from OptimaProblemScaling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaProblemScaling`

Whether `optima` rescales the problem before solving: not at all, by the start point, or the objective and each constraint.

Tolerances are absolute numbers, so a problem written in awkward units (an objective in the millions, a constraint in micro-units) is judged unfairly. With `ObjectiveAndConstraints` (the default of the algorithms' options) a rescaled copy is solved: the objective and each constraint row are multiplied by a constant chosen from their gradient at the start (the objective is only ever scaled down, and a function whose gradient is already between 1e-2 and 1e2 is left alone), and the answer, residuals and multipliers are reported back in your units. `InitialPoint` scales the variables by their size at the start. `None` solves the problem as written. Bounds are never scaled. [`optima_problem_scaling_name`](optima_problem_scaling_name.md) names a value.

## Fields

### `None`

`enumerator`, default `0`.

Solve as written.

### `InitialPoint`

`enumerator`, default `1`.

Scale the variables by their size at the start.

### `ObjectiveAndConstraints`

`enumerator`, default `2`.

Solve a rescaled copy; report in the caller's units (default).

## Guidance

Set as `problem_scaling` in the sequential, working-set or interior options.

## Practical Examples

### Example 1

An objective in awkward units converges either way and returns the same point; the scaled run matches the exact value `1e6 · (√5 − 1)²` more closely.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series target = pluss::series::make({1.0, 2.0});

  const auto big = [&](const pluss::Series &x) { return 1.0e6 * (x - target).square().sum(); };
  for (const optim::OptimaProblemScaling scaling : {optim::OptimaProblemScaling::None, optim::OptimaProblemScaling::InitialPoint, optim::OptimaProblemScaling::ObjectiveAndConstraints}) {
    optim::OptimaOptions options;
    options.sequential_qp.problem_scaling = scaling;
    const optim::OptimaState result = optim::optima(big, pluss::series::make({0.0, 0.0}), options, optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; }));
    out.writeln(optim::optima_problem_scaling_name(scaling), ": exit =", optim::optima_exit_flag_name(result.exit_flag), ", objective =", result.evaluation.value.item<double>());
  }
}
```

Output

```text
None : exit = Converged , objective = 1527863.58
InitialPoint : exit = Converged , objective = 1527863.58
ObjectiveAndConstraints : exit = Converged , objective = 1527864.04
```

### Example 2

Scaling is invisible in the result: the reported objective is in your units either way.

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

  for (const optim::OptimaProblemScaling scaling : {optim::OptimaProblemScaling::None, optim::OptimaProblemScaling::ObjectiveAndConstraints}) {
    optim::OptimaOptions options;
    options.sequential_qp.problem_scaling = scaling;
    out.writeln(optim::optima_problem_scaling_name(scaling), ": objective =", optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc).evaluation.value.item<double>());
  }
}
```

Output

```text
None : objective = 1.527864
ObjectiveAndConstraints : objective = 1.527864
```

### Example 3

The interior algorithm takes the same setting.

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
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.problem_scaling = optim::OptimaProblemScaling::None;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = Converged , iterations = 7 , evaluations = 13
```

## See Also

- [`SequentialQPOptions`](SequentialQPOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaOutput.html">
    <span class="label">Previous</span>
    <span class="title">OptimaOutput</span>
  </a>
  <a class="next-link" href="OptimaProgressSnapshot.html">
    <span class="label">Next</span>
    <span class="title">OptimaProgressSnapshot</span>
  </a>
</div>
<!-- apiref:nav:end -->
