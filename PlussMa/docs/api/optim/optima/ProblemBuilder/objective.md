<!-- generated from objective.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8d62eeee0dc1 -->

# `ProblemBuilder::objective`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Sets the objective to a function of the parameters, differentiated automatically.

## Signature

```cpp
template <typename Fn> optim::ProblemBuilder &optim::ProblemBuilder::objective(Fn &&fn);
```

## What It Does

`fn` receives the parameter pack (or, when it is written to take a single `Series`, the block `x`) and returns a scalar `Series`. Everything the function does with `Series` arithmetic is recorded, and the solver reads the exact gradient (and, for the algorithms that use curvature, Hessian products) from that record; you never differentiate by hand. The function is called many times, at points the solver chooses, so it should be pure. Calling `objective` again replaces the previous one. A problem needs exactly one objective, from this method or from [`objective_analytic`](objective_analytic.md).

## Parameters

`Fn &&fn`
: A callable `Pack -> Series` or `Series (Series)` returning a scalar.

## Returns

The same builder, for chaining.

## Errors

- A non-scalar result is rejected when `optima` starts.

## Practical Examples

### Example 1

A pack-taking objective, read by block name.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::OptimaState result = optim::optima(optim::problem().objective([](const Pack &p) { return (p.at("x") - 3.0).square().sum(); }), pluss::series::make({0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [3.0000]
```

### Example 2

A Series-taking objective: the function is written on the vector directly, which suits problems with one block.

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

  const optim::OptimaState result = optim::optima(optim::problem().objective([&](const pluss::Series &x) { return (x - target).square().sum(); }), pluss::series::make({0.0, 0.0}));
  out.writeln("x =", result.params.at("x").items<double>());
}
```

Output

```text
x = [1.0000, 2.0000]
```

### Example 3

Several named blocks in one objective. Minimise `(a − 1)² + (b − 2)² + ab` from `(0, 0)`: the cross term bends the optimum to `(0, 2)`.

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
  const optim::OptimaState result = optim::optima(
      optim::problem().objective([](const Pack &p) { return (p.at("a") - 1.0).square() + (p.at("b") - 2.0).square() + p.at("a") * p.at("b"); }), start);
  out.writeln("a =", result.params.at("a").items<double>(), " b =", result.params.at("b").items<double>());
}
```

Output

```text
a = [0.0000]  b = [2.0000]
```

## See Also

- [`objective_analytic`](objective_analytic.md): supply the gradient yourself
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nonlinear_inequality.html">
    <span class="label">Previous</span>
    <span class="title">nonlinear_inequality</span>
  </a>
  <a class="next-link" href="objective_analytic.html">
    <span class="label">Next</span>
    <span class="title">objective_analytic</span>
  </a>
</div>
<!-- apiref:nav:end -->
