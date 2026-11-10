<!-- generated from with_mads_config.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c33afa3a3e1b -->

# `DfoBuilder::with_mads_config`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the settings of the mesh-adaptive methods.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_mads_config(const dfo::MadsBlackboxAdvancedConfig &cfg) const;
```

## What It Does

Sets `DfoOptions::mads`, a [`MadsBlackboxAdvancedConfig`](../MadsBlackboxAdvancedConfig.md) read by `OrthoMads`, `LtMads` and `MadsAdvanced`: mesh and frame sizes, opportunistic polling, constraint handling, scalarisation of several objectives, the cache and the surrogate. The poll scheme is set by the chosen kind, the common settings are applied on top, and when the problem has constraints and the config's constraint handling is `Ignore`, progressive barrier handling is used instead.

## Parameters

`const dfo::MadsBlackboxAdvancedConfig &cfg`
: The mesh-adaptive settings.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Opportunistic polling off. By default the poll stops at the first improving point; turning it off evaluates the whole poll set each iteration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  for (const bool opportunistic : {true, false}) {
    dfo::MadsBlackboxAdvancedConfig cfg;
    cfg.opportunistic = opportunistic;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::OrthoMads).with_mads_config(cfg).solve();
    out.writeln("opportunistic", opportunistic, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
opportunistic true : evaluations = 402 , minimum = 0.00000000
opportunistic false : evaluations = 516 , minimum = 0.00000000
```

### Example 2

Constraint handling, for the constraint `x₀ ≤ 1` on a black box. `Ignore` pays no attention to it and returns the unconstrained optimum `(3, 3)`; the two barrier modes keep every accepted point feasible. Both reach the exact answer `(1, 3)`: when a poll fails on the constraint boundary, the search also polls along the boundary, using the constraint values the failed poll has just computed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  for (const dfo::ConstraintHandling mode : {dfo::ConstraintHandling::Ignore, dfo::ConstraintHandling::ExtremeBarrier, dfo::ConstraintHandling::ProgressiveBarrier}) {
    dfo::MadsBlackboxAdvancedConfig cfg;
    cfg.constraint_handling = mode;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_blackbox([&](const Series &x) {
                                 dfo::BlackboxEvaluation e;
                                 e.objectives = f(x);
                                 e.constraints = x.select(0, 0) - 1.0;
                                 return e;
                               }).with_solver(dfo::DfoSolverKind::OrthoMads).with_mads_config(cfg).solve();
    out.writeln("mode", static_cast<int>(mode), ": minimizer =", r.minimizer.items<double>());
  }
}
```

Output

```text
mode 0 : minimizer = [3.000, 3.000]
mode 1 : minimizer = [1.000, 3.000]
mode 2 : minimizer = [1.000, 3.000]
```

### Example 3

The search step. `search_points` adds a number of space-filling trial points before each poll; on the Rosenbrock valley it finds descent the poll alone is slow to find.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const std::int64_t points : {0, 20}) {
    dfo::MadsBlackboxAdvancedConfig cfg;
    cfg.search_points = points;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_solver(dfo::DfoSolverKind::OrthoMads).with_mads_config(cfg).with_max_evaluations(1500).solve();
    out.writeln("search points", points, ": minimum =", r.minimum);
  }
}
```

Output

```text
search points 0 : minimum = 0.00163020
search points 20 : minimum = 0.00000000
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_initial.html">
    <span class="label">Previous</span>
    <span class="title">with_initial</span>
  </a>
  <a class="next-link" href="with_max_evaluations.html">
    <span class="label">Next</span>
    <span class="title">with_max_evaluations</span>
  </a>
</div>
<!-- apiref:nav:end -->
