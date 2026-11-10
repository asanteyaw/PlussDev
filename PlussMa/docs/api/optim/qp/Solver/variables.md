<!-- generated from variables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d7324d9cda20 -->

# `qp::Solver::variables`

[Back to `qp::Solver`](../Solver.md)

The number of variables `n` of the prepared problem: the length of `x` and of `q`.

## Signature

```cpp
std::int64_t qp::Solver::variables() const noexcept;
```

## What It Does

The number of columns of `A` and the order of `P`: the length of every solution vector and of the linear term `q` you must pass to [`solve`](solve.md). Read it to size inputs for a solver you did not build yourself, or to check that data lines up with a solver before using it.

## Returns

The number of variables, a non-negative integer.

## Practical Examples

### Example 1

A three-variable problem reports 3.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const qp::Solver solver = qp::prepare(pluss::series::eye(3, pluss::ScalarType::Float64), pluss::series::make({{1.0, 1.0, 1.0}}));
  out.writeln("variables =", solver.variables());
}
```

Output

```text
variables = 3
```

### Example 2

Sizing a `q` from the solver, so the same code works for any prepared problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const double inf = std::numeric_limits<double>::infinity();
  const qp::Solver solver = qp::prepare(pluss::series::eye(4, pluss::ScalarType::Float64), pluss::series::eye(4, pluss::ScalarType::Float64));
  const Series q = pluss::series::zeros({solver.variables()}, pluss::ScalarType::Float64) - 1.0;
  const qp::Result result = solver.solve(q, pluss::series::full({solver.constraints()}, -inf, pluss::ScalarType::Float64), pluss::series::full({solver.constraints()}, inf, pluss::ScalarType::Float64));
  out.writeln("x =", result.x.items<double>(), "  (solution of min |x|^2/2 - sum(x) is all ones)");
}
```

Output

```text
x = [1.0000, 1.0000, 1.0000, 1.0000]   (solution of min |x|^2/2 - sum(x) is all ones)
```

### Example 3

`variables()` and `constraints()` together: a budget problem with 5 assets, one equality row and five bound rows has 5 variables and 6 constraints.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series A = pluss::series::make({{1.0, 1.0, 1.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 0.0, 0.0}, {0.0, 0.0, 1.0, 0.0, 0.0}, {0.0, 0.0, 0.0, 1.0, 0.0}, {0.0, 0.0, 0.0, 0.0, 1.0}});
  const qp::Solver solver = qp::prepare(pluss::series::eye(5, pluss::ScalarType::Float64), A);
  out.writeln("variables =", solver.variables(), ", constraints =", solver.constraints());
}
```

Output

```text
variables = 5 , constraints = 6
```

## See Also

- [`constraints`](constraints.md): the number of rows of A

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="solve.html">
    <span class="label">Previous</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
