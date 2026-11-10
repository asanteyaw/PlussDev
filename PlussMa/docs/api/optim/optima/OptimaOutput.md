<!-- generated from OptimaOutput.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaOutput`

The detailed record of a run, with the counts, residual measures and the stall diagnosis.

`OptimaState::output` summarises how the run went in plain numbers: `iterations` and `function_evaluations`, the `constraint_violation` and `first_order_optimality` at the end, the last `step_size` and the fraction of the step taken by the line search (`line_search_fraction`), the algorithm's control value (`backend_parameter`) and `complementarity`, the Krylov iterations used by iterative linear solves, whether a best feasible point existed and was returned, the size and number of active multipliers, and a diagnosis of why the run stalled (`stall_category`, with the measure and reference value behind it).

## Fields

### `algorithm`

`std::string`, default `empty`.

Short algorithm name such as `sqp`, `internal-protocol`, `merit-method`.

### `iterations`

`std::int64_t`, default `0`.

Iterations.

### `function_evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `constraint_violation`

`double`, default `0`.

Largest constraint violation at the end.

### `first_order_optimality`

`double`, default `0`.

First-order optimality measure at the end.

### `step_size`

`double`, default `0`.

Last step length.

### `line_search_fraction`

`double`, default `0`.

Fraction of the full step the line search accepted.

### `backend_parameter`

`double`, default `0`.

Penalty, merit or barrier value at the end.

### `complementarity`

`double`, default `0`.

Complementarity residual (interior method).

### `krylov_iterations`

`std::int64_t`, default `0`.

Iterative linear-solve iterations.

### `best_feasible_available`

`bool`, default `false`.

A feasible iterate was seen.

### `returned_best_feasible`

`bool`, default `false`.

The returned point is the best feasible one rather than the last iterate.

### `best_feasible_value`

`double`, default `+inf`.

Objective at the best feasible point.

### `best_feasible_constraint_violation`

`double`, default `+inf`.

Violation at the best feasible point.

### `best_feasible_first_order_optimality`

`double`, default `+inf`.

Optimality at the best feasible point.

### `multiplier_max_norm`

`double`, default `0`.

Largest multiplier magnitude.

### `active_multiplier_count`

`std::int64_t`, default `0`.

Multipliers that are non-zero.

### `detail_message`

`std::string`, default `empty`.

Extra detail about the ending.

### `stall_category`

`optim::OptimaStallCategory`, default `None`.

Diagnosis of a stall.

### `stall_measure`

`double`, default `0`.

The quantity behind the diagnosis.

### `stall_reference`

`double`, default `0`.

Its reference value.

## Guidance

Read as `state.output`.

## Practical Examples

### Example 1

Reading the record of a converged run.

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
  const optim::OptimaOutput &o = result.output;
  out.writeln("algorithm =", o.algorithm, ", iterations =", o.iterations, ", evaluations =", o.function_evaluations);
  out.writeln("violation =", o.constraint_violation, ", first-order optimality =", o.first_order_optimality);
  out.writeln("active multipliers =", o.active_multiplier_count, ", largest =", o.multiplier_max_norm);
}
```

Output

```text
algorithm = sqp , iterations = 4 , evaluations = 8
violation = 0.000000 , first-order optimality = 0.000000
active multipliers = 1 , largest = 1.236068
```

### Example 2

The stall diagnosis of a run that was cut short.

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
  options.sequential_qp.max_iterations = 1;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", stall =", optim::optima_stall_category_name(result.output.stall_category));
}
```

Output

```text
exit = MaxIterations , stall = iteration-limit
```

### Example 3

Best feasible bookkeeping.

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
  out.writeln("available =", result.output.best_feasible_available, ", returned =", result.output.returned_best_feasible);
  out.writeln("best feasible value =", result.output.best_feasible_value);
}
```

Output

```text
available = true , returned = true
best feasible value = 1.527864
```

## See Also

- [`OptimaState`](OptimaState.md): holds it
- [`detailed_summary`](reports_detailed_summary.md): prints it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaOptions.html">
    <span class="label">Previous</span>
    <span class="title">OptimaOptions</span>
  </a>
  <a class="next-link" href="OptimaProblemScaling.html">
    <span class="label">Next</span>
    <span class="title">OptimaProblemScaling</span>
  </a>
</div>
<!-- apiref:nav:end -->
