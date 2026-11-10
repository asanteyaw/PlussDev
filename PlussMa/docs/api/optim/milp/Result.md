<!-- generated from Result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `milp::Result`

What `milp::solve` returns: the best integer solution, its objective, the proven bound and search counters.

`x` and `objective` are `Series` (a vector and a scalar). If no integer solution was found, `objective` is `+inf`. `best_bound` is a proven lower bound on the optimum, so `objective - best_bound` is the remaining gap (zero, to tolerance, for `Optimal`).

## Fields

### `x`

`pluss::Series`, default `zeros`.

The best integer solution found, shape `[n]`; integer coordinates are exact integers.

### `objective`

`pluss::Series`, default `+inf`.

Its objective, a scalar; `+inf` when no solution was found.

### `best_bound`

`double`, default `-inf`.

A proven lower bound on the optimal objective.

### `status`

`milp::Status`, default `Infeasible`.

The verdict.

### `nodes`

`int`, default `0`.

Subproblems processed.

### `lp_rounds`

`int`, default `0`.

Batched relaxation solves performed.

## Guidance

Check `status` before using `x`.

## Practical Examples

### Example 1

An optimal solve: the bound meets the objective.

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
  out.writeln("objective =", result.objective.item<double>(), ", best_bound =", result.best_bound);
  out.writeln("gap       =", result.objective.item<double>() - result.best_bound);
}
```

Output

```text
objective = -23.000000 , best_bound = -23.000000
gap       = 0.000000
```

### Example 2

Counters. `nodes` is the number of subproblems processed and `lp_rounds` the number of batched relaxations solved.

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
  out.writeln("nodes =", result.nodes, ", lp_rounds =", result.lp_rounds);
}
```

Output

```text
nodes = 7 , lp_rounds = 3
```

### Example 3

No solution found. For an infeasible problem the objective is `+inf`.

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
  milp::Problem none;
  none.lp.c = pluss::series::make({1.0});
  none.lp.lower = pluss::series::make({0.2});
  none.lp.upper = pluss::series::make({0.8});
  none.integer = pluss::series::make({1.0});
  const milp::Result result = milp::solve(none);
  out.writeln("status =", milp::to_string(result.status), ", objective =", result.objective.item<double>());
}
```

Output

```text
status = Infeasible , objective = inf
```

## See Also

- [`solve`](solve.md): produces a Result
- [`Status`](Status.md): the verdicts

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Problem.html">
    <span class="label">Previous</span>
    <span class="title">Problem</span>
  </a>
  <a class="next-link" href="Status.html">
    <span class="label">Next</span>
    <span class="title">Status</span>
  </a>
</div>
<!-- apiref:nav:end -->
