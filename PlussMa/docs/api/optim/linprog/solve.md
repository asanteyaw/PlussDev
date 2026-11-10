<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `linprog::solve`

Solves a linear program `minimise cᵀx` subject to equality rows, inequality rows and bounds, for one problem or a batch, and returns the solution with its certificates.

## Signature

```cpp
linprog::Result linprog::solve(const linprog::Problem &problem, const linprog::Options &options = {});
```

## What It Does

`linprog::solve` minimises `cᵀx` subject to `A_eq x = b_eq`, `A_ub x ≤ b_ub` and `lower ≤ x ≤ upper`. Every part but `c` is optional: a [`Problem`](Problem.md) with only `c` and bounds is a bound-constrained problem; infinite bounds (`±inf`) mean no bound, and a variable can be free, bounded below, bounded above, or boxed.

The default method is an interior-point method (the homogeneous self-dual embedding with Mehrotra predictor–corrector steps). Its strength is that it always ends with a *verdict*, `status`, and the evidence for it: for an `Optimal` lane the primal point, the dual objective (equal to the primal one) and the dual values `dual_eq`, `dual_ub`; for `PrimalInfeasible` Farkas multipliers that prove no feasible point exists; for `Unbounded` a recession ray along which the objective decreases forever. The accuracy is that of an interior-point method, about `1e-9` relative on the residuals and the gap, not a vertex-exact answer. Lanes it cannot certify (an objective pinned to its optimum, almost no interior) are re-solved by the exact simplex when `polish` is true, which is the default. Select the simplex itself with `Options::method`; it returns exact vertex solutions, one lane at a time, and an optimal *basis* that can warm-start a related problem (see [`Problem`](Problem.md)).

**Batches.** The constraint matrices are shared by all lanes. `c`, `b_eq`, `b_ub` and the bound *values* may carry leading axes, one lane per index, and all lanes are solved together in the same batched operations, so a parameter sweep, the objectives of a flux-variability analysis, or every open node of a branch-and-bound level is one call. The pattern of which bounds are finite must be the same in every lane. When the batch is large, `max_lanes` bounds the workspace by solving it in chunks.

## Parameters

`const linprog::Problem &problem`
: The cost, constraint rows, bounds and optional simplex warm start. See `Problem`.

`const linprog::Options &options`
: Method, iteration limit, tolerances, polishing and chunking. See `Options`.

## Returns

A `linprog::Result`: for each lane the solution `x`, `objective`, `dual_objective`, `status`, the duals `dual_eq` and `dual_ub`, a recession `ray` and `farkas` multipliers, plus the iteration count and (for the simplex) the optimal bases.

## Errors

- `linprog: c must be a [..., n] Series` when `c` is missing.

## Practical Examples

### Example 1

A first problem and how to read the answer. Maximise `x₀ + 2x₁` (minimise its negative) under `x₀ + x₁ ≤ 4` and `x₀ + 3x₁ ≤ 6`: the optimum is the vertex `(3, 1)` with objective `−5`. The dual objective equals the primal one (the certificate of optimality), and `dual_ub` gives the price of each inequality: relaxing either right-hand side by one unit improves the objective by `0.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  const linprog::Result result = linprog::solve(problem);

  out.writeln("x              =", result.x.items<double>());
  out.writeln("objective      =", result.objective.item<double>());
  out.writeln("dual objective =", result.dual_objective.item<double>());
  out.writeln("dual_ub        =", result.dual_ub.items<double>());
  out.writeln("status         =", linprog::to_string(static_cast<linprog::Status>(static_cast<int>(result.status.item<double>()))), ", iterations =", result.iterations);
}
```

Output

```text
x              = [3.000000, 1.000000]
objective      = -5.000000
dual objective = -5.000000
dual_ub        = [-0.500000, -0.500000]
status         = Optimal , iterations = 5
```

### Example 2

Equality rows, bounds and the dual of an equality. Minimise `x₀ + 2x₁ + 3x₂` with `x₀ + x₁ + x₂ = 1` and `x₀ ≤ 0.6`, all variables non-negative. The cheap variable is capped, so the remainder goes to the next cheapest: `x = (0.6, 0.4, 0)`. The equality's dual value, `d objective / d b_eq`, is `2`: one more unit of right-hand side would be bought at the price of `x₁`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  linprog::Problem problem;
  problem.c = pluss::series::make({1.0, 2.0, 3.0});
  problem.a_eq = pluss::series::make({{1.0, 1.0, 1.0}});
  problem.b_eq = pluss::series::make({1.0});
  problem.lower = pluss::series::make({0.0, 0.0, 0.0});
  problem.upper = pluss::series::make({0.6, 1.0, 1.0});
  const linprog::Result result = linprog::solve(problem);

  out.writeln("x       =", result.x.items<double>());
  out.writeln("dual_eq =", result.dual_eq.items<double>());
}
```

Output

```text
x       = [0.600000, 0.400000, 0.000000]
dual_eq = [2.000000]
```

### Example 3

A batch of objectives sharing one constraint set. Three cost vectors give three optima in one call; `x` has shape `[3, 2]`, `objective` and `status` have one entry per lane.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  problem.c = pluss::series::make({{-1.0, -2.0}, {-3.0, -1.0}, {1.0, 1.0}});
  const linprog::Result result = linprog::solve(problem);

  out.writeln("x shape   =", result.x.sizes());
  out.writeln("x         =", result.x.reshape({6}).items<double>());
  out.writeln("objective =", result.objective.items<double>());
  out.writeln("status    =", result.status.items<double>(), " (0 = Optimal)");
}
```

Output

```text
x shape   = [3, 2]
x         = [3.000000, 1.000000, 4.000000, 0.000000, 0.000000, 0.000000]
objective = [-5.000000, -12.000000, 0.000000]
status    = [0.000000, 0.000000, 0.000000]  (0 = Optimal)
```

### Example 4

A batch of right-hand sides. When `b_eq` varies per lane, `c` must carry the batch axis too (here repeated), and `dual_eq` has shape `[lanes, rows]`. The objective as a function of `b_eq` is piecewise linear; its slope is the dual.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  linprog::Problem problem;
  problem.c = pluss::series::make({{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}});
  problem.a_eq = pluss::series::make({{1.0, 1.0, 1.0}});
  problem.b_eq = pluss::series::make({0.5, 1.0, 1.5}).reshape({3, 1});
  problem.lower = pluss::series::make({0.0, 0.0, 0.0});
  problem.upper = pluss::series::make({0.6, 1.0, 1.0});
  const linprog::Result result = linprog::solve(problem);

  out.writeln("b_eq      =", pluss::make_series({0.5, 1.0, 1.5}).items<double>());
  out.writeln("objective =", result.objective.items<double>());
  out.writeln("dual_eq shape =", result.dual_eq.sizes());
}
```

Output

```text
b_eq      = [0.500000, 1.000000, 1.500000]
objective = [0.500000, 1.400000, 2.400000]
dual_eq shape = [3, 1]
```

### Example 5

Certificates of failure. `x₀ + x₁ ≤ 1` together with `x₀ + x₁ ≥ 3` cannot hold: status `PrimalInfeasible` and Farkas multipliers. `min −x₀` with `x₀ − x₁ ≤ 1` is unbounded: status `Unbounded` and a recession ray `d` (the objective decreases along it forever).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  linprog::Problem infeasible;
  infeasible.c = pluss::series::make({1.0, 1.0});
  infeasible.a_ub = pluss::series::make({{1.0, 1.0}, {-1.0, -1.0}});
  infeasible.b_ub = pluss::series::make({1.0, -3.0});
  infeasible.lower = pluss::series::make({0.0, 0.0});
  const linprog::Result i = linprog::solve(infeasible);
  out.writeln("infeasible: status =", linprog::to_string(static_cast<linprog::Status>(static_cast<int>(i.status.item<double>()))), ", farkas =", i.farkas.items<double>());

  linprog::Problem unbounded;
  unbounded.c = pluss::series::make({-1.0, 0.0});
  unbounded.a_ub = pluss::series::make({{1.0, -1.0}});
  unbounded.b_ub = pluss::series::make({1.0});
  unbounded.lower = pluss::series::make({0.0, 0.0});
  const linprog::Result u = linprog::solve(unbounded);
  out.writeln("unbounded : status =", linprog::to_string(static_cast<linprog::Status>(static_cast<int>(u.status.item<double>()))), ", ray =", u.ray.items<double>());
}
```

Output

```text
infeasible: status = PrimalInfeasible , farkas = [-1.6478, -0.8522]
unbounded : status = Unbounded , ray = [0.8672, 1.5664]
```

### Example 6

The exact simplex instead of the interior point. It returns a vertex, plus the optimal basis, which can be fed to a related problem as a warm start (see `Problem`); here a changed cost reuses the basis and needs no iterations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options simplex;
  simplex.method = linprog::Method::Simplex;
  const linprog::Result first = linprog::solve(problem, simplex);

  linprog::Problem changed = problem;
  changed.c = pluss::series::make({-1.0, -2.5});
  changed.warm_basis = first.basis[0];
  changed.warm_at_upper = first.at_upper[0];
  const linprog::Result second = linprog::solve(changed, simplex);

  out.writeln("first : x =", first.x.items<double>(), ", iterations =", first.iterations);
  out.writeln("second: x =", second.x.items<double>(), ", iterations =", second.iterations, ", warm_started =", static_cast<int>(second.warm_started[0]));
}
```

Output

```text
first : x = [3.000000, 1.000000] , iterations = 4
second: x = [3.000000, 1.000000] , iterations = 0 , warm_started = 1
```

### Example 7

Bounding the workspace. `max_lanes = 2` solves a batch of three lanes in chunks of two; the answers are identical to the unchunked call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  linprog::Problem problem;
  problem.c = pluss::series::make({{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}});
  problem.a_eq = pluss::series::make({{1.0, 1.0, 1.0}});
  problem.b_eq = pluss::series::make({0.5, 1.0, 1.5}).reshape({3, 1});
  problem.lower = pluss::series::make({0.0, 0.0, 0.0});
  problem.upper = pluss::series::make({0.6, 1.0, 1.0});
  linprog::Options chunked;
  chunked.max_lanes = 2;
  out.writeln("all lanes at once :", linprog::solve(problem).objective.items<double>());
  out.writeln("chunks of two     :", linprog::solve(problem, chunked).objective.items<double>());
}
```

Output

```text
all lanes at once : [0.500000, 1.400000, 2.400000]
chunks of two     : [0.500000, 1.400000, 2.400000]
```

### Example 8

Stopping short. With `max_iterations = 2` and no polishing, the solver cannot finish and reports `IterationLimit`; always check `status`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options short_run;
  short_run.max_iterations = 2;
  short_run.polish = false;
  const linprog::Result result = linprog::solve(problem, short_run);
  out.writeln("status =", linprog::to_string(static_cast<linprog::Status>(static_cast<int>(result.status.item<double>()))));
}
```

Output

```text
status = IterationLimit
```

### Example 9

A problem with no `c` is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  try {
    (void)linprog::solve(linprog::Problem{});
  } catch (const std::exception &e) {
    out.writeln("error =", first_line(e));
  }
}
```

Output

```text
error = linprog: c must be a [..., n] Series
```

## See Also

- [`Problem`](Problem.md): the inputs
- [`Options`](Options.md): method and tolerances
- [`Result`](Result.md): reading the outputs
- [`Status`](Status.md): the verdicts
- [`milp::solve`](../milp/solve.md): adds integrality

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Status.html">
    <span class="label">Previous</span>
    <span class="title">Status</span>
  </a>
  <a class="next-link" href="success.html">
    <span class="label">Next</span>
    <span class="title">success</span>
  </a>
</div>
<!-- apiref:nav:end -->
