<!-- generated from with_check_every.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 25e1886f9343 -->

# `DfoBuilder::with_check_every`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets how often the improvement test is applied.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_check_every(std::int64_t check_every) const;
```

## What It Does

Sets `DfoOptions::check_every`: the improvement-based stopping test of the population and mesh methods is evaluated every this many iterations. A larger value lets a slow run continue through a flat stretch; a smaller value stops sooner.

## Parameters

`std::int64_t check_every`
: Iterations between tests.

## Returns

A new builder with the change; the original is untouched.

## Practical Examples

### Example 1

Checking often and rarely.

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

  for (const std::int64_t k : {1, 25}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f)
                                 .with_solver(dfo::DfoSolverKind::DifferentialEvolution).with_tolerance(1e-6).with_check_every(k).solve();
    out.writeln("check every", k, ": generations =", r.diagnostics.iterations, ", minimum =", r.minimum);
  }
}
```

Output

```text
check every 1 : generations = 75 , minimum = 0.00000883
check every 25 : generations = 375 , minimum = 0.00000000
```

### Example 2

On a swarm.

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

  for (const std::int64_t k : {1, 25}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0, 0.0, 0.0})).with_objective(f)
                                 .with_solver(dfo::DfoSolverKind::ParticleSwarm).with_tolerance(1e-6).with_check_every(k).solve();
    out.writeln("check every", k, ": generations =", r.diagnostics.iterations, ", minimum =", r.minimum);
  }
}
```

Output

```text
check every 1 : generations = 30 , minimum = 0.00000000
check every 25 : generations = 75 , minimum = 0.00000000
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
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  for (const std::int64_t k : {1, 25}) {
    const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::OrthoMads).with_tolerance(1e-4).with_check_every(k).solve();
    out.writeln("check every", k, ": iterations =", r.diagnostics.iterations, ", evaluations =", r.diagnostics.evaluations);
  }
}
```

Output

```text
check every 1 : iterations = 79 , evaluations = 253
check every 25 : iterations = 125 , evaluations = 401
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_bounds.html">
    <span class="label">Previous</span>
    <span class="title">with_bounds</span>
  </a>
  <a class="next-link" href="with_constraints.html">
    <span class="label">Next</span>
    <span class="title">with_constraints</span>
  </a>
</div>
<!-- apiref:nav:end -->
