<!-- generated from with_display_iter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a2d1b6e8229a -->

# `DfoBuilder::with_display_iter`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Turns the per-iteration display on or off.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_display_iter(bool enabled = true) const;
```

## What It Does

Sets `DfoOptions::display_iter`. When on, the method prints one line per iteration to the standard output: the iteration number, the evaluation count, the best value and the name of the step taken (`phase`); the mesh-adaptive methods add the mesh and frame sizes. The display does not change the run.

## Parameters

`bool enabled`
: Whether to print; default true.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Watching a short run.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::NelderMead).with_max_iterations(4).with_display_iter().solve();
  out.writeln("done:", dfo::to_string(r.status));
}
```

Output

```text
iter=1 evals=3 f=17.9985 phase=seed
iter=2 evals=5 f=17.9955 phase=expand
iter=3 evals=6 f=17.9955 phase=reflect
iter=4 evals=8 f=17.9895 phase=expand
done: ReachedIterationBudget
```

### Example 2

Turning it off again.

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

  const dfo::DfoBuilder noisy = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_max_iterations(3).with_display_iter();
  const dfo::DfoResult r = noisy.with_display_iter(false).solve();
  out.writeln("quiet run:", dfo::to_string(r.status), ", display flag =", noisy.with_display_iter(false).options().display_iter);
}
```

Output

```text
quiet run: ReachedIterationBudget , display flag = false
```

### Example 3

On a mesh method.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::OrthoMads).with_max_iterations(3).with_display_iter().solve();
  out.writeln("done:", dfo::to_string(r.status));
}
```

Output

```text
iter=0 evals=1 f=18 phase=bootstrap mesh=0.01 frame=0.02
iter=1 evals=2 f=17.8804 phase=accept_poll mesh=0.04 frame=0.04
iter=2 evals=3 f=17.642 phase=accept_poll mesh=0.08 frame=0.08
iter=3 evals=4 f=17.1748 phase=accept_poll mesh=0.16 frame=0.16
done: ReachedIterationBudget
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_de_prmm_config.html">
    <span class="label">Previous</span>
    <span class="title">with_de_prmm_config</span>
  </a>
  <a class="next-link" href="with_initial.html">
    <span class="label">Next</span>
    <span class="title">with_initial</span>
  </a>
</div>
<!-- apiref:nav:end -->
