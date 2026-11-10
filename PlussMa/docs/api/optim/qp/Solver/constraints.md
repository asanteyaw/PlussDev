<!-- generated from constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: df040ee76639 -->

# `qp::Solver::constraints`

[Back to `qp::Solver`](../Solver.md)

The number of constraint rows `m` of the prepared problem: the length of `lower`, `upper` and of `A x`.

## Signature

```cpp
std::int64_t qp::Solver::constraints() const noexcept;
```

## What It Does

The number of rows of `A`, which is the length of the `lower` and `upper` vectors that [`solve`](solve.md) expects and of the constraint values `z` and multipliers `y` in the result. It is independent of the number of variables: a problem may have more or fewer constraints than variables.

## Returns

The number of constraint rows, a non-negative integer.

## Practical Examples

### Example 1

Two rows of constraints on three variables.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const qp::Solver solver = qp::prepare(pluss::series::eye(3, pluss::ScalarType::Float64), pluss::series::make({{1.0, 1.0, 1.0}, {1.0, -1.0, 0.0}}));
  out.writeln("constraints =", solver.constraints(), ", variables =", solver.variables());
}
```

Output

```text
constraints = 2 , variables = 3
```

### Example 2

The multipliers have one entry per constraint. After a solve, `result.y` and `result.z` both have `constraints()` entries.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double inf = std::numeric_limits<double>::infinity();
  const qp::Solver solver = qp::prepare(pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::make({{1.0, 1.0}, {1.0, 0.0}, {0.0, 1.0}}));
  const qp::Result result = solver.solve(pluss::series::make({-2.0, -5.0}), pluss::series::make({1.0, 0.0, 0.0}), pluss::series::make({1.0, 0.7, inf}));
  out.writeln("constraints =", solver.constraints(), ", y entries =", result.y.numel(), ", z entries =", result.z.numel());
}
```

Output

```text
constraints = 3 , y entries = 3 , z entries = 3
```

### Example 3

A free problem is posed with `A = I`: as many constraints as variables, all bounds infinite.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const qp::Solver solver = qp::prepare(pluss::series::eye(4, pluss::ScalarType::Float64), pluss::series::eye(4, pluss::ScalarType::Float64));
  out.writeln("variables =", solver.variables(), ", constraints =", solver.constraints());
}
```

Output

```text
variables = 4 , constraints = 4
```

## See Also

- [`variables`](variables.md): the number of variables

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Introduction.html">
    <span class="label">Previous</span>
    <span class="title">Introduction</span>
  </a>
  <a class="next-link" href="dtype.html">
    <span class="label">Next</span>
    <span class="title">dtype</span>
  </a>
</div>
<!-- apiref:nav:end -->
