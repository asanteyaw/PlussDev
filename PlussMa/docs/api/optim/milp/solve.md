<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `milp::solve`

Solves a mixed-integer linear program by branch-and-bound and returns the best solution found with a proven bound.

## Signature

```cpp
milp::Result milp::solve(const milp::Problem &problem, const milp::Options &options = {});
```

## What It Does

`milp::solve` minimises `cᵀx` subject to the same rows and bounds as [`linprog::solve`](../linprog/solve.md), with the extra requirement that the variables flagged in `Problem::integer` take integer values. A 0/1 variable is an integer variable bounded by `[0, 1]`; every integer variable must have finite lower and upper bounds.

The search is branch-and-bound on the batched linear-programming solver. The open nodes of the search tree are *lanes of one batched call*: each round takes up to `batch` nodes (best bound first), solves all their relaxations together, rounds the relaxations to find incumbents early, and splits the fractional ones. A node is discarded when its bound cannot beat the incumbent by more than `gap_tolerance`. When the queue empties the incumbent is optimal; when `max_nodes` is reached first, the incumbent (if any) and the proven `best_bound` are returned with status `NodeLimit`, so the remaining gap is known. Returned integer coordinates are rounded to exact integers.

## Parameters

`const milp::Problem &problem`
: The linear program and the integrality flags. See `Problem`.

`const milp::Options &options`
: Node limit, batch size, tolerances and the linear-programming options. See `Options`.

## Returns

A `milp::Result` with the solution `x`, `objective`, the proven `best_bound`, the `status` and counters.

## Errors

- `milp: integer variables must have finite lower and upper bounds`.
- `milp: c must be a 1-D Series` when `c` is not a vector.
- `milp: integer must have one entry per variable`.

## Practical Examples

### Example 1

A knapsack. Four items with values `10, 13, 7, 8` and weights `3, 4, 2, 3`, capacity `7`. The best choice is items `a` and `b` (value 23, weight 7). The proven bound equals the objective at optimality.

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
  // maximise 10a + 13b + 7c + 8d   subject to   3a + 4b + 2c + 3d <= 7,  each item taken or not
  milp::Problem problem;
  problem.lp.c = pluss::series::make({-10.0, -13.0, -7.0, -8.0});
  problem.lp.a_ub = pluss::series::make({{3.0, 4.0, 2.0, 3.0}});
  problem.lp.b_ub = pluss::series::make({7.0});
  problem.lp.lower = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  problem.lp.upper = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  problem.integer = pluss::series::make({1.0, 1.0, 1.0, 1.0});

  const milp::Result result = milp::solve(problem);

  out.writeln("x          =", result.x.items<double>());
  out.writeln("objective  =", result.objective.item<double>());
  out.writeln("best bound =", result.best_bound);
  out.writeln("status     =", milp::to_string(result.status), ", nodes =", result.nodes);
}
```

Output

```text
x          = [1.000000, 1.000000, 0.000000, 0.000000]
objective  = -23.000000
best bound = -23.000000
status     = Optimal , nodes = 7
```

### Example 2

Integrality has a price. Solving the same knapsack as a plain linear program lets item b be taken half-way (value 23.5); the integer answer is worse (23).

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
  // maximise 10a + 13b + 7c + 8d   subject to   3a + 4b + 2c + 3d <= 7,  each item taken or not
  milp::Problem problem;
  problem.lp.c = pluss::series::make({-10.0, -13.0, -7.0, -8.0});
  problem.lp.a_ub = pluss::series::make({{3.0, 4.0, 2.0, 3.0}});
  problem.lp.b_ub = pluss::series::make({7.0});
  problem.lp.lower = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  problem.lp.upper = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  problem.integer = pluss::series::make({1.0, 1.0, 1.0, 1.0});

  const linprog::Result relaxed = linprog::solve(problem.lp);
  const milp::Result integral = milp::solve(problem);
  out.writeln("relaxation objective =", relaxed.objective.item<double>(), ", x =", relaxed.x.items<double>());
  out.writeln("integer objective    =", integral.objective.item<double>(), ", x =", integral.x.items<double>());
}
```

Output

```text
relaxation objective = -23.500000 , x = [1.000000, 0.500000, 1.000000, 0.000000]
integer objective    = -23.000000 , x = [1.000000, 1.000000, 0.000000, 0.000000]
```

### Example 3

Mixing integer and continuous variables. `x₀` is integer, `x₁` continuous: minimise `−3x₀ − x₁` with `2x₀ + x₁ ≤ 5.5` and `x₀ ≤ 2.7`. The relaxation would take `x₀ = 2.7`; the integer variable is rounded down to `2` and the continuous one takes up the slack, `1.5`.

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
  problem.lp.c = pluss::series::make({-3.0, -1.0});
  problem.lp.a_ub = pluss::series::make({{2.0, 1.0}});
  problem.lp.b_ub = pluss::series::make({5.5});
  problem.lp.lower = pluss::series::make({0.0, 0.0});
  problem.lp.upper = pluss::series::make({2.7, 100.0});
  problem.integer = pluss::series::make({1.0, 0.0});
  const milp::Result result = milp::solve(problem);
  out.writeln("x = ", result.x.items<double>(), ", status =", milp::to_string(result.status));
}
```

Output

```text
x =  [2.000000, 1.500000] , status = Optimal
```

### Example 4

Stopping early. With `max_nodes = 1` the search ends after the root: the status is `NodeLimit`, and `best_bound` still says how good a solution could possibly be, which is the guarantee you carry away.

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
  // maximise 10a + 13b + 7c + 8d   subject to   3a + 4b + 2c + 3d <= 7,  each item taken or not
  milp::Problem problem;
  problem.lp.c = pluss::series::make({-10.0, -13.0, -7.0, -8.0});
  problem.lp.a_ub = pluss::series::make({{3.0, 4.0, 2.0, 3.0}});
  problem.lp.b_ub = pluss::series::make({7.0});
  problem.lp.lower = pluss::series::make({0.0, 0.0, 0.0, 0.0});
  problem.lp.upper = pluss::series::make({1.0, 1.0, 1.0, 1.0});
  problem.integer = pluss::series::make({1.0, 1.0, 1.0, 1.0});

  milp::Options options;
  options.max_nodes = 1;
  const milp::Result result = milp::solve(problem, options);
  out.writeln("status =", milp::to_string(result.status), ", nodes =", result.nodes);
  out.writeln("best bound =", result.best_bound, ", objective =", result.objective.item<double>());
}
```

Output

```text
status = NodeLimit , nodes = 1
best bound = -23.500000 , objective = inf
```

### Example 5

Infeasible problems and bad input. No integer lies in `[0.2, 0.8]`: the status is `Infeasible`. An integer variable with an infinite bound is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace milp = pluss::subsp::math::milp;
namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };
  milp::Problem none;
  none.lp.c = pluss::series::make({1.0});
  none.lp.lower = pluss::series::make({0.2});
  none.lp.upper = pluss::series::make({0.8});
  none.integer = pluss::series::make({1.0});
  out.writeln("status =", milp::to_string(milp::solve(none).status));

  milp::Problem open_bound;
  open_bound.lp.c = pluss::series::make({1.0});
  open_bound.lp.lower = pluss::series::make({0.0});
  open_bound.integer = pluss::series::make({1.0});
  try {
    (void)milp::solve(open_bound);
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
}
```

Output

```text
status = Infeasible
error = milp: integer variables must have finite lower and upper bounds
```

## See Also

- [`Problem`](Problem.md): the inputs
- [`Options`](Options.md): limits and tolerances
- [`Result`](Result.md): the outputs
- [`Status`](Status.md): the verdicts
- [`linprog::solve`](../linprog/solve.md): the relaxation solver

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Status.html">
    <span class="label">Previous</span>
    <span class="title">Status</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
<!-- apiref:nav:end -->
