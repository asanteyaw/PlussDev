<!-- generated from SolverDisplay.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::SolverDisplay`

Whether a Nelder-Mead run prints its progress.

`None` is silent. `Iter` prints one line per iteration (iteration, evaluations, best value, step name) to the standard output. Set as `NelderMeadConfig::display`; through the front door use `DfoOptions::display_iter`.

## Fields

### `None`

`enumerator`, default `0`.

No output.

### `Iter`

`enumerator`, default `1`.

One line per iteration.

## Guidance

Set as `NelderMeadConfig::display`.

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

  dfo::NelderMeadConfig config;
  config.display = dfo::SolverDisplay::Iter;
  config.max_iterations = 4;
  dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
}
```

Output

```text
iter=1 evals=3 f=20.05 phase=seed
iter=2 evals=5 f=5.1618 phase=expand
iter=3 evals=7 f=4.4978 phase=reflect
iter=4 evals=9 f=4.4978 phase=adjust_outer
```

### Example 2

Silent by default.

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

  dfo::NelderMeadConfig config;
  out.writeln("default is None =", config.display == dfo::SolverDisplay::None);
}
```

Output

```text
default is None = true
```

### Example 3

Turning it on and off.

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

  dfo::NelderMeadConfig config;
  config.max_iterations = 2;
  config.display = dfo::SolverDisplay::None;
  out.writeln("quiet:", dfo::to_string(dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).status));
}
```

Output

```text
quiet: ReachedIterationBudget
```

## See Also

- [`NelderMeadConfig`](NelderMeadConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PsdMadsConfig.html">
    <span class="label">Previous</span>
    <span class="title">PsdMadsConfig</span>
  </a>
  <a class="next-link" href="SolverStatus.html">
    <span class="label">Next</span>
    <span class="title">SolverStatus</span>
  </a>
</div>
<!-- apiref:nav:end -->
