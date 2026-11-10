<!-- generated from SeptimaProgressSnapshot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaProgressSnapshot`

What a `septima` progress hook receives each iteration.

`iteration`, `function_evaluations`, `objective`, `gradient_inf_norm`, `stationarity`, `step_inf_norm` and `constraint_violation` are the measures also found in the result, at this iteration; `message` says what just happened. The hook returns `true` to stop (exit flag `Stopped`) and `false` to keep going. [`reports::iter`](../optima/reports_iter.md) is a ready-made hook that prints one line per iteration and works with both `optima` and `septima`.

## Fields

### `iteration`

`std::int64_t`, default `0`.

Iteration number.

### `function_evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `objective`

`double`, default `+inf`.

Objective value.

### `gradient_inf_norm`

`double`, default `+inf`.

Largest gradient component.

### `stationarity`

`double`, default `+inf`.

Augmented-Lagrangian stationarity.

### `step_inf_norm`

`double`, default `0`.

Largest step component.

### `constraint_violation`

`double`, default `0`.

Largest violation.

### `message`

`std::string`, default `empty`.

What just happened.

## Guidance

Passed to every `progress_hook`.

## Practical Examples

### Example 1

Recording the objective each iteration.

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
  options.max_iterations = 5;
  options.progress_hook = [&history](const optim::SeptimaProgressSnapshot &s) { history.push_back(s.objective); return false; };
  (void)optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("objective =", history);
}
```

Output

```text
objective = [18.0000, 12.5000, 8.0353, 4.6069, 2.1879]
```

### Example 2

Stopping on a condition: when the gradient is below 1.

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
  options.step.adam.learning_rate = 0.5;
  options.progress_hook = [](const optim::SeptimaProgressSnapshot &s) { return s.gradient_inf_norm <= 1.0; };
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("exit =", optim::septima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
}
```

Output

```text
exit = Stopped , iterations = 7
```

### Example 3

The ready-made printing hook.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  options.progress_hook = optim::reports::iter(std::cout, 3);
  (void)optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
}
```

Output

```text
iter 1  objective 1.800e+01  gradient 6.000e+00  step 6.000e+00
iter 2  objective 1.800e+01  gradient 6.000e+00  step 3.000e+00
iter 3  objective 0.000e+00  gradient 0.000e+00  step 0.000e+00
```

## See Also

- [`SeptimaSolveOptions`](SeptimaSolveOptions.md): holds the hook

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaOptions.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaOptions</span>
  </a>
  <a class="next-link" href="SeptimaResult.html">
    <span class="label">Next</span>
    <span class="title">SeptimaResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
