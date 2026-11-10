<!-- generated from SeptimaOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaOptions`

Chooses the optimizer of `septima` and holds the settings of Adam, L-BFGS and the constraint penalty schedule.

`SeptimaOptions` is the `step` member of [`SeptimaSolveOptions`](SeptimaSolveOptions.md). `optimizer` selects `Adam` or `Lbfgs` ([`SeptimaOptimizer`](SeptimaOptimizer.md)); the settings of each live in `adam` ([`AdamOptions`](AdamOptions.md)) and `lbfgs` ([`LbfgsOptions`](LbfgsOptions.md)), and only the chosen one is read. The remaining fields govern constraints, which are enforced by an augmented Lagrangian: the penalty weights `equality_penalty` and `inequality_penalty`, how often the multipliers are updated (`dual_update_ratio`, `dual_update_floor`, `dual_update_max_steps`, `dual_learning_rate`), how the penalty is raised when the violation stops falling (`penalty_growth`, `penalty_growth_trigger`, `max_penalty`), whether bounds are projected, and the recovery from a non-finite evaluation. Without constraints only `optimizer`, `adam`, `lbfgs` and the NaN recovery matter.

## Fields

### `optimizer`

`optim::SeptimaOptimizer`, default `Adam`.

`Adam` or `Lbfgs`.

### `adam`

`optim::AdamOptions`, default `{}`.

Adam's settings, read when `optimizer` is `Adam`.

### `lbfgs`

`optim::SeptimaOptions::LbfgsOptions`, default `{}`.

L-BFGS's settings, read when `optimizer` is `Lbfgs`.

### `dual_learning_rate`

`double`, default `0`.

Step of the multiplier update `λ += rate · residual`. 0 means the penalty weight itself, the classical method of multipliers; a positive value overrides it.

### `equality_penalty`

`double`, default `10`.

Weight of the quadratic penalty on equality residuals.

### `inequality_penalty`

`double`, default `10`.

Weight of the quadratic penalty on inequality violations.

### `dual_update_ratio`

`double`, default `0.1`.

The multipliers are updated once the augmented problem's stationarity is at most this times the current constraint violation.

### `dual_update_floor`

`double`, default `1e-10`.

The stationarity asked for before a multiplier update is never smaller than this.

### `dual_update_max_steps`

`std::int64_t`, default `50`.

A multiplier update happens after this many steps even if the stationarity test has not been met.

### `penalty_growth`

`double`, default `10`.

Factor by which the penalties are multiplied when a multiplier update leaves the violation larger than `penalty_growth_trigger` times its previous value. 1 keeps the penalties fixed.

### `penalty_growth_trigger`

`double`, default `0.25`.

How much the violation must shrink between multiplier updates to avoid raising the penalty.

### `max_penalty`

`double`, default `1e8`.

The largest penalty weight the run will reach.

### `project_bounds`

`bool`, default `true`.

Clip the iterate back inside the bounds each step.

### `enable_constraint_autodiff`

`bool`, default `true`.

Differentiate the constraint residuals through the objective's tape. Needed for equality residuals that are not real; see the complex-parameter rules.

### `nan_recovery_attempts`

`std::int64_t`, default `2`.

How many times a step that produced a non-finite value is retried with a smaller step.

### `nan_recovery_shrink`

`double`, default `0.5`.

The factor by which the step is shrunk on each retry.

## Guidance

Used as `SeptimaSolveOptions::step`.

## Practical Examples

### Example 1

Switching optimizer. The same problem with each: L-BFGS needs far fewer iterations on this smooth objective.

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

  for (const optim::SeptimaOptimizer optimizer : {optim::SeptimaOptimizer::Adam, optim::SeptimaOptimizer::Lbfgs}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optimizer;
    options.step.adam.learning_rate = 0.1;
    options.max_iterations = 3000;
    const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
    out.writeln(optimizer == optim::SeptimaOptimizer::Adam ? "Adam " : "Lbfgs", ": iterations =", result.iteration, ", x =", result.params[0].items<double>());
  }
}
```

Output

```text
Adam  : iterations = 385 , x = [3.0000, 3.0000]
Lbfgs : iterations = 3 , x = [3.0000, 3.0000]
```

### Example 2

The penalty. The equality `x₀ + x₁ = 2` is enforced more or less strictly depending on `equality_penalty`; the run raises a too-small penalty itself, which `state.penalty_scale` reports.

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
  for (const double penalty : {1.0, 100.0}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.equality_penalty = penalty;
    const optim::SeptimaResult result = optim::septima([&](const Series &x) { return 0.5 * (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
    out.writeln("penalty", penalty, ": x =", result.params[0].items<double>(), ", violation =", result.constraint_violation, ", multiplier updates =", result.state.dual_updates);
  }
}
```

Output

```text
penalty 1.000000 : x = [1.500000, 0.500000] , violation = 0.000001 , multiplier updates = 6
penalty 100.000000 : x = [1.500000, 0.500000] , violation = 0.000000 , multiplier updates = 3
```

### Example 3

Fixed penalties. With `penalty_growth = 1` the run never raises the penalty.

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
  options.step.penalty_growth = 1.0;
  const optim::SeptimaResult result = optim::septima([&](const Series &x) { return 0.5 * (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
  out.writeln("penalty scale =", result.state.penalty_scale, ", x =", result.params[0].items<double>());
}
```

Output

```text
penalty scale = 1.000000 , x = [1.500000, 0.500000]
```

### Example 4

Projection. With `project_bounds = false` the bounds are only a penalty; the point may sit slightly outside.

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
  const Series target = pluss::series::make({3.0, 3.0});
  optim::SeptimaConstraints constraints;
  constraints.ubs = {pluss::series::make({2.0, 2.0})};
  for (const bool project : {true, false}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.project_bounds = project;
    const optim::SeptimaResult result = optim::septima([&](const Series &x) { return (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
    out.writeln("project", project, ": x =", result.params[0].items<double>());
  }
}
```

Output

```text
project true : x = [2.000000, 2.000000]
project false : x = [3.000000, 3.000000]
```

## See Also

- [`SeptimaSolveOptions`](SeptimaSolveOptions.md): holds it
- [`AdamOptions`](AdamOptions.md): Adam settings
- [`LbfgsOptions`](LbfgsOptions.md): L-BFGS settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaOptimizer.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaOptimizer</span>
  </a>
  <a class="next-link" href="SeptimaProgressSnapshot.html">
    <span class="label">Next</span>
    <span class="title">SeptimaProgressSnapshot</span>
  </a>
</div>
<!-- apiref:nav:end -->
