<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7569af35e307 -->

# `DfoBuilder::solve`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Runs the collected problem with the collected options.

## Signature

```cpp
dfo::DfoResult dfo::DfoBuilder::solve() const;
```

## What It Does

Equivalent to `DFO::solve(problem(), options())`. The builder is not changed, so a base builder can be solved, extended and solved again.

## Returns

A [`DfoResult`](../DfoResult.md).

## Errors

- A missing start, objective or black box, or conflicting parts, throw.

## Practical Examples

### Example 1

A complete chain.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).with_seed(11).solve();
  out.writeln(dfo::to_string(r.selected_solver), ", minimizer =", r.minimizer.items<double>());
}
```

Output

```text
DifferentialEvolution , minimizer = [2.0000, 2.0000]
```

### Example 2

Solving a base and an extension.

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

  const dfo::DfoBuilder base = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f);
  out.writeln("free   :", base.solve().minimizer.items<double>());
  out.writeln("boxed  :", base.with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0})).solve().minimizer.items<double>());
}
```

Output

```text
free   : [3.0000, 3.0000]
boxed  : [2.0000, 2.0000]
```

### Example 3

An incomplete builder is rejected by name.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  try {
    (void)dfo::DFO::build(pluss::series::make({0.0})).solve();
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
DFO::solve: provide either objective or blackbox callback
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="problem.html">
    <span class="label">Previous</span>
    <span class="title">problem</span>
  </a>
  <a class="next-link" href="with_auto_policy.html">
    <span class="label">Next</span>
    <span class="title">with_auto_policy</span>
  </a>
</div>
<!-- apiref:nav:end -->
