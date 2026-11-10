<!-- generated from BlackboxSolverDisplay.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::BlackboxSolverDisplay`

Whether a mesh-adaptive run prints its progress.

`None` is silent; `Iter` prints one line per iteration (iteration, evaluations, best value, the kind of iteration, mesh and frame sizes) to the standard output. Set as the `display` member of the MADS configurations; through the front door use `DfoOptions::display_iter`.

## Fields

### `None`

`enumerator`, default `0`.

No output.

### `Iter`

`enumerator`, default `1`.

One line per iteration.

## Guidance

Set as `display` in a MADS configuration.

## Practical Examples

### Example 1

Printing a short run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::MadsBlackboxConfig config;
  config.display = dfo::BlackboxSolverDisplay::Iter;
  config.max_iterations = 4;
  dfo::orthomads_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
}
```

Output

```text
iter=0 evals=1 f=24.2 phase=bootstrap mesh=0.12 frame=0.24
iter=1 evals=2 f=4.45626 phase=accept_poll mesh=0.48 frame=0.48
iter=2 evals=6 f=4.45626 phase=refine_mesh mesh=0.12 frame=0.24
iter=3 evals=10 f=4.45626 phase=refine_mesh mesh=0.03 frame=0.12
iter=4 evals=14 f=3.7477 phase=accept_poll mesh=0.12 frame=0.24
```

### Example 2

The black-box form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 3.0).square().sum().reshape({1});   // minimum 0 at (3, 3)
    return e;
  };

  dfo::MadsBlackboxAdvancedConfig config;
  config.display = dfo::BlackboxSolverDisplay::Iter;
  config.max_iterations = 3;
  dfo::mads_blackbox_minimizer(black_box, pluss::series::make({0.0, 0.0}), config);
}
```

Output

```text
iter=0 evals=1 f=18 phase=bootstrap mesh=0.01 frame=0.02
iter=1 evals=2 f=17.8804 phase=accept_poll mesh=0.04 frame=0.04
iter=2 evals=3 f=17.642 phase=accept_poll mesh=0.08 frame=0.08
iter=3 evals=4 f=17.1748 phase=accept_poll mesh=0.16 frame=0.16
```

### Example 3

The default is silent.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("default is None =", dfo::MadsBlackboxConfig{}.display == dfo::BlackboxSolverDisplay::None);
}
```

Output

```text
default is None = true
```

## See Also

- [`MadsBlackboxConfig`](MadsBlackboxConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="BlackboxSolverDiagnostics.html">
    <span class="label">Previous</span>
    <span class="title">BlackboxSolverDiagnostics</span>
  </a>
  <a class="next-link" href="BlackboxSolverOutcome.html">
    <span class="label">Next</span>
    <span class="title">BlackboxSolverOutcome</span>
  </a>
</div>
<!-- apiref:nav:end -->
