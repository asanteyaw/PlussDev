<!-- generated from SeptimaState.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaState`

The optimizer's memory at the end of a run: Adam moments, L-BFGS history, multipliers and penalty bookkeeping.

Held in `SeptimaResult::state`. For Adam, `adam` holds the moment estimates and step count. For L-BFGS, `lbfgs_s_history` and `lbfgs_y_history` are the stored step and gradient-difference pairs. With constraints, `lambda_equalities` and `lambda_inequalities` are the Lagrange-multiplier estimates (one `Series` per residual group), `dual_updates` counts multiplier updates, `penalty_scale` is the factor by which the run raised the penalty weights, and `constraint_violation` and `stationarity` are the last measured values. The multipliers are the main thing to read from it: each is the sensitivity of the optimal objective to its constraint.

## Fields

### `adam`

`optim::AdamState`, default `{}`.

Adam's moments and step count.

### `lbfgs_s_history`

`std::vector<pluss::Series>`, default `empty`.

Stored steps.

### `lbfgs_y_history`

`std::vector<pluss::Series>`, default `empty`.

Stored gradient differences.

### `lbfgs_prev_grad_flat`

`pluss::Series`, default `undefined`.

The previous flattened gradient.

### `lbfgs_prev_step_flat`

`pluss::Series`, default `undefined`.

The previous flattened step.

### `lambda_equalities`

`std::vector<pluss::Series>`, default `empty`.

Equality multipliers.

### `lambda_inequalities`

`std::vector<pluss::Series>`, default `empty`.

Inequality multipliers.

### `constraint_violation`

`double`, default `0`.

The last measured violation.

### `stationarity`

`double`, default `+inf`.

The last measured stationarity.

### `steps_since_dual_update`

`std::int64_t`, default `0`.

Steps since the last multiplier update.

### `dual_updates`

`std::int64_t`, default `0`.

Multiplier updates so far.

### `penalty_scale`

`double`, default `1`.

Factor applied to the penalty weights.

### `last_update_violation`

`double`, default `+inf`.

The violation at the last multiplier update.

### `step`

`std::int64_t`, default `0`.

Steps taken.

## Guidance

Read from `result.state`.

## Practical Examples

### Example 1

Multipliers. For the equality `x₀ + x₁ = 2` the multiplier is the price of the constraint, `0.5`.

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
  const Series target = pluss::series::make({2.0, 1.0});
  optim::SeptimaConstraints constraints;
  constraints.equalities = [](const std::vector<Series> &x) { return std::vector<Series>{x[0].sum() - 2.0}; };
  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const optim::SeptimaResult result = optim::septima([&](const Series &x) { return 0.5 * (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
  out.writeln("lambda =", result.state.lambda_equalities[0].item<double>());
}
```

Output

```text
lambda = 0.5000
```

### Example 2

Penalty bookkeeping.

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
  const Series target = pluss::series::make({2.0, 1.0});
  optim::SeptimaConstraints constraints;
  constraints.equalities = [](const std::vector<Series> &x) { return std::vector<Series>{x[0].sum() - 2.0}; };
  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const optim::SeptimaResult result = optim::septima([&](const Series &x) { return 0.5 * (x - target).square().sum(); }, pluss::series::make({0.0, 0.0}), options, constraints);
  out.writeln("penalty scale =", result.state.penalty_scale, ", multiplier updates =", result.state.dual_updates, ", steps =", result.state.step);
}
```

Output

```text
penalty scale = 1.0000 , multiplier updates = 5 , steps = 15
```

### Example 3

L-BFGS memory: the pairs stored at the end of an unconstrained run.

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
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
  out.writeln("stored pairs =", result.state.lbfgs_s_history.size(), ", steps =", result.state.step);
}
```

Output

```text
stored pairs = 1 , steps = 3
```

## See Also

- [`SeptimaResult`](SeptimaResult.md): holds it
- [`AdamState`](AdamState.md): the Adam part

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaSolveOptions.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaSolveOptions</span>
  </a>
  <a class="next-link" href="adam_state_like.html">
    <span class="label">Next</span>
    <span class="title">adam_state_like</span>
  </a>
</div>
<!-- apiref:nav:end -->
