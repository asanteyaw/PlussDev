# `pluss::ai::nn::optim::Minimax`

A primal-dual optimizer for training under equality and/or inequality
constraints, and/or per-parameter box bounds, built on top of any of the
other six optimizers' own update rule. Every step does two things: a
**primal** update (moving the parameter, driven by
[`Adam`](Adam.md)/[`AdamW`](AdamW.md)/[`SGD`](SGD.md)/[`RMSprop`](RMSprop.md)'s
own formula -- selectable via `primal_method` -- plus, when constraints
are supplied, a penalty gradient pulling infeasible parameters back toward
the feasible region), and a **dual** update (adjusting a Lagrange
multiplier per constraint, via gradient ascent on how much that constraint
is currently violated). This is the augmented Lagrangian method: a penalty
term alone can never fully enforce a constraint at any finite weight, but
the multiplier's own accumulation closes that gap over training.

Constraints are expressed as [`MinimaxConstraints`](MinimaxConstraints.md)
-- residual callables (`c(x) = 0` for equality, `h(x) <= 0` for
inequality) plus optional per-parameter `lower_bounds`/`upper_bounds` --
supplied once, at construction. Every method not specific to the
primal-dual formula (`step`, `parameters`, `size`, `clear`,
`last_step_lane_telemetry`) is inherited unchanged from
[`Optimizer`](Optimizer.md).

## Signature

```cpp
explicit nn::optim::Minimax::Minimax(pluss::RangeView<pluss::Series*> parameters,
                                     nn::optim::MinimaxOptions options = {},
                                     nn::optim::MinimaxConstraints constraints = {});
explicit nn::optim::Minimax::Minimax(std::initializer_list<pluss::Series*> parameters,
                                     nn::optim::MinimaxOptions options = {},
                                     nn::optim::MinimaxConstraints constraints = {});
explicit nn::optim::Minimax::Minimax(pluss::RangeView<nn::optim::MinimaxGroup> groups,
                                     nn::optim::MinimaxConstraints constraints = {});
explicit nn::optim::Minimax::Minimax(std::initializer_list<nn::optim::MinimaxGroup> groups,
                                     nn::optim::MinimaxConstraints constraints = {});
explicit nn::optim::Minimax::Minimax(nn::Module& module,
                                     nn::optim::MinimaxOptions options = {},
                                     nn::optim::MinimaxConstraints constraints = {});

const nn::optim::MinimaxOptions& nn::optim::Minimax::options() const noexcept;
void nn::optim::Minimax::set_options(const nn::optim::MinimaxOptions& options);
void nn::optim::Minimax::set_group_options(std::size_t group_index, const nn::optim::MinimaxOptions& options);
std::size_t nn::optim::Minimax::group_count() const noexcept;

const nn::optim::MinimaxConstraints& nn::optim::Minimax::constraints() const noexcept;
void nn::optim::Minimax::set_constraints(const nn::optim::MinimaxConstraints& constraints);

nn::optim::Minimax::State nn::optim::Minimax::capture_state() const;
void nn::optim::Minimax::restore_state(const nn::optim::Minimax::State& state);
```

`Minimax` takes `constraints` as a *separate* argument from `options` at
every constructor (rather than folding it into `MinimaxOptions`, the way
`clip`/`execution` are folded into every other optimizer's own options),
since the residual callables and bounds are per-optimizer state shared
across every parameter group, not a per-group hyperparameter.

## Construction

```cpp
nn::optim::MinimaxOptions options{};
options.learning_rate = 0.1;
options.inequality_penalty = 10.0;

nn::optim::MinimaxConstraints constraints{};
constraints.inequalities = [](const std::vector<Series>& vars) {
  return std::vector<Series>{vars[0].sub(3.0)};  // x <= 3
};
constraints.inequalities_trace = [](const std::vector<diff::Trace>& vars) {
  return std::vector<diff::Trace>{vars[0] - 3.0};
};

nn::optim::Minimax minimax({&x}, options, constraints);
```

Every constraint an `enable_constraint_autodiff = true`
[`MinimaxOptions`](MinimaxOptions.md) (the default) needs to actually
*enforce* -- as opposed to merely tracking violation through the dual
multiplier update, which alone never moves the primal parameter -- must be
supplied in **both** forms: the plain `Series`-based callable
(`equalities`/`inequalities`) *and* the `Trace`-based counterpart
(`equalities_trace`/`inequalities_trace`). Construction validates this and
throws immediately if one is set without the other while
`enable_constraint_autodiff` is on -- see
[`MinimaxConstraints`](MinimaxConstraints.md) for exactly which state
each form feeds and the exact error.

## State

`options()` / `set_options(options)` / `set_group_options(index, options)` / `group_count()`
: Same shape as every other optimizer's own `## State` -- see
  [`SGD`](SGD.md#state).

`constraints()` / `set_constraints(constraints)`
: Read or replace the [`MinimaxConstraints`](MinimaxConstraints.md) this
  optimizer enforces. Unlike per-group `options`, there is exactly one
  `MinimaxConstraints` per `Minimax` instance, shared across every group.

`capture_state()` / `restore_state(state)`
: Captures/restores every group's own primal moment/momentum buffers
  (whichever `primal_method` needs), plus the dual multipliers for every
  equality/inequality/bound constraint, plus the step counter and the
  moving-violation estimate the `ViolationAdaptive` schedule reads.
  Independent of parameter values, the same as every other optimizer's
  `capture_state`.

## Methods

Inherited unchanged from [`Optimizer`](Optimizer.md): `step`
(four overloads), `parameters`, `size`, `clear`,
`last_step_lane_telemetry`.

## Practical Examples

Three examples: a single inequality constraint pulling a parameter back
from an infeasible unconstrained minimum, an equality constraint
coupling two parameters, and per-parameter box bounds enforced with no
`equalities`/`inequalities` at all.

### Example 1

Minimizing `f(x) = 0.5 * (x - 5)^2` (unconstrained minimum `x = 5`) under
the inequality constraint `x <= 3`. The objective's own gradient (`x -
5`) always pulls toward `5`; only the constraint penalty pulls back once
`x` crosses `3`. As a *soft* penalty method, `x` overshoots past the
boundary before the accumulating dual multiplier pulls it back near the
constraint.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  namespace diff = pluss::subsp::math::diff;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series target = pluss::make_series(5.0F, f32);

  optim::MinimaxOptions options{};
  options.learning_rate = 0.1;
  options.dual_learning_rate = 0.2;
  options.inequality_penalty = 10.0;

  optim::MinimaxConstraints constraints{};
  constraints.inequalities = [](const std::vector<Series>& vars) {
    return std::vector<Series>{vars[0].sub(3.0)};
  };
  constraints.inequalities_trace = [](const std::vector<diff::Trace>& vars) {
    return std::vector<diff::Trace>{vars[0] - 3.0};
  };

  Series x = pluss::make_series(0.0F, f32);
  optim::Minimax minimax(std::vector<Series*>{&x}, options, constraints);

  for (int step = 0; step < 60; ++step) {
    const Series gradient = x.sub(target);
    minimax.step(std::vector<Series>{gradient});
    if (step == 0 || step == 4 || step == 9 || step == 19 || step == 39 || step == 59) {
      std::cout << "step " << step << ": x = " << x.item<float>() << "\n";
    }
  }

  return 0;
}
```

Output

```text
step 0: x = 0.100001
step 4: x = 0.49902
step 9: x = 0.992405
step 19: x = 1.93967
step 39: x = 3.40134
step 59: x = 2.97
```

### Example 2

Minimizing `f(x, y) = 0.5 * (x - 5)^2 + 0.5 * (y - 5)^2` (unconstrained
minimum `(5, 5)`) under the equality constraint `x + y = 4`. The
constrained optimum is the point on that line closest to `(5, 5)`: `(2,
2)`, by symmetry.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  namespace diff = pluss::subsp::math::diff;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series target = pluss::make_series(5.0F, f32);

  optim::MinimaxOptions options{};
  options.learning_rate = 0.1;
  options.dual_learning_rate = 0.2;
  options.equality_penalty = 10.0;

  optim::MinimaxConstraints constraints{};
  constraints.equalities = [](const std::vector<Series>& vars) {
    return std::vector<Series>{vars[0].add(vars[1]).sub(4.0)};
  };
  constraints.equalities_trace = [](const std::vector<diff::Trace>& vars) {
    return std::vector<diff::Trace>{vars[0] + vars[1] - 4.0};
  };

  Series x = pluss::make_series(0.0F, f32);
  Series y = pluss::make_series(0.0F, f32);
  optim::Minimax minimax(std::vector<Series*>{&x, &y}, options, constraints);

  for (int step = 0; step < 200; ++step) {
    const Series gx = x.sub(target);
    const Series gy = y.sub(target);
    minimax.step(std::vector<Series>{gx, gy});
  }
  std::cout << "x = " << x.item<float>() << ", y = " << y.item<float>()
             << ", x + y = " << (x.item<float>() + y.item<float>()) << "\n";

  return 0;
}
```

Output

```text
x = 2.0098, y = 2.0098, x + y = 4.01961
```

### Example 3

Per-parameter box bounds, with **no** `equalities`/`inequalities`
callables at all -- `lower_bounds`/`upper_bounds` alone are enough for
`Minimax` to engage its constraint machinery. `project_bounds` defaults
to `true`, so the candidate value is hard-clamped into the bounds every
step, on top of the soft penalty already pulling it there.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <vector>

int main() {
  namespace nn = pluss::ai::nn;
  namespace optim = nn::optim;
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const Series target = pluss::make_series(5.0F, f32);

  optim::MinimaxOptions options{};
  options.learning_rate = 0.1;
  options.dual_learning_rate = 0.2;

  optim::MinimaxConstraints constraints{};
  constraints.lower_bounds = {pluss::make_series(0.0F, f32)};
  constraints.upper_bounds = {pluss::make_series(2.0F, f32)};

  Series x = pluss::make_series(0.0F, f32);
  optim::Minimax minimax(std::vector<Series*>{&x}, options, constraints);

  for (int step = 0; step < 30; ++step) {
    const Series gradient = x.sub(target);
    minimax.step(std::vector<Series>{gradient});
  }
  std::cout << "x after 30 steps (bounded to [0, 2]): " << x.item<float>() << "\n";

  return 0;
}
```

Output

```text
x after 30 steps (bounded to [0, 2]): 2
```

## Changelog

- **0.1** — the primal-dual constrained optimizer: a selectable primal
  method (Adam/AdamW/SGD/RMSprop), equality/inequality residual
  constraints, per-parameter bounds, NaN recovery, and constant/
  exponential/violation-adaptive dual and penalty schedules.

## See Also

- [`MinimaxOptions`](MinimaxOptions.md) -- every field's default and
  effect.
- [`MinimaxConstraints`](MinimaxConstraints.md) -- the residual callables
  and bounds this optimizer enforces.
- [`MinimaxGroup`](MinimaxGroup.md) -- per-group construction.
- [`Optimizer`](Optimizer.md) -- the shared base.
- [`Adam`](Adam.md), [`AdamW`](AdamW.md), [`SGD`](SGD.md),
  [`RMSprop`](RMSprop.md) -- the four primal update rules
  `MinimaxOptions::primal_method` selects between.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

MinimaxConstraints
MinimaxGroup
MinimaxOptions
```
<!-- apiref:toctree:end -->
