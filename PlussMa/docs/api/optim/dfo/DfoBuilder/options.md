<!-- generated from options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4e725a2c6d76 -->

# `DfoBuilder::options`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Returns the options collected so far.

## Signature

```cpp
const dfo::DfoOptions &dfo::DfoBuilder::options() const noexcept;
```

## What It Does

Read-only access to the [`DfoOptions`](../DfoOptions.md) the builder holds.

## Returns

A reference to the options; valid while the builder lives.

## Practical Examples

### Example 1

Reading settings back.

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

  const dfo::DfoBuilder b = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_seed(9).with_max_evaluations(120);
  out.writeln("seed =", b.options().rng_seed, ", budget =", b.options().max_evaluations, ", solver =", dfo::to_string(b.options().solver));
}
```

Output

```text
seed = 9 , budget = 120 , solver = Auto
```

### Example 2

The defaults of an empty builder.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const dfo::DfoBuilder b;
  out.writeln("solver =", dfo::to_string(b.options().solver), ", check every =", b.options().check_every, ", seed =", b.options().rng_seed);
}
```

Output

```text
solver = Auto , check every = 10 , seed = 5489
```

### Example 3

Reusing the options elsewhere.

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

  const dfo::DfoBuilder b = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_solver(dfo::DfoSolverKind::PatternSearch);
  const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({5.0, 5.0}), b.options());
  out.writeln(dfo::to_string(r.selected_solver), ", minimizer =", r.minimizer.items<double>());
}
```

Output

```text
PatternSearch , minimizer = [3.0000, 3.0000]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="problem.html">
    <span class="label">Next</span>
    <span class="title">problem</span>
  </a>
</div>
<!-- apiref:nav:end -->
