<!-- generated from prepare.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `qp::prepare`

Checks and factorises the fixed part of a quadratic program once, and returns a `Solver` that solves it repeatedly.

## Signature

```cpp
qp::Solver qp::prepare(const pluss::Series &p, const pluss::Series &a, qp::Options options = {});
```

## What It Does

Everything in a QP that depends only on `P` and `A` can be done once: validating `P` as positive semidefinite, checking the dimensions, and factorising the linear system the iteration solves. `prepare` does exactly that and returns a [`Solver`](Solver.md) holding the results. When only `q` or the bounds change between problems (and the dimensions and the quadratic terms do not), calling the solver again skips all of that setup, which is the dominant cost for small and medium problems.

The options are fixed at preparation: the returned solver uses them for every solve. `prepare` refuses an indefinite `P` immediately, rather than at the first solve.

## Parameters

`const pluss::Series &p`
: The quadratic matrix `P`, symmetric positive semidefinite.

`const pluss::Series &a`
: The constraint matrix `A`.

`qp::Options options`
: The options used by every solve of the returned solver.

## Returns

A `qp::Solver` for this `P` and `A`.

## Errors

- `P must be positive semidefinite`.
- A shape mismatch between `P` and `A` is refused.

## Practical Examples

### Example 1

Prepare once, solve for several linear terms. The same `P` and `A`, three different `q`; each solve needs no new factorisation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>
#include <vector>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double inf = std::numeric_limits<double>::infinity();
  const Series P = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
  const Series A = pluss::series::eye(2, pluss::ScalarType::Float64);
  const Series free_below = pluss::series::make({-inf, -inf}), free_above = pluss::series::make({inf, inf});

  const qp::Solver solver = qp::prepare(P, A);
  for (const std::vector<double> &q : {std::vector<double>{1.0, 1.0}, {-2.0, -1.0}, {0.0, 3.0}}) {
    const qp::Result result = solver.solve(pluss::make_series(q, {2}), free_below, free_above);
    out.writeln("q =", q, ": x =", result.x.items<double>());
  }
}
```

Output

```text
q = [1.00000, 1.00000] : x = [-0.14286, -0.42857]
q = [-2.00000, -1.00000] : x = [0.42857, 0.28571]
q = [0.00000, 3.00000] : x = [0.42857, -1.71429]
```

### Example 2

The solver describes the problem it was built for: `variables()` is the length of `x`, `constraints()` the number of rows of `A`, and `dtype()` and `platform()` the arithmetic it works in.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const Series P = pluss::series::make({{2.0, 0.0, 0.0}, {0.0, 2.0, 0.0}, {0.0, 0.0, 2.0}});
  const Series A = pluss::series::make({{1.0, 1.0, 1.0}, {1.0, 0.0, 0.0}});
  const qp::Solver solver = qp::prepare(P, A);

  out.writeln("variables   =", solver.variables());
  out.writeln("constraints =", solver.constraints());
  out.writeln("dtype is Float64 =", solver.dtype() == pluss::ScalarType::Float64);
}
```

Output

```text
variables   = 3
constraints = 2
dtype is Float64 = true
```

### Example 3

Options are fixed at preparation. A solver prepared with tight tolerances uses them for every solve, so the same problem takes more iterations than with a solver prepared with the defaults.

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
  const Series P = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
  const Series A = pluss::series::eye(2, pluss::ScalarType::Float64);
  const Series q = pluss::series::make({1.0, 1.0}), free_below = pluss::series::make({-inf, -inf}), free_above = pluss::series::make({inf, inf});
  qp::Options tight;
  tight.abs_tol = 1e-10;
  tight.rel_tol = 1e-10;

  out.writeln("defaults : iterations =", qp::prepare(P, A).solve(q, free_below, free_above).iterations);
  out.writeln("tight    : iterations =", qp::prepare(P, A, tight).solve(q, free_below, free_above).iterations);
}
```

Output

```text
defaults : iterations = 20
tight    : iterations = 40
```

### Example 4

An indefinite `P` is refused at preparation, before any `q` exists.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try {
    (void)qp::prepare(pluss::series::make({{1.0, 2.0}, {2.0, -1.0}}), pluss::series::eye(2, pluss::ScalarType::Float64));
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
}
```

Output

```text
error = math::qp::solve: P must be positive semidefinite
```

## See Also

- [`Solver`](Solver.md): the object returned
- [`solve`](solve.md): the one-shot form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="WarmStart.html">
    <span class="label">Previous</span>
    <span class="title">WarmStart</span>
  </a>
  <a class="next-link" href="solve.html">
    <span class="label">Next</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
