<!-- generated from with_blackbox.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f12ae7d07681 -->

# `DfoBuilder::with_blackbox`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets a black-box evaluation that returns objectives, constraints and a success flag.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_blackbox(std::function<dfo::BlackboxEvaluation(const pluss::Series &)> blackbox) const;
template <class Fn> dfo::DfoBuilder dfo::DfoBuilder::with_blackbox(Fn &&blackbox) const;
```

## What It Does

A black box may return a `BlackboxEvaluation` (objectives, constraints, a flag for a failed evaluation and a flag for whether to count it), a `Series` of objectives or a plain number. With a black box the front door runs the mesh-adaptive search (`MadsAdvanced`), the one method that handles several objectives, constraints and failed evaluations together; constraint values the black box returns are used only when `with_mads_config` chooses a barrier (see `ConstraintHandling`).

## Parameters

`std::function<dfo::BlackboxEvaluation(const pluss::Series &)> blackbox`
: The evaluation, or a callable returning a `BlackboxEvaluation`, a `Series` or a number.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

A number-returning black box.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_blackbox([&](const Series &x) { return f(x).item<double>(); }).solve();
  out.writeln(dfo::to_string(r.selected_solver), ", minimizer =", r.minimizer.items<double>());
}
```

Output

```text
MadsAdvanced , minimizer = [3.0000, 3.0000]
```

### Example 2

A failing simulation. Returning `evaluation_ok = false` marks a point as failed; the search treats it as unusable and goes elsewhere. The simulator here fails where `a + b ≥ 5`, so the best usable point is the nearest one to `(3, 3)` on the line `a + b = 5`, around `(2.5, 2.5)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_blackbox([](const Series &x) {
    dfo::BlackboxEvaluation e;
    const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
    e.objectives = pluss::make_series((a - 3.0) * (a - 3.0) + (b - 3.0) * (b - 3.0));
    e.evaluation_ok = a + b < 5.0;   // the simulator fails above the line a + b = 5
    return e;
  }).solve();
  out.writeln("minimizer =", r.minimizer.items<double>(), ", sum =", r.minimizer.sum().item<double>());
}
```

Output

```text
minimizer = [2.5340, 2.4660] , sum = 5.0000
```

### Example 3

A black box with a constraint. `constraints` holds `g(x) ≤ 0`; here `x₀ + x₁ ≤ 2`. The constraint is used once the configuration chooses a barrier; with the default, `Ignore`, it would only be reported.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::MadsBlackboxAdvancedConfig config;
  config.constraint_handling = dfo::ConstraintHandling::ProgressiveBarrier;
  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_mads_config(config).with_blackbox([&](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = f(x);
    e.constraints = x.sum() - 2.0;
    return e;
  }).solve();
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.0000, 1.0000]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_auto_policy.html">
    <span class="label">Previous</span>
    <span class="title">with_auto_policy</span>
  </a>
  <a class="next-link" href="with_bounds.html">
    <span class="label">Next</span>
    <span class="title">with_bounds</span>
  </a>
</div>
<!-- apiref:nav:end -->
