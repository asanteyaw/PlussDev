<!-- generated from with_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc636343002b -->

# `DfoBuilder::with_constraints`

[Back to `dfo::DfoBuilder`](../DfoBuilder.md)

Sets inequality constraints `g(x) ≤ 0`.

## Signature

```cpp
dfo::DfoBuilder dfo::DfoBuilder::with_constraints(std::function<pluss::Series(const pluss::Series &)> constraints) const;
template <class Fn> dfo::DfoBuilder dfo::DfoBuilder::with_constraints(Fn &&constraints) const;
```

## What It Does

The callable returns `g(x)` as a scalar or a vector, and the point is feasible when every element is `≤ 0`. With constraints the front door runs pattern search. Use [`with_constraints_values`](with_constraints_values.md) when equality constraints are needed too. Give `with_constraints` **or** `with_constraints_values`, not both.

## Parameters

`std::function<pluss::Series(const pluss::Series &)> constraints`
: `g(x)`, a `Series` or a number; feasible when every element is `≤ 0`.

## Returns

A new builder with the change; the original is untouched.

## Errors

- Also setting `constraints_values` throws when the problem is solved.
- Choosing Nelder-Mead, differential evolution or a swarm with constraints throws.

## Practical Examples

### Example 1

One constraint. The unconstrained optimum `(3, 3)` violates `x₀ + x₁ ≤ 2`; the answer sits on the line.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_constraints([](const Series &x) { return x.sum() - 2.0; }).solve();
  out.writeln("minimizer =", r.minimizer.items<double>(), ", solver =", dfo::to_string(r.selected_solver));
}
```

Output

```text
minimizer = [1.0007, 0.9993] , solver = PatternSearch
```

### Example 2

A vector of constraints: a disc `x₀² + x₁² ≤ 2` and a half-plane `x₀ ≤ 1`.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_constraints([](const Series &x) {
    return (x.square().sum() - 2.0).reshape({1}).unite({(x.select(0, 0) - 1.0).reshape({1})}, 0);
  }).solve();
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.0000, 1.0000]
```

### Example 3

A constraint that returns a number.

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

  const dfo::DfoResult r = dfo::DFO::build(pluss::series::make({0.0, 0.0})).with_objective(f).with_constraints([](const Series &x) { return x.sum().item<double>() - 2.0; }).solve();
  out.writeln("minimizer =", r.minimizer.items<double>());
}
```

Output

```text
minimizer = [1.0007, 0.9993]
```

## See Also

- [`DfoBuilder`](../DfoBuilder.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_check_every.html">
    <span class="label">Previous</span>
    <span class="title">with_check_every</span>
  </a>
  <a class="next-link" href="with_constraints_values.html">
    <span class="label">Next</span>
    <span class="title">with_constraints_values</span>
  </a>
</div>
<!-- apiref:nav:end -->
