<!-- generated from SeptimaSolveOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaSolveOptions`

Budgets, stopping tolerances, the optimizer settings and the progress hook of a `septima` run.

`SeptimaSolveOptions` is the third argument of [`septima`](septima.md). It nests the settings of the optimizer in `step` ([`SeptimaOptions`](SeptimaOptions.md)) and adds the loop's own controls: the iteration budget, how many non-improving iterations to tolerate, and four stopping tolerances. A run ends `Converged` when the gradient is below `gradient_tolerance` (and, with constraints, the violation is below `constraint_tolerance`), or when the objective change and the step are both below their tolerances; the rest of the endings are the failure flags. The defaults suit well-scaled problems; the usual changes are `max_iterations`, `gradient_tolerance` and the optimizer inside `step`.

## Fields

### `step`

`optim::SeptimaOptions`, default `{}`.

The optimizer and its settings, the penalty and multiplier schedule, and NaN recovery.

### `max_iterations`

`std::int64_t`, default `200`.

The iteration budget.

### `max_no_progress_iterations`

`std::int64_t`, default `25`.

Consecutive iterations without improvement before the run ends `NoProgress`.

### `objective_tolerance`

`double`, default `1e-12`.

An objective change below this, together with a small step, counts as converged.

### `gradient_tolerance`

`double`, default `1e-6`.

The largest gradient component (of the augmented Lagrangian, with blocked bound components removed) accepted as stationary.

### `step_tolerance`

`double`, default `1e-10`.

A step below this, together with a small objective change, counts as converged.

### `constraint_tolerance`

`double`, default `1e-6`.

The largest constraint violation accepted as feasible.

### `stop_on_nonfinite_objective`

`bool`, default `true`.

End the run `NonFiniteEvaluation` when the objective is NaN or infinite; false lets the NaN recovery try first.

### `progress_hook`

`std::function<bool(const optim::SeptimaProgressSnapshot &)>`, default `empty`.

Called each iteration; return `true` to stop the run with `Stopped`.

## Guidance

Passed to `septima`.

## Practical Examples

### Example 1

A looser gradient tolerance ends the run sooner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  for (const double tolerance : {1e-3, 1e-9}) {
    optim::SeptimaSolveOptions options;
    options.step.adam.learning_rate = 0.1;
    options.max_iterations = 3000;
    options.gradient_tolerance = tolerance;
    const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
    out.writeln("tolerance", tolerance, ": iterations =", result.iteration, ", exit =", optim::septima_exit_flag_name(result.exit_flag));
  }
}
```

Output

```text
tolerance 0.0010000000 : iterations = 385 , exit = Converged
tolerance 0.0000000010 : iterations = 424 , exit = Converged
```

### Example 2

The budget. With `max_iterations = 5` the run ends `MaxIterations` and the point is the last iterate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.1;
  options.max_iterations = 5;
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("exit =", optim::septima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
  out.writeln("x =", result.params[0].items<double>());
}
```

Output

```text
exit = MaxIterations , iterations = 5
x = [0.4982, 0.4982]
```

### Example 3

A progress hook. It is called each iteration with a snapshot and returns `true` to stop; here it returns `false` every time and records the objective.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  std::vector<double> history;
  optim::SeptimaSolveOptions options;
  options.step.adam.learning_rate = 0.5;
  options.max_iterations = 6;
  options.progress_hook = [&history](const optim::SeptimaProgressSnapshot &s) { history.push_back(s.objective); return false; };
  (void)optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("objective history =", history);
}
```

Output

```text
objective history = [18.0000, 12.5000, 8.0353, 4.6069, 2.1879, 0.7126]
```

## See Also

- [`septima`](septima.md): consumes it
- [`SeptimaOptions`](SeptimaOptions.md): the `step` member

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaResult.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaResult</span>
  </a>
  <a class="next-link" href="SeptimaState.html">
    <span class="label">Next</span>
    <span class="title">SeptimaState</span>
  </a>
</div>
<!-- apiref:nav:end -->
