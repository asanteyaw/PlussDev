<!-- generated from ubs.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ce51785891e2 -->

# `ProblemBuilder::ubs`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Sets upper bounds on the unknowns.

## Signature

```cpp
optim::ProblemBuilder &optim::ProblemBuilder::ubs(const pluss::Series &upper);
optim::ProblemBuilder &optim::ProblemBuilder::ubs(const optim::ParameterPack<pluss::Series> &upper);
```

## What It Does

`ubs(upper)` requires `x ≤ upper` componentwise, with the same two forms and the same rules as [`lbs`](lbs.md): a `Series` for the block `x`, or a pack with matching block names; `+inf` means no bound on an entry. A lower bound above its upper bound is an error.

## Parameters

`upper`
: A `Series` for block `x`, or a pack with one block per unknown.

## Returns

The same builder, for chaining.

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

  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); }).ubs(pluss::series::make({0.5, 5.0})), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [0.5000, 2.0000]
```

### Example 2

A box: both bounds. The target `(3, −2)` is outside the box `[0,2]²`, so the answer is the nearest corner.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({3.0, -2.0});
  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const Pack &p) { return (p.at("x") - target).square().sum(); })
      .lbs(pluss::series::make({0.0, 0.0})).ubs(pluss::series::make({2.0, 2.0})), pluss::series::make({1.0, 1.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [2.0000, 0.0000]
```

### Example 3

A pack of upper bounds, block by block.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const Pack start = optim::pack_scalars({{"a", pluss::make_series(0.0)}, {"b", pluss::make_series(0.0)}});
  const Pack upper = optim::pack_scalars({{"a", pluss::make_series(0.5)}, {"b", pluss::make_series(10.0)}});
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("a") - 2.0).square() + (p.at("b") - 3.0).square(); }).ubs(upper), start);
  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
a = [0.5000]  b = [3.0000]
```

## See Also

- [`lbs`](lbs.md): lower bounds
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="objective_analytic.html">
    <span class="label">Previous</span>
    <span class="title">objective_analytic</span>
  </a>
</div>
<!-- apiref:nav:end -->
