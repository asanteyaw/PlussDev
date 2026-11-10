<!-- generated from PSDisplay.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSDisplay`

Whether a pattern-search run prints its progress.

`None` is silent; `Iter` prints one line per iteration to the standard output. Set as `PSConfig::display`; through the front door use `DfoOptions::display_iter`.

## Fields

### `None`

`enumerator`, default `0`.

No output.

### `Iter`

`enumerator`, default `1`.

One line per iteration.

## Guidance

Set as `PSConfig::display`.

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

  dfo::PSConfig config;
  config.display = dfo::PSDisplay::Iter;
  config.max_iterations = 3;
  dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
}
```

Output

```text
iter=0 evals=1 f=24.2 merit=24.2 cv=0 mesh=1 step=bootstrap
iter=1 evals=2 f=1.57636 merit=1.57636 cv=0 mesh=2 step=successful_poll
iter=2 evals=7 f=1.57636 merit=1.57636 cv=0 mesh=1 step=refine_mesh
iter=3 evals=9 f=1.12099 merit=1.12099 cv=0 mesh=2 step=successful_poll
```

### Example 2

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
  out.writeln("default is None =", dfo::PSConfig{}.display == dfo::PSDisplay::None);
}
```

Output

```text
default is None = true
```

### Example 3

Switching it off again.

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

  dfo::PSConfig config;
  config.display = dfo::PSDisplay::None;
  config.max_iterations = 3;
  out.writeln(dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).status));
}
```

Output

```text
ReachedIterationBudget
```

## See Also

- [`PSConfig`](PSConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSDirectionFamily.html">
    <span class="label">Previous</span>
    <span class="title">PSDirectionFamily</span>
  </a>
  <a class="next-link" href="PSMeshScaling.html">
    <span class="label">Next</span>
    <span class="title">PSMeshScaling</span>
  </a>
</div>
<!-- apiref:nav:end -->
