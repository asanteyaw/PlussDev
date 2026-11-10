<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7a2972c43915 -->

# `qp::Solver::solve`

[Back to `qp::Solver`](../Solver.md)

Solves the prepared problem for a new linear term and bounds.

## Signature

```cpp
qp::Result qp::Solver::solve(const pluss::Series &q, const pluss::Series &lower, const pluss::Series &upper, qp::WarmStart warm = {}) const;
```

## What It Does

This is the repeated step of a prepared problem: given a new `q` and new bounds, it runs the iteration using the factorisation stored in the solver, with no setup. Everything else (`P`, `A`, the options) is fixed. The result is the same [`qp::Result`](../Result.md) as the one-shot [`qp::solve`](../solve.md) returns, and a [`WarmStart`](../WarmStart.md) can be passed to begin from a previous solution, which is the typical use in model-predictive control: each step starts from the last one.

`q` may carry leading batch dimensions, solving a batch of problems that share the prepared matrices. The call does not change the solver, so it is safe to call repeatedly.

## Parameters

`const pluss::Series &q`
: The linear term, ending in the number of variables; may carry leading batch dimensions.

`const pluss::Series &lower`
: The lower bounds on `A x`.

`const pluss::Series &upper`
: The upper bounds on `A x`.

`qp::WarmStart warm`
: An optional starting point, usually `previous.warm()`.

## Returns

A `qp::Result`.

## Errors

- `q must end in <n>`.
- `lower must not exceed upper`.

## Practical Examples

### Example 1

A new `q` each time, with the factorisation reused. The solver is prepared for `P = [[4, 1], [1, 2]]` and no constraints; the unconstrained minimiser is `−P⁻¹q`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double inf = std::numeric_limits<double>::infinity();
  const Series P = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
  const qp::Solver solver = qp::prepare(P, pluss::series::eye(2, pluss::ScalarType::Float64));
  const Series free_below = pluss::series::make({-inf, -inf}), free_above = pluss::series::make({inf, inf});

  out.writeln("q = (1, 1)   : x =", solver.solve(pluss::series::make({1.0, 1.0}), free_below, free_above).x.items<double>(), "  (-1/7, -3/7)");
  out.writeln("q = (-2, -1) : x =", solver.solve(pluss::series::make({-2.0, -1.0}), free_below, free_above).x.items<double>(), "  (3/7, 2/7)");
}
```

Output

```text
q = (1, 1)   : x = [-0.14286, -0.42857]   (-1/7, -3/7)
q = (-2, -1) : x = [0.42857, 0.28571]   (3/7, 2/7)
```

### Example 2

New bounds with the same matrices; only `lower` and `upper` change. The free minimiser is already non-negative, so requiring `x ≥ 0` changes nothing; capping `x₀ ≤ 0.2` does.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace qp = pluss::subsp::math::qp;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double inf = std::numeric_limits<double>::infinity();
  const Series P = pluss::series::make({{4.0, 1.0}, {1.0, 2.0}});
  const qp::Solver solver = qp::prepare(P, pluss::series::eye(2, pluss::ScalarType::Float64));
  const Series q = pluss::series::make({-2.0, -1.0});

  const qp::Result free = solver.solve(q, pluss::series::make({-inf, -inf}), pluss::series::make({inf, inf}));
  const qp::Result nonnegative = solver.solve(q, pluss::series::make({0.0, 0.0}), pluss::series::make({inf, inf}));
  const qp::Result capped = solver.solve(q, pluss::series::make({0.0, 0.0}), pluss::series::make({0.2, inf}));
  out.writeln("free              : x =", free.x.items<double>());
  out.writeln("x >= 0            : x =", nonnegative.x.items<double>());
  out.writeln("0 <= x0 <= 0.2    : x =", capped.x.items<double>());
}
```

Output

```text
free              : x = [0.42857, 0.28571]
x >= 0            : x = [0.42857, 0.28571]
0 <= x0 <= 0.2    : x = [0.20000, 0.40000]
```

### Example 3

A receding sequence with warm starts. Each step solves a slightly different problem starting from the previous solution; the iteration counts are lower than the cold-start count for the same problems.

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
  const qp::Solver solver = qp::prepare(P, pluss::series::eye(2, pluss::ScalarType::Float64));
  const Series lower = pluss::series::make({0.0, 0.0}), upper = pluss::series::make({inf, inf});

  qp::WarmStart warm;
  for (int step = 0; step < 4; ++step) {
    const Series q = pluss::series::make({-1.0 - 0.01 * step, -2.0});
    const qp::Result warm_result = solver.solve(q, lower, upper, warm);
    const qp::Result cold_result = solver.solve(q, lower, upper);
    out.writeln("step", step, ": warm iterations =", warm_result.iterations, ", cold =", cold_result.iterations);
    warm = warm_result.warm();
  }
}
```

Output

```text
step 0 : warm iterations = 30 , cold = 30
step 1 : warm iterations = 10 , cold = 20
step 2 : warm iterations = 10 , cold = 20
step 3 : warm iterations = 10 , cold = 20
```

### Example 4

A shape that does not match the prepared problem is refused.

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

  const qp::Solver solver = qp::prepare(pluss::series::eye(2, pluss::ScalarType::Float64), pluss::series::eye(2, pluss::ScalarType::Float64));
  try {
    (void)solver.solve(pluss::series::make({1.0, 2.0, 3.0}), pluss::series::make({0.0, 0.0}), pluss::series::make({1.0, 1.0}));
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
}
```

Output

```text
error = math::qp::solve: q must end in 2
```

## See Also

- [`qp::solve`](../solve.md): the one-shot form
- [`variables`](variables.md): the expected length of q

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="platform.html">
    <span class="label">Previous</span>
    <span class="title">platform</span>
  </a>
  <a class="next-link" href="variables.html">
    <span class="label">Next</span>
    <span class="title">variables</span>
  </a>
</div>
<!-- apiref:nav:end -->
