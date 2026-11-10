<!-- generated from lbs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b28c4a8508a4 -->

# `ProblemBuilder::lbs`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Sets lower bounds on the unknowns.

## Signature

```cpp
optim::ProblemBuilder &optim::ProblemBuilder::lbs(const pluss::Series &lower);
optim::ProblemBuilder &optim::ProblemBuilder::lbs(const optim::ParameterPack<pluss::Series> &lower);
```

## What It Does

`lbs(lower)` requires `x ≥ lower` componentwise. The `Series` form applies to the single block `x`, with one entry per unknown; the pack form gives bounds block by block, with the same block names and sizes as the start. Use `-inf` for "no lower bound" on an entry. A pack that names a block the problem does not have is an error. Bounds are respected by the iterates of the interior method strictly and by the others up to tolerance, and they are never rescaled.

## Parameters

`lower`
: A `Series` for block `x`, or a pack with one block per unknown.

## Returns

The same builder, for chaining.

## Errors

- A bound pack naming a missing block, or of the wrong size, is rejected when `optima` starts.

## Practical Examples

### Example 1

A Series of bounds on `x`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});

  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); }).lbs(pluss::series::make({1.5, 0.0})), pluss::series::make({2.0, 2.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.5000, 2.0000]
```

### Example 2

A pack of bounds, block by block, with one block left unbounded.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double inf = std::numeric_limits<double>::infinity();
  const Pack start = optim::pack_scalars({{"a", pluss::make_series(1.0)}, {"b", pluss::make_series(1.0)}});
  const Pack lower = optim::pack_scalars({{"a", pluss::make_series(0.5)}, {"b", pluss::make_series(-inf)}});
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return p.at("a").square() + (p.at("b") + 3.0).square(); }).lbs(lower), start);
  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
a = [0.5000]  b = [-3.0000]
```

### Example 3

A bound that is not active does nothing. The minimum of `(x − 3)²` is already above the lower bound `0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("x") - 3.0).square().sum(); }).lbs(pluss::series::make({0.0})), pluss::series::make({1.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [3.0000]
```

## See Also

- [`ubs`](ubs.md): upper bounds
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="build.html">
    <span class="label">Previous</span>
    <span class="title">build</span>
  </a>
  <a class="next-link" href="ln_eq.html">
    <span class="label">Next</span>
    <span class="title">ln_eq</span>
  </a>
</div>
<!-- apiref:nav:end -->
