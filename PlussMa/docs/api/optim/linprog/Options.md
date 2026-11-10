<!-- generated from Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `linprog::Options`

Method, iteration limit, tolerances, polishing and chunking of the linear-programming solver.

`Options` selects between the interior-point method and the simplex, sets how long the interior point may run and how accurate it must be, and controls two reliability features: *polishing* (re-solving uncertified lanes by the exact simplex) and *chunking* (bounding the batched workspace). The defaults are chosen so that a plain `linprog::solve(problem)` is reliable; most users change `method` and perhaps `max_iterations` or `tolerance`.

## Fields

### `method`

`linprog::Method`, default `InteriorPoint`.

`InteriorPoint` (batched, certificates) or `Simplex` (exact vertices, one lane at a time, bases for warm starts).

### `max_iterations`

`int`, default `150`.

The interior-point iteration limit. A lane that has not finished ends `IterationLimit` (and is polished when `polish` is on).

### `tolerance`

`double`, default `1e-9`.

The relative tolerance on the primal and dual residuals and the duality gap for `Optimal`.

### `ray_tolerance`

`double`, default `1e-8`.

The tolerance on the residual of the infeasibility and unboundedness certificates.

### `accept_tolerance`

`double`, default `1e-6`.

A lane that stalls at the limit of accuracy but whose merit is below this ends `NearOptimal` instead of failing.

### `stall_patience`

`int`, default `6`.

How many iterations without halving the merit before a lane is declared stalled.

### `regularization`

`double`, default `1e-12`.

A relative diagonal shift of the normal equations, for numerical stability.

### `primal_regularization`

`double`, default `1e-9`.

A proximal term capping the dynamic range of the normal-equations matrix.

### `refinement_sweeps`

`int`, default `4`.

Iterative-refinement sweeps against the exact Newton system; they keep degenerate vertices from stalling.

### `polish`

`bool`, default `true`.

Re-solve lanes that end `IterationLimit` or `NearOptimal` with the exact simplex and replace them by its verdict. This is what makes problems with almost no interior reliable under the default method.

### `confirm_unbounded`

`bool`, default `true`.

A recession ray alone shows dual infeasibility; with this on, primal feasibility is also confirmed before `Unbounded` is reported.

### `max_lanes`

`std::int64_t`, default `0`.

If positive, solve at most this many lanes per batched call, bounding the `[B, m, m]` workspace. 0 solves all lanes at once.

### `sparse_density`

`double`, default `0.05`.

Assemble the normal equations from the non-zeros when the matrix is sparser than this (and has at least 128 rows).

## Guidance

Leave everything but `method` at its default unless a particular problem needs otherwise: loosen `tolerance` for speed on large sweeps, raise `max_iterations` if lanes end `IterationLimit`, choose `Simplex` for exact vertices or warm starts, set `max_lanes` when the batch makes the `[B, m, m]` workspace too large.

## Practical Examples

### Example 1

`method`. The interior point (default) and the simplex find the same optimum; the simplex reports an exact vertex and an optimal basis, the interior point a point to about `1e-9`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options simplex;
  simplex.method = linprog::Method::Simplex;
  const linprog::Result interior = linprog::solve(problem);
  const linprog::Result vertex = linprog::solve(problem, simplex);
  out.writeln("interior point: x =", interior.x.items<double>());
  out.writeln("simplex       : x =", vertex.x.items<double>(), ", bases returned =", vertex.basis.size());
}
```

Output

```text
interior point: x = [3.0000000000, 1.0000000000]
simplex       : x = [3.0000000000, 1.0000000000] , bases returned = 1
```

### Example 2

`tolerance` and `max_iterations`. A looser relative tolerance finishes in fewer iterations with a slightly less accurate point; a tight iteration cap ends the run with `IterationLimit`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  // minimise -x0 - 2 x1   subject to   x0 + x1 <= 4,  x0 + 3 x1 <= 6,  x >= 0
  linprog::Problem problem;
  problem.c = pluss::series::make({-1.0, -2.0});
  problem.a_ub = pluss::series::make({{1.0, 1.0}, {1.0, 3.0}});
  problem.b_ub = pluss::series::make({4.0, 6.0});
  problem.lower = pluss::series::make({0.0, 0.0});

  linprog::Options loose, capped;
  loose.tolerance = 1e-4;
  capped.max_iterations = 2;
  capped.polish = false;
  const linprog::Result tight = linprog::solve(problem);
  const linprog::Result relaxed = linprog::solve(problem, loose);
  const linprog::Result stopped = linprog::solve(problem, capped);
  out.writeln("default 1e-9 : iterations =", tight.iterations, ", x0 =", tight.x.items<double>()[0]);
  out.writeln("loose   1e-4 : iterations =", relaxed.iterations, ", x0 =", relaxed.x.items<double>()[0]);
  out.writeln("2 iterations : status =", linprog::to_string(static_cast<linprog::Status>(static_cast<int>(stopped.status.item<double>()))));
}
```

Output

```text
default 1e-9 : iterations = 5 , x0 = 3.00000000
loose   1e-4 : iterations = 3 , x0 = 2.99999370
2 iterations : status = IterationLimit
```

### Example 3

`polish` and `max_lanes`. Polishing re-solves lanes the interior point could not certify, by the simplex; `max_lanes` bounds the batched workspace by chunking the lanes. Neither changes a well-behaved answer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace linprog = pluss::subsp::math::linprog;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  linprog::Problem problem;
  problem.c = pluss::series::make({{1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}, {1.0, 2.0, 3.0}});
  problem.a_eq = pluss::series::make({{1.0, 1.0, 1.0}});
  problem.b_eq = pluss::series::make({0.5, 1.0, 1.5}).reshape({3, 1});
  problem.lower = pluss::series::make({0.0, 0.0, 0.0});
  problem.upper = pluss::series::make({0.6, 1.0, 1.0});
  linprog::Options plain;
  plain.polish = false;
  plain.max_lanes = 1;
  out.writeln("defaults               :", linprog::solve(problem).objective.items<double>());
  out.writeln("no polish, one lane/call:", linprog::solve(problem, plain).objective.items<double>());
}
```

Output

```text
defaults               : [0.50000000, 1.40000000, 2.40000000]
no polish, one lane/call: [0.50000000, 1.40000000, 2.40000000]
```

## See Also

- [`solve`](solve.md): consumes Options
- [`Method`](Method.md): the two methods
- [`Status`](Status.md): what `accept_tolerance` and `stall_patience` decide

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Method.html">
    <span class="label">Previous</span>
    <span class="title">Method</span>
  </a>
  <a class="next-link" href="Problem.html">
    <span class="label">Next</span>
    <span class="title">Problem</span>
  </a>
</div>
<!-- apiref:nav:end -->
