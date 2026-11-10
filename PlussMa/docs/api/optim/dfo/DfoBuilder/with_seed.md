<!-- generated from with_seed.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4bc23d5113aa -->

# `DfoBuilder::with_seed`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets the seed of the random numbers.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_seed(std::uint64_t seed) const;
```

## What It Does

Sets `DfoOptions::rng_seed`. The evolutionary and swarm methods and the randomised polls of pattern search and LT-MADS draw random numbers; a fixed seed makes a run exactly repeatable and a different seed gives an independent run, which is how restarts are made.

## Parameters

`std::uint64_t seed`
: The seed.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Repeatability: the same seed gives the same run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  const dfo::DfoBuilder base = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f)
                                   .with_solver(dfo::DfoSolverKind::DifferentialEvolution).with_max_iterations(30);
  for (const std::uint64_t seed : {3, 3}) out.writeln("seed", seed, ": minimum =", base.with_seed(seed).solve().minimum);
}
```

Output

```text
seed 3 : minimum = 0.01603337
seed 3 : minimum = 0.01603337
```

### Example 2

Different seeds give different runs.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - pluss::series::make({1.0, -2.0, 0.5, 3.0})).square().sum(); };   // minimum 0 at (1, -2, 0.5, 3)

  const dfo::DfoBuilder base = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f)
                                   .with_solver(dfo::DfoSolverKind::ParticleSwarm).with_max_iterations(30);
  for (const std::uint64_t seed : {1, 2, 3}) out.writeln("seed", seed, ": minimum =", base.with_seed(seed).solve().minimum);
}
```

Output

```text
seed 1 : minimum = 0.00001451
seed 2 : minimum = 0.00000252
seed 3 : minimum = 0.00000526
```

### Example 3

Restarts. A deliberately small population and few generations make single runs unreliable on `x² + 10 sin²(3x)`, which has many local minima around its global minimum at 0; running several seeds and keeping the best is the remedy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto f = [](const Series &x) { return x.square().sum() + (x * 3.0).sin().square().sum() * 10.0; };
  dfo::DifferentialEvolutionConfig cfg;
  cfg.population_size = 6;
  double best = 1e9;
  for (std::uint64_t seed = 1; seed <= 5; ++seed) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({4.0})).with_objective(f)
                                 .with_solver(dfo::DfoSolverKind::DifferentialEvolution).with_de_config(cfg)
                                 .with_bounds(pluss::series::make({-6.0}), pluss::series::make({6.0})).with_max_iterations(12).with_seed(seed).solve();
    out.writeln("seed", seed, ": minimizer =", r.minimizer.items<double>(), ", minimum =", r.minimum);
    best = std::min(best, r.minimum);
  }
  out.writeln("best =", best);
}
```

Output

```text
seed 1 : minimizer = [-0.006460] , minimum = 0.003798
seed 2 : minimizer = [-0.015503] , minimum = 0.021856
seed 3 : minimizer = [0.012301] , minimum = 0.013763
seed 4 : minimizer = [-0.075921] , minimum = 0.515615
seed 5 : minimizer = [-0.020258] , minimum = 0.037298
best = 0.003798
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_pso_dv_config.html">
    <span class="label">Previous</span>
    <span class="title">with_pso_dv_config</span>
  </a>
  <a class="next-link" href="with_solver.html">
    <span class="label">Next</span>
    <span class="title">with_solver</span>
  </a>
</div>
<!-- apiref:nav:end -->
