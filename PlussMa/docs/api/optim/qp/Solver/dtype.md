<!-- generated from dtype.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a60c7f41dae0 -->

# `qp::Solver::dtype`

[Back to `qp::Solver`](../Solver.md)

The floating-point type the prepared problem is solved in.

## Signature

```cpp
pluss::ScalarType qp::Solver::dtype() const noexcept;
```

## What It Does

The solver works in the dtype of the matrices it was prepared with, and results come back in it. Read it to know the precision you are getting, or to build compatible inputs for later solves. `Float64` is the usual choice; lower precisions limit the accuracy the tolerances can reach.

## Returns

A `pluss::ScalarType`.

## Practical Examples

### Example 1

Matrices built in `Float64` give a `Float64` solver.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const qp::Solver solver = qp::prepare(pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::eye(2, pluss::ScalarType::Float64));
  out.writeln("Float64 =", solver.dtype() == pluss::ScalarType::Float64);
}
```

Output

```text
Float64 = true
```

### Example 2

Building the next problem's data in the solver's dtype.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const qp::Solver solver = qp::prepare(pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::eye(2, pluss::ScalarType::Float64));
  const Series q = pluss::make_series({1.0, -1.0}).to(solver.dtype());
  out.writeln("q has the solver's dtype =", q.scalar_type() == solver.dtype());
}
```

Output

```text
q has the solver's dtype = true
```

### Example 3

A solution is in the solver's dtype.

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
  const qp::Solver solver = qp::prepare(pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::eye(2, pluss::ScalarType::Float64));
  const qp::Result result = solver.solve(pluss::series::make({1.0, 1.0}), pluss::series::make({-inf, -inf}), pluss::series::make({inf, inf}));
  out.writeln("solution dtype matches =", result.x.scalar_type() == solver.dtype());
}
```

Output

```text
solution dtype matches = true
```

## See Also

- [`platform`](platform.md): where it runs

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="constraints.html">
    <span class="label">Previous</span>
    <span class="title">constraints</span>
  </a>
  <a class="next-link" href="platform.html">
    <span class="label">Next</span>
    <span class="title">platform</span>
  </a>
</div>
<!-- apiref:nav:end -->
