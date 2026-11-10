<!-- generated from SeptimaResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaResult`

What `septima` returns: the answer blocks, the objective, the stopping measures and the exit flag.

`params` holds the answer, one `Series` per initial block with its original shape and dtype. `objective_value` is the objective at the answer (`objective` is the same as a `Series`). `gradient_inf_norm` is the largest component of the objective's own gradient; `stationarity` is the same for the augmented Lagrangian with components blocked by a bound removed, which is what the gradient tolerance is judged on; `step_inf_norm` is the last step; `constraint_violation` is the largest violation. `iteration` and `function_evaluations` count the work. `converged`, `exit_flag` and `message` say how it ended, and `state` carries the optimizer's memory and the final multipliers.

## Fields

### `params`

`std::vector<pluss::Series>`, default `empty`.

The answer, one block per initial block.

### `state`

`optim::SeptimaState`, default `{}`.

Optimizer memory and multipliers.

### `objective`

`pluss::Series`, default `undefined`.

The objective at the answer.

### `iteration`

`std::int64_t`, default `0`.

Iterations performed.

### `function_evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `objective_value`

`double`, default `+inf`.

The objective at the answer as a number.

### `gradient_inf_norm`

`double`, default `+inf`.

Largest component of the objective's gradient.

### `stationarity`

`double`, default `+inf`.

Largest component of the augmented-Lagrangian gradient, bound-blocked components removed.

### `step_inf_norm`

`double`, default `0`.

Largest component of the last step.

### `constraint_violation`

`double`, default `+inf`.

The largest constraint violation.

### `converged`

`bool`, default `false`.

Whether a stopping test was met.

### `exit_flag`

`optim::SeptimaExitFlag`, default `MaxIterations`.

Why the run stopped.

### `message`

`std::string`, default `empty`.

A sentence about the ending.

## Guidance

Check `converged` before using `params`.

## Practical Examples

### Example 1

Reading a result.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("x =", result.params[0].items<double>());
  out.writeln("objective =", result.objective_value, ", gradient =", result.gradient_inf_norm, ", stationarity =", result.stationarity);
  out.writeln("converged =", result.converged, ", exit =", optim::septima_exit_flag_name(result.exit_flag), ", message =", result.message);
  out.writeln("iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
x = [3.000000, 3.000000]
objective = 0.000000 , gradient = 0.000000 , stationarity = 0.000000
converged = true , exit = Converged , message = converged
iterations = 3 , evaluations = 3
```

### Example 2

With constraints: the violation and the multiplier.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series target = pluss::series::make({2.0, 1.0});
  optim::SeptimaConstraints constraints;
  constraints.equalities = [](const std::vector<Series> &x) { return std::vector<Series>{x[0].sum() - 2.0}; };
  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const optim::SeptimaResult result = optim::septima([&](const Series &x) { return 0.5 * (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
  out.writeln("violation =", result.constraint_violation, ", multiplier =", result.state.lambda_equalities[0].item<double>());
  out.writeln("multiplier updates =", result.state.dual_updates);
}
```

Output

```text
violation = 0.000000 , multiplier = 0.500000
multiplier updates = 5
```

### Example 3

Judging a result. Use the flag and the measures, not an exact comparison.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.max_iterations = 4;
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("converged =", result.converged, ", exit =", optim::septima_exit_flag_name(result.exit_flag), ", gradient =", result.gradient_inf_norm);
}
```

Output

```text
converged = false , exit = MaxIterations , gradient = 5.994000
```

## See Also

- [`septima`](septima.md): produces it
- [`SeptimaState`](SeptimaState.md): the optimizer's state

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaProgressSnapshot.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaProgressSnapshot</span>
  </a>
  <a class="next-link" href="SeptimaSolveOptions.html">
    <span class="label">Next</span>
    <span class="title">SeptimaSolveOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
