<!-- generated from build.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3966a33f3daa -->

# `ProblemBuilder::build`

[Back to `optim::ProblemBuilder`](../ProblemBuilder.md)

Produces the finished `ConstrainedProblem` from the builder.

## Signature

```cpp
optim::ConstrainedProblem optim::ProblemBuilder::build() const;
```

## What It Does

`build()` copies the objective and constraints collected so far into a `ConstrainedProblem` value. The builder converts to a `ConstrainedProblem` implicitly, so `build()` is only needed to name the type, to hold a problem in a variable, or to keep a builder going after taking a snapshot of it.

## Returns

A `ConstrainedProblem`.

## Practical Examples

### Example 1

Naming the problem type.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::ConstrainedProblem problem = optim::problem().objective([](const Pack &p) { return (p.at("x") - 1.0).square().sum(); }).build();
  out.writeln("x =", optim::optima(problem, pluss::series::make({0.0})).params.at("x").items<double>());
}
```

Output

```text
x = [1.0000]
```

### Example 2

Snapshots. The builder keeps going after `build()`; each snapshot is an independent problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  auto builder = optim::problem().objective([](const Pack &p) { return (p.at("x") - 4.0).square().sum(); });
  const optim::ConstrainedProblem free_problem = builder.build();
  const optim::ConstrainedProblem capped = builder.ubs(pluss::series::make({1.0})).build();
  out.writeln("free   x =", optim::optima(free_problem, pluss::series::make({0.0})).params.at("x").items<double>());
  out.writeln("capped x =", optim::optima(capped, pluss::series::make({0.0})).params.at("x").items<double>());
}
```

Output

```text
free   x = [4.000]
capped x = [1.000]
```

### Example 3

Implicit conversion. The builder can be passed where a problem is expected, so `build()` can be omitted.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::ProblemBuilder builder = optim::problem().objective([](const Pack &p) { return (p.at("x") - 2.0).square().sum(); });
  const optim::ConstrainedProblem implicit = builder;
  out.writeln("x =", optim::optima(implicit, pluss::series::make({0.0})).params.at("x").items<double>());
}
```

Output

```text
x = [2.0000]
```

## See Also

- [`optima`](../optima.md): solves it
- [`ProblemBuilder`](Introduction.md): the builder
- [`optima`](../optima.md): solves the problem

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="lbs.html">
    <span class="label">Next</span>
    <span class="title">lbs</span>
  </a>
</div>
<!-- apiref:nav:end -->
