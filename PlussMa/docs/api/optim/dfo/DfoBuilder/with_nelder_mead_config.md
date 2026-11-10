<!-- generated from with_nelder_mead_config.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1927cfedde17 -->

# `DfoBuilder::with_nelder_mead_config`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the settings of the Nelder-Mead method.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_nelder_mead_config(const dfo::NelderMeadConfig &cfg) const;
```

## What It Does

Sets `DfoOptions::nelder_mead`, a [`NelderMeadConfig`](../NelderMeadConfig.md) that carries every setting of the Nelder-Mead method. It is read only when that method runs, whether chosen by `Auto` or by [`with_solver`](with_solver.md). The common settings (budgets, tolerance, seed, display) and the bounds of the problem are applied on top of it when the method starts.

## Parameters

`const dfo::NelderMeadConfig &cfg`
: The Nelder-Mead settings.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

A larger initial simplex. `simplex_delta` is the relative size of the first simplex around the start.

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

  for (const double delta : {0.05, 0.5}) {
    dfo::NelderMeadConfig cfg;
    cfg.simplex_delta = delta;
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_nelder_mead_config(cfg).solve();
    out.writeln("delta", delta, ": evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
  }
}
```

Output

```text
delta 0.05000000 : evaluations = 159 , minimum = 0.00000000
delta 0.50000000 : evaluations = 190 , minimum = 0.00000000
```

### Example 2

Tighter tolerances.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::NelderMeadConfig cfg;
  cfg.position_tolerance = 1e-10;
  cfg.value_tolerance = 1e-10;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_nelder_mead_config(cfg).solve();
  out.writeln("evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
}
```

Output

```text
evaluations = 249 , minimum = 0.000000000000
```

### Example 3

A more aggressive reflection and expansion.

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

  dfo::NelderMeadConfig cfg;
  cfg.expansion = 3.0;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({-1.2, 1.0})).with_objective(rosenbrock).with_nelder_mead_config(cfg).solve();
  out.writeln("evaluations =", r.diagnostics.evaluations, ", minimum =", r.minimum);
}
```

Output

```text
evaluations = 184 , minimum = 0.00000000
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_max_total_evaluations.html">
    <span class="label">Previous</span>
    <span class="title">with_max_total_evaluations</span>
  </a>
  <a class="next-link" href="with_objective.html">
    <span class="label">Next</span>
    <span class="title">with_objective</span>
  </a>
</div>
<!-- apiref:nav:end -->
