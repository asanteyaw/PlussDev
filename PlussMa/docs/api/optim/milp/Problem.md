<!-- generated from Problem.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `milp::Problem`

A mixed-integer linear program: a linear program plus the flags of which variables are integer.

`Problem` holds a [`linprog::Problem`](../linprog/Problem.md) in `lp` and a vector `integer` marking integer variables with a non-zero entry. Unlike the linear-programming solver, a mixed-integer problem is a single problem: `c` and the bounds are plain length-`n` vectors, not batches. Every integer variable needs finite lower and upper bounds, which are tightened to the integers inside them.

## Fields

### `lp`

`linprog::Problem`, default `empty`.

The cost vector `c` (length `n`), the optional equality and inequality rows, and the bounds, as for `linprog::solve`, but a single problem (no batch axes). Required bounds on integer variables must be finite.

### `integer`

`pluss::Series`, default `undefined`.

A length-`n` vector; a non-zero entry marks the variable as integer.

## Guidance

`integer` must have exactly one entry per variable.

## Practical Examples

### Example 1

Integer flags. `integer = {1, 0}` makes `x₀` integer and `x₁` continuous; the relaxation of `max x₀ + x₁, x₀ + x₁ ≤ 2.5` would split the 2.5 anywhere, the integer problem forces `x₀` to a whole number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace milp = pluss::subsp::math::milp;
namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  milp::Problem problem;
  problem.lp.c = pluss::series::make({-1.0, -1.0});
  problem.lp.a_ub = pluss::series::make({{1.0, 1.0}});
  problem.lp.b_ub = pluss::series::make({2.5});
  problem.lp.lower = pluss::series::make({0.0, 0.0});
  problem.lp.upper = pluss::series::make({5.0, 5.0});
  problem.integer = pluss::series::make({1.0, 0.0});
  const milp::Result result = milp::solve(problem);
  out.writeln("objective =", result.objective.item<double>());
  out.writeln("x0 is an integer:", std::abs(result.x.items<double>()[0] - std::round(result.x.items<double>()[0])) < 1e-12);
}
```

Output

```text
objective = -2.500000
x0 is an integer: true
```

### Example 2

Equality rows. Choose exactly two of four items (`Σ x = 2`) to minimise the cost `4, 1, 3, 2`: items 2 and 4, cost 3.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace milp = pluss::subsp::math::milp;
namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  milp::Problem problem;
  problem.lp.c = pluss::series::make({4.0, 1.0, 3.0, 2.0});
  problem.lp.a_eq = pluss::series::make({{1.0, 1.0, 1.0, 1.0}});
  problem.lp.b_eq = pluss::series::make({2.0});
  problem.lp.lower = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  problem.lp.upper = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  problem.integer = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  const milp::Result result = milp::solve(problem);
  out.writeln("x =", result.x.items<double>(), ", cost =", result.objective.item<double>());
}
```

Output

```text
x = [0.000000, 1.000000, 0.000000, 1.000000] , cost = 3.000000
```

### Example 3

A general integer variable, not only 0/1. Minimise `−x` with `3x ≤ 10`, `x ∈ {0,…,10}`: the answer is `3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace milp = pluss::subsp::math::milp;
namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  milp::Problem problem;
  problem.lp.c = pluss::series::make({-1.0});
  problem.lp.a_ub = pluss::series::make({3.0}).reshape({1, 1});
  problem.lp.b_ub = pluss::series::make({10.0});
  problem.lp.lower = pluss::series::make({0.0});
  problem.lp.upper = pluss::series::make({10.0});
  problem.integer = pluss::series::make({1.0});
  out.writeln("x =", milp::solve(problem).x.items<double>());
}
```

Output

```text
x = [3.000000]
```

## See Also

- [`solve`](solve.md): consumes a Problem
- [`linprog::Problem`](../linprog/Problem.md): the underlying linear program

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Options.html">
    <span class="label">Previous</span>
    <span class="title">Options</span>
  </a>
  <a class="next-link" href="Result.html">
    <span class="label">Next</span>
    <span class="title">Result</span>
  </a>
</div>
<!-- apiref:nav:end -->
