<!-- generated from MadsPollScheme.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::MadsPollScheme`

How the poll directions of a mesh-adaptive search are built.

`Ortho2N` builds `2n` directions from an orthogonal basis, rotated every iteration, so they are evenly spread and reproducible; this is OrthoMADS. `LtMadsRandom2N` builds them from a random lower-triangular integer matrix, the original MADS construction, and draws on `rng_seed`. The scheme is `poll_scheme` of the configuration; the functions `orthomads_minimizer` and `ltmads_minimizer` fix it, and the front door's `OrthoMads` and `LtMads` kinds set it.

## Fields

### `Ortho2N`

`enumerator`, default `0`.

`2n` orthogonal directions (OrthoMADS).

### `LtMadsRandom2N`

`enumerator`, default `1`.

`2n` random lower-triangular directions (LT-MADS).

## Guidance

Set as `MadsBlackboxConfig::poll_scheme`.

## Practical Examples

### Example 1

The two schemes.

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

  for (const auto scheme : {dfo::MadsPollScheme::Ortho2N, dfo::MadsPollScheme::LtMadsRandom2N}) {
    dfo::MadsBlackboxConfig config;
    config.poll_scheme = scheme;
    const dfo::BlackboxSolverOutcome o = dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln(static_cast<int>(scheme), ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
0 : evaluations = 1000 , minimum = 0.00853561
1 : evaluations = 1000 , minimum = 0.00494886
```

### Example 2

Through the front door.

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

  for (const auto kind : {dfo::DfoSolverKind::OrthoMads, dfo::DfoSolverKind::LtMads}) {
    dfo::DfoOptions options;
    options.solver = kind;
    const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}), options);
    out.writeln(dfo::to_string(r.selected_solver), ": evaluations =", r.diagnostics.evaluations);
  }
}
```

Output

```text
OrthoMads : evaluations = 402
LtMads : evaluations = 463
```

### Example 3

Seeds matter only for the random scheme.

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

  for (const auto scheme : {dfo::MadsPollScheme::Ortho2N, dfo::MadsPollScheme::LtMadsRandom2N}) {
    for (const std::uint64_t seed : {1, 2}) {
      dfo::MadsBlackboxConfig config;
      config.poll_scheme = scheme;
      config.rng_seed = seed;
      out.writeln(static_cast<int>(scheme), "seed", seed, ": evaluations =", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).diagnostics.evaluations);
    }
  }
}
```

Output

```text
0 seed 1 : evaluations = 1000
0 seed 2 : evaluations = 1000
1 seed 1 : evaluations = 1000
1 seed 2 : evaluations = 1000
```

## See Also

- [`MadsBlackboxConfig`](MadsBlackboxConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MadsBlackboxConfig.html">
    <span class="label">Previous</span>
    <span class="title">MadsBlackboxConfig</span>
  </a>
  <a class="next-link" href="MultiObjectiveScalarization.html">
    <span class="label">Next</span>
    <span class="title">MultiObjectiveScalarization</span>
  </a>
</div>
<!-- apiref:nav:end -->
