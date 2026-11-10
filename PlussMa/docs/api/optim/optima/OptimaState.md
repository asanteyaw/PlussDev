<!-- generated from OptimaState.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaState`

What `optima` returns: the point, how good it is, the multipliers, and how the run ended.

`params` is the answer, a pack with the layout of the start (read a block by name: `result.params.at("x")`). `evaluation` is the problem evaluated at that point: `value` (the objective), `gradient`, the constraint residuals and `max_constraint_violation`. `converged` and `exit_flag` say whether the run met its tests and why it stopped (see [`OptimaExitFlag`](OptimaExitFlag.md)); `message` is a sentence about it; `iteration` and `function_evaluations` count the work; `stationarity` is the optimality measure at the answer. [`multipliers`](ConstraintMultipliers.md) holds the Lagrange multipliers, and `output` is an [`OptimaOutput`](OptimaOutput.md) with the detailed record. `best_feasible` holds the best feasible point seen when the final point is not the best.

## Fields

### `algorithm`

`optim::OptimaAlgorithm`, default `SequentialQP`.

The algorithm that ran.

### `params`

`optim::ParameterPack<pluss::Series>`, default `empty`.

The answer, one block per unknown.

### `evaluation`

`optim::ProblemEvaluation`, default `empty`.

The problem evaluated at `params`.

### `iteration`

`std::int64_t`, default `0`.

Iterations performed.

### `function_evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `last_step_size`

`double`, default `0`.

Length of the last accepted step fraction.

### `backend_parameter`

`double`, default `0`.

The algorithm's own control value at the end: the penalty weight, the merit penalty or the barrier parameter.

### `stationarity`

`double`, default `0`.

The optimality measure at the answer.

### `converged`

`bool`, default `false`.

Whether the optimality and feasibility tests were met.

### `exit_flag`

`optim::OptimaExitFlag`, default `Stopped`.

Why the run stopped.

### `message`

`std::string`, default `empty`.

A sentence about the ending.

### `output`

`optim::OptimaOutput`, default `{}`.

The detailed record of the run.

### `multipliers`

`optim::ConstraintMultipliers`, default `empty`.

The Lagrange multipliers by constraint family.

### `best_feasible`

`std::optional<optim::BestFeasiblePoint>`, default `none`.

The best feasible iterate seen, when kept.

## Guidance

Check `converged`/`exit_flag` before using `params`.

## Practical Examples

### Example 1

The main members of a result.

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
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  out.writeln("x         =", result.params.at("x").items<double>());
  out.writeln("objective =", result.evaluation.value.item<double>());
  out.writeln("violation =", result.evaluation.max_constraint_violation);
  out.writeln("converged =", result.converged, ", exit =", optim::optima_exit_flag_name(result.exit_flag));
  out.writeln("message   =", result.message);
}
```

Output

```text
x         = [0.447214, 0.894427]
objective = 1.527864
violation = 0.000000
converged = true , exit = Converged
message   = optimality and feasibility tolerances satisfied
```

### Example 2

Work counters. Iterations are steps; evaluations are objective calls (the line search and corrections add some).

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
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  out.writeln("iterations =", result.iteration, ", function_evaluations =", result.function_evaluations);
  out.writeln("last step size =", result.last_step_size, ", stationarity =", result.stationarity);
}
```

Output

```text
iterations = 4 , function_evaluations = 8
last step size = 1.000000 , stationarity = 0.000000
```

### Example 3

Judging a result. A tolerance-based test on the violation and the flag is the safe check, not a comparison with an exact number.

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
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  const bool usable = result.converged && result.evaluation.max_constraint_violation <= 1e-6;
  out.writeln("usable =", usable);
}
```

Output

```text
usable = true
```

### Example 4

The best feasible point. When a run ends away from its best feasible iterate, `best_feasible` carries it; `output` says whether it was returned.

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
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  out.writeln("best feasible kept =", result.best_feasible.has_value(), ", returned =", result.output.returned_best_feasible);
}
```

Output

```text
best feasible kept = true , returned = true
```

## See Also

- [`optima`](optima.md): produces it
- [`OptimaOutput`](OptimaOutput.md): the detailed record
- [`ConstraintMultipliers`](ConstraintMultipliers.md): the multipliers

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaStallCategory.html">
    <span class="label">Previous</span>
    <span class="title">OptimaStallCategory</span>
  </a>
  <a class="next-link" href="PenaltyMethodOptions.html">
    <span class="label">Next</span>
    <span class="title">PenaltyMethodOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
