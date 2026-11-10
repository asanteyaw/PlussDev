<!-- generated from PSConstraintPolicy.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSConstraintPolicy`

How nonlinear constraint violations are handled.

`ExtremeBarrier` rejects a violating point outright. `MeritPenalty` minimises the objective plus a penalty on the violation, with weight `initial_penalty` grown by `penalty_growth`. `AugmentedLagrangian` adds multiplier estimates to the penalty and refines both over up to `al_max_outer_iterations` rounds; it converges to the constrained optimum where a fixed penalty only approaches it, and is the default.

## Fields

### `ExtremeBarrier`

`enumerator`, default `0`.

Reject violating points.

### `MeritPenalty`

`enumerator`, default `1`.

Objective plus a growing violation penalty.

### `AugmentedLagrangian`

`enumerator`, default `2`.

Penalty plus multiplier estimates.

## Guidance

Set as `PSConfig::constraint_policy`.

## Practical Examples

### Example 1

The three policies on the unit-disc problem. The exact value is 3.3431.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };   // exact constrained optimum 3.3431 at (0.7071, 0.7071)

  for (const auto policy : {dfo::PSConstraintPolicy::ExtremeBarrier, dfo::PSConstraintPolicy::MeritPenalty, dfo::PSConstraintPolicy::AugmentedLagrangian}) {
    dfo::PSConfig config;
    config.constraint_policy = policy;
    config.max_iterations = 4000;
    config.max_evaluations = 40000;
    config.nonlinear_constraints = [](const Series &x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.0, 0.0}), config);
    out.writeln(static_cast<int>(policy), ":", dfo::to_string(o.status), ", objective =", o.minimum, ", violation =", o.diagnostics.max_constraint_violation, ", rounds =", o.diagnostics.outer_iterations);
  }
}
```

Output

```text
0 : Converged , objective = 3.3437 , violation = 0.0000 , rounds = 0
1 : Converged , objective = 3.3438 , violation = 0.0000 , rounds = 0
2 : Converged , objective = 3.3431 , violation = 0.0000 , rounds = 8
```

### Example 2

The starting penalty of the merit policy. The weight is grown after each failed poll until the penalized minimum is feasible, so a small start converges too; a start near the right weight needs fewer evaluations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };   // exact constrained optimum 3.3431 at (0.7071, 0.7071)

  for (const double penalty : {1.0, 1000.0}) {
    dfo::PSConfig config;
    config.constraint_policy = dfo::PSConstraintPolicy::MeritPenalty;
    config.initial_penalty = penalty;
    config.nonlinear_constraints = [](const Series &x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.0, 0.0}), config);
    out.writeln("initial penalty", penalty, ": objective =", o.minimum, ", evaluations =", o.diagnostics.evaluations, ", violation =", o.diagnostics.max_constraint_violation);
  }
}
```

Output

```text
initial penalty 1.0000 : objective = 3.3443 , evaluations = 284 , violation = 0.0000
initial penalty 1000.0000 : objective = 3.3437 , evaluations = 241 , violation = 0.0000
```

### Example 3

The number of augmented-Lagrangian rounds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto f = [](const Series &x) { return (x - 2.0).square().sum(); };   // exact constrained optimum 3.3431 at (0.7071, 0.7071)

  for (const std::int64_t rounds : {2, 20}) {
    dfo::PSConfig config;
    config.al_max_outer_iterations = rounds;
    config.nonlinear_constraints = [](const Series &x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({0.0, 0.0}), config);
    out.writeln("rounds allowed", rounds, ": objective =", o.minimum, ", violation =", o.diagnostics.max_constraint_violation);
  }
}
```

Output

```text
rounds allowed 2 : objective = 3.31049 , violation = 0.01798
rounds allowed 20 : objective = 3.34316 , violation = 0.00000
```

## See Also

- [`PSConfig`](PSConfig.md): holds it
- [`PSConstraintValues`](PSConstraintValues.md): the constraint values

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSConfig.html">
    <span class="label">Previous</span>
    <span class="title">PSConfig</span>
  </a>
  <a class="next-link" href="PSConstraintValues.html">
    <span class="label">Next</span>
    <span class="title">PSConstraintValues</span>
  </a>
</div>
<!-- apiref:nav:end -->
