<!-- generated from WorkingSetOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::WorkingSetOptions`

Settings of the working-set algorithm: every sequential-QP setting plus the controls of the active-set bookkeeping.

Used when `OptimaOptions::algorithm` is `WorkingSet`, through `OptimaOptions::working_set`. `WorkingSetOptions` *is* a `SequentialQPOptions` (all of its fields are available under `working_set`, with the same meaning) plus the fields below, which govern the working set: the list of constraints currently treated as active. The algorithm adds a constraint when a step would cross it, drops one whose multiplier has become negligible, and keeps the set between iterations so the quadratic subproblems stay small and warm.

## Fields

### `favor_working_set_updates`

`bool`, default `true`.

Prefer steps that update the working set over steps from the full model.

### `max_working_set_violations`

`std::int64_t`, default `8`.

The most constraints that may be added at one iteration.

### `add_constraint_tolerance`

`double`, default `1e-3`.

A constraint within this of its limit is added to the working set.

### `drop_multiplier_tolerance`

`double`, default `1e-8`.

A constraint whose multiplier is below this is dropped.

### `feasibility_priority_ratio`

`double`, default `0.8`.

How strongly reducing the violation is preferred over reducing the objective.

### `max_stalled_iterations`

`std::int64_t`, default `8`.

Iterations with an unchanged working set before the algorithm falls back to a full step.

### `multiplier_smoothing`

`double`, default `0.5`.

Blend of the old and new multiplier estimates used for the drop test.

### `multiplier_retention_tolerance`

`double`, default `1e-6`.

A multiplier above this keeps its constraint in the set.

## Guidance

Set through `OptimaOptions::working_set`.

## Practical Examples

### Example 1

Disabling the working-set preference. With `favor_working_set_updates = false` the algorithm behaves like plain sequential QP.

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

  optim::OptimaOptions on, off;
  on.algorithm = optim::OptimaAlgorithm::WorkingSet;
  off.algorithm = optim::OptimaAlgorithm::WorkingSet;
  off.working_set.favor_working_set_updates = false;
  for (const auto *o : {&on, &off})
    out.writeln("favor =", o->working_set.favor_working_set_updates, ": iterations =", optim::optima(objective, pluss::series::make({0.0, 0.0}), *o, disc).iteration);
}
```

Output

```text
favor = true : iterations = 8
favor = false : iterations = 9
```

### Example 2

Inherited settings apply: a smaller iteration budget stops the run.

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
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  options.working_set.max_iterations = 2;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
exit = MaxIterations , iterations = 2 , evaluations = 7
```

### Example 3

Tolerances for adding and dropping constraints.

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
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  options.working_set.add_constraint_tolerance = 1e-1;
  options.working_set.drop_multiplier_tolerance = 1e-6;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [0.4472, 0.8944]
```

## See Also

- [`SequentialQPOptions`](SequentialQPOptions.md): the inherited settings
- [`OptimaOptions`](OptimaOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SequentialQPOptions.html">
    <span class="label">Previous</span>
    <span class="title">SequentialQPOptions</span>
  </a>
  <a class="next-link" href="audit_optima_derivatives.html">
    <span class="label">Next</span>
    <span class="title">audit_optima_derivatives</span>
  </a>
</div>
<!-- apiref:nav:end -->
