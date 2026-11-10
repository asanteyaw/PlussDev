<!-- generated from problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fd73af5b8cb -->

# `DfoBuilder::problem`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Returns the problem collected so far.

## Signature

```cpp
const dfo::DfoProblem &dfo::DfoBuilder::problem() const noexcept;
```

## What It Does

Read-only access to the [`DfoProblem`](../DfoProblem.md) the builder holds. Useful for checking what a chain assembled, or for handing the problem to another call.

## Returns

A reference to the problem; valid while the builder lives.

## Practical Examples

### Example 1

Reading the start.

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

  const dfo::DfoBuilder b = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f);
  out.writeln("start =", b.problem().initial.items<double>(), ", has objective =", static_cast<bool>(b.problem().objective));
}
```

Output

```text
start = [0.0000, 0.0000] , has objective = true
```

### Example 2

Bounds set by `with_bounds`.

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

  const dfo::DfoBuilder b = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_bounds(pluss::series::make({0.0, 0.0}), pluss::series::make({2.0, 2.0}));
  out.writeln("lower =", b.problem().lower_bounds.items<double>(), ", upper =", b.problem().upper_bounds.items<double>());
}
```

Output

```text
lower = [0.0000, 0.0000] , upper = [2.0000, 2.0000]
```

### Example 3

Handing the problem on. A builder's problem can be solved directly by the front door.

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

  const dfo::DfoBuilder b = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f);
  const dfo::DfoResult r = dfo::DFO::solve(b.problem());
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [3.0000, 3.0000]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="options.html">
    <span class="label">Previous</span>
    <span class="title">options</span>
  </a>
  <a class="next-link" href="solve.html">
    <span class="label">Next</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
