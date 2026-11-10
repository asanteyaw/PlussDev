<!-- generated from PSMeshScaling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSMeshScaling`

How the mesh is scaled per variable.

A single mesh size treats every variable alike, which is poor when they differ by orders of magnitude. `Off` uses one mesh size. `RangeLog` scales each variable's step by the logarithm of its bound range, so bounds of very different size are balanced; it needs bounds to act. `NupsDynamic` adapts a scale per variable from the success and failure of the polls, with the `nups_*` settings.

## Fields

### `Off`

`enumerator`, default `0`.

One mesh size.

### `RangeLog`

`enumerator`, default `1`.

Scale by the logarithm of each bound range.

### `NupsDynamic`

`enumerator`, default `2`.

Adapt a per-variable scale from poll outcomes.

## Guidance

Set as `PSConfig::mesh_scaling`.

## Practical Examples

### Example 1

A badly scaled problem: one variable of order 1 and one of order 1000, with bounds that tell the ranges. With one mesh size the search stops at 604 for the second variable (the optimum is 500); `RangeLog` reaches it in about half the evaluations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto f = [](const Series &x) { return (x.select(0, 0) - 1.0).square() + ((x.select(0, 1) - 500.0) / 1000.0).square(); };
  for (const auto scaling : {dfo::PSMeshScaling::Off, dfo::PSMeshScaling::RangeLog, dfo::PSMeshScaling::NupsDynamic}) {
    dfo::PSConfig config;
    config.mesh_scaling = scaling;
    config.lower_bounds = pluss::series::make({-10.0, 0.0});
    config.upper_bounds = pluss::series::make({10.0, 2000.0});
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(f, pluss::series::make({5.0, 100.0}), config);
    out.writeln(static_cast<int>(scaling), ": evaluations =", o.diagnostics.evaluations, ", minimizer =", o.minimizer.items<double>());
  }
}
```

Output

```text
0 : evaluations = 639 , minimizer = [1.001, 603.896]
1 : evaluations = 324 , minimizer = [1.000, 499.996]
2 : evaluations = 698 , minimizer = [0.952, 499.999]
```

### Example 2

The NUPS updates are counted in the diagnostics.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x.select(0, 0) - 1.0).square() + ((x.select(0, 1) - 500.0) / 1000.0).square(); };
  dfo::PSConfig config;
  config.mesh_scaling = dfo::PSMeshScaling::NupsDynamic;
  config.lower_bounds = pluss::series::make({-10.0, 0.0});
  config.upper_bounds = pluss::series::make({10.0, 2000.0});
  out.writeln("updates =", dfo::pattern_search_minimizer(f, pluss::series::make({5.0, 100.0}), config).diagnostics.nups_updates);
}
```

Output

```text
updates = 192
```

### Example 3

NUPS growth and contraction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto f = [](const Series &x) { return (x.select(0, 0) - 1.0).square() + ((x.select(0, 1) - 500.0) / 1000.0).square(); };
  for (const double growth : {1.2, 2.5}) {
    dfo::PSConfig config;
    config.mesh_scaling = dfo::PSMeshScaling::NupsDynamic;
    config.nups_expansion = growth;
    config.lower_bounds = pluss::series::make({-10.0, 0.0});
    config.upper_bounds = pluss::series::make({10.0, 2000.0});
    out.writeln("nups_expansion", growth, ": evaluations =", dfo::pattern_search_minimizer(f, pluss::series::make({5.0, 100.0}), config).diagnostics.evaluations);
  }
}
```

Output

```text
nups_expansion 1.20 : evaluations = 232
nups_expansion 2.50 : evaluations = 237
```

## See Also

- [`PSConfig`](PSConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSDisplay.html">
    <span class="label">Previous</span>
    <span class="title">PSDisplay</span>
  </a>
  <a class="next-link" href="PSOutcome.html">
    <span class="label">Next</span>
    <span class="title">PSOutcome</span>
  </a>
</div>
<!-- apiref:nav:end -->
