# `pluss::ai::nn::optim::MinimaxConstraints`

The equality/inequality residual callables and optional per-parameter
bounds [`Minimax`](Minimax.md) enforces -- supplied once at construction,
shared across every parameter group.

## Signature

```cpp
std::function<std::vector<pluss::Series>(const std::vector<pluss::Series>&)> equalities;
std::function<std::vector<pluss::Series>(const std::vector<pluss::Series>&)> inequalities;
std::function<std::vector<pluss::subsp::math::diff::Trace>(const std::vector<pluss::subsp::math::diff::Trace>&)> equalities_trace;
std::function<std::vector<pluss::subsp::math::diff::Trace>(const std::vector<pluss::subsp::math::diff::Trace>&)> inequalities_trace;
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
constraints.inequalities_trace = [](const std::vector<diff::Trace>& x) {
  return std::vector<diff::Trace>{x[0] - 3.0};  // same residual, autodiff-traced
};
constraints.lower_bounds = {pluss::make_series(0.0, f32)};
```

## State

`equalities` / `inequalities` -- `function<vector<Series>(const vector<Series>&)>`
: Given the current *value* of every optimizer parameter (in parameter
  order), returns one residual `Series` per constraint. `equalities`
  residuals target `c(x) = 0`; `inequalities` residuals target `h(x) <=
  0` (a positive value means violated). Used unconditionally, by the dual
  multiplier update -- this is what lets `Minimax` track how violated a
  constraint currently is, regardless of `enable_constraint_autodiff`.
  Inequality residuals must be real-valued (the multiplier's own
  `clamp_min(0)` has no ordering over complex values).

`equalities_trace` / `inequalities_trace` -- `function<vector<Trace>(const vector<Trace>&)>`
: The same residual computation, written over
  `pluss::subsp::math::diff::Trace` instead of `Series`, so it can be
  differentiated automatically. Required, alongside the matching plain
  callable, whenever
  [`MinimaxOptions::enable_constraint_autodiff`](MinimaxOptions.md#fields)
  is `true` (the default) -- this is the form that actually contributes a
  penalty gradient to the primal step; the plain form alone never moves
  the parameter. Construction throws if one form is set without the
  other while autodiff is enabled (see Example 2).

`lower_bounds` / `upper_bounds` -- `vector<Series>`
: Optional, index-aligned with the optimizer's own parameter list. Empty
  (the default) means "no bounds" on any parameter. A non-empty vector's
  size must equal the parameter count; each entry is either the full
  shape of the corresponding parameter or a `numel() == 1` scalar
  (broadcast). Unlike `equalities`/`inequalities`, bounds need no
  `_trace` counterpart -- the penalty term over a bound violation is
  built directly from the parameter itself inside `Minimax`'s own traced
  objective, so autodiff reaches it automatically whenever
  `enable_constraint_autodiff` is on. Bounds are rejected for complex
  parameters, for the same real-ordering reason as inequality residuals.

## Methods

None -- `MinimaxConstraints` is a plain data aggregate, consumed only by
[`Minimax`](Minimax.md)'s constructor and
[`set_constraints`](Minimax.md#state).

## Practical Examples

Two examples: the inequality-constraint setup [`Minimax`](Minimax.md)'s
own Example 1 runs, shown here as the `MinimaxConstraints` value in
isolation, and the exact validation error construction throws when a
`_trace` counterpart is missing.

### Example 1

Both forms of the same residual (`x <= 3`) -- the plain form the dual
update reads, and the `_trace` form autodiff needs to turn that residual
into a primal-step correction.

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

  optim::MinimaxConstraints constraints{};
  constraints.inequalities = [](const std::vector<Series>& vars) {
    return std::vector<Series>{vars[0].sub(3.0)};
  };
  constraints.inequalities_trace = [](const std::vector<diff::Trace>& vars) {
    return std::vector<diff::Trace>{vars[0] - 3.0};
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
residual at x=4 (x - 3): 1
residual at x=2 (x - 3): -1
```

### Example 2

Supplying `inequalities` without `inequalities_trace`, with
`enable_constraint_autodiff` left at its default `true` -- construction
throws immediately rather than silently building an optimizer that would
never actually enforce the constraint.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>
#include <string>
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
  // inequalities_trace intentionally left unset.

  Series x = pluss::make_series(0.0F, f32);
  try {
    optim::Minimax minimax(std::vector<Series*>{&x}, optim::MinimaxOptions{}, constraints);
  } catch (const std::exception& e) {
    // e.what() carries a second "Triggered at: ..." line with the engine's
    // own internal source location; only the message itself is printed here.
    const std::string message(e.what());
    std::cout << "caught: " << message.substr(0, message.find('\n')) << "\n";
  }

  return 0;
}
```

Output

```text
caught: pluss::ai::nn::optim::Minimax: inequalities callback requires inequalities_trace for autodiff constraints
```

## Changelog

- **0.1** — plain and autodiff-traced residual callables for equality and
  inequality constraints, plus optional per-parameter bounds.

## See Also

- [`Minimax`](Minimax.md) -- the optimizer this configures.
- [`MinimaxOptions`](MinimaxOptions.md) -- `enable_constraint_autodiff`
  and the penalty/dual-rate fields these constraints feed.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
