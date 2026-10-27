# `pluss::ai::nn::optim::MinimaxConstraints`

The equality/inequality residual callables and optional per-parameter
bounds [`Minimax`](Minimax.md) enforces -- supplied once at construction,
shared across every parameter group.

## Signature

```cpp
std::function<std::vector<pluss::Series>(const std::vector<pluss::Series>&)> equalities;
std::function<std::vector<pluss::Series>(const std::vector<pluss::Series>&)> inequalities;
std::vector<pluss::Series> lower_bounds;
std::vector<pluss::Series> upper_bounds;
```

A plain aggregate -- every field defaults empty, so `MinimaxConstraints{}`
is the no-constraints case (an ordinary unconstrained primal optimizer,
just with `Minimax`'s own selectable primal method and NaN recovery).

## Construction

```cpp
nn::optim::MinimaxConstraints constraints{};
constraints.inequalities = [](const std::vector<Series>& x) {
  return std::vector<Series>{x[0].sub(3.0)};  // x[0] <= 3
};
constraints.lower_bounds = {pluss::make_series(0.0, f32)};
```

## State

`equalities` / `inequalities` -- `function<vector<Series>(const vector<Series>&)>`
: Given the current *value* of every optimizer parameter (in parameter
  order), returns one residual `Series` per constraint. `equalities`
  residuals target `c(x) = 0`; `inequalities` residuals target `h(x) <=
  0` (a positive value means violated). Write them over plain `Series`:
  the dual multiplier update reads their values, and when
  [`MinimaxOptions::enable_constraint_autodiff`](MinimaxOptions.md#fields)
  is `true` (the default) the same callables are differentiated to give
  the penalty gradient of the primal step -- one residual function serves
  both.
  Inequality residuals must be real-valued (the multiplier's own
  `clamp_min(0)` has no ordering over complex values).

`lower_bounds` / `upper_bounds` -- `vector<Series>`
: Optional, index-aligned with the optimizer's own parameter list. Empty
  (the default) means "no bounds" on any parameter. A non-empty vector's
  size must equal the parameter count; each entry is either the full
  shape of the corresponding parameter or a `numel() == 1` scalar
  (broadcast). The penalty term over a bound violation is built directly from the
  parameter itself, so autodiff reaches it whenever
  `enable_constraint_autodiff` is on. Bounds are rejected for complex
  parameters, for the same real-ordering reason as inequality residuals.

## Methods

None -- `MinimaxConstraints` is a plain data aggregate, consumed only by
[`Minimax`](Minimax.md)'s constructor and
[`set_constraints`](Minimax.md#state).

## Practical Examples

The inequality-constraint setup [`Minimax`](Minimax.md)'s own Example 1
runs, shown here as the `MinimaxConstraints` value in isolation.

### Example 1

One residual (`x <= 3`) written over plain `Series`: the dual update reads
its value, and autodiff differentiates it into a primal-step correction.

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

  optim::MinimaxConstraints constraints{};
  constraints.inequalities = [](const std::vector<Series>& vars) {
    return std::vector<Series>{vars[0].sub(3.0)};
  };

  Series x = pluss::make_series(4.0F, f32);
  const auto residual = constraints.inequalities({x});
  std::cout << "residual at x=4 (x - 3): " << residual[0].item<float>()
             << " (positive means violated)\n";

  Series x_feasible = pluss::make_series(2.0F, f32);
  const auto residual2 = constraints.inequalities({x_feasible});
  std::cout << "residual at x=2 (x - 3): " << residual2[0].item<float>()
             << " (negative or zero means feasible)\n";

  return 0;
}
```

Output

```text
residual at x=4 (x - 3): 1 (positive means violated)
residual at x=2 (x - 3): -1 (negative or zero means feasible)
```

## Changelog

- **0.1** — residual callables over plain `Series` for equality and
  inequality constraints, plus optional per-parameter bounds.

## See Also

- [`Minimax`](Minimax.md) -- the optimizer this configures.
- [`MinimaxOptions`](MinimaxOptions.md) -- `enable_constraint_autodiff`
  and the penalty/dual-rate fields these constraints feed.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
