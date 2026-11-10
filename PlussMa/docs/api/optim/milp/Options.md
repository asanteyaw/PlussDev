<!-- generated from Options.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `milp::Options`

Node limit, batch size and tolerances of the branch-and-bound search.

`Options` bounds the search (`max_nodes`), sets how many relaxations are solved together (`batch`), sets the tolerances for accepting a relaxation as integral and for declaring the incumbent optimal, and carries the [`linprog::Options`](../linprog/Options.md) used for every relaxation.

## Fields

### `lp`

`linprog::Options`, default `{}`.

Options for each relaxation: interior point (default) or simplex, tolerances, polishing.

### `max_nodes`

`int`, default `200000`.

The most nodes to process. On reaching it the result has status `NodeLimit`.

### `batch`

`int`, default `64`.

How many open nodes are solved per round, as lanes of one batched linear-programming call.

### `integer_tolerance`

`double`, default `1e-6`.

A relaxed value within this distance of an integer counts as integral.

### `gap_tolerance`

`double`, default `1e-7`.

A node is pruned, and the incumbent declared optimal, when the bound gap is at most this absolute amount or this fraction of `1 + |incumbent|`.

## Guidance

The defaults are for exact optimality of moderate problems; lower `max_nodes` to bound the effort and read `best_bound` for the guarantee.

## Practical Examples

### Example 1

`max_nodes`. The search is cut at one node; the bound is still valid.

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
}
```

Output

```text
status = NodeLimit , nodes = 1
```

### Example 2

`batch`. Fewer nodes per round means more rounds; the answer is the same.

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

  milp::Options narrow;
  narrow.batch = 1;
  const milp::Result a = milp::solve(problem);
  const milp::Result b = milp::solve(problem, narrow);
  out.writeln("default batch: objective =", a.objective.item<double>(), ", rounds =", a.lp_rounds);
  out.writeln("batch = 1    : objective =", b.objective.item<double>(), ", rounds =", b.lp_rounds);
}
```

Output

```text
default batch: objective = -23.000000 , rounds = 3
batch = 1    : objective = -23.000000 , rounds = 5
```

### Example 3

`lp`. The relaxations can be solved by the simplex instead of the interior point; each child node then warm-starts from its parent's optimal basis.

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
  options.lp.method = linprog::Method::Simplex;
  const milp::Result result = milp::solve(problem, options);
  out.writeln("objective =", result.objective.item<double>(), ", x =", result.x.items<double>());
}
```

Output

```text
objective = -23.000000 , x = [1.000000, 1.000000, 0.000000, 0.000000]
```

## See Also

- [`solve`](solve.md): consumes Options
- [`linprog::Options`](../linprog/Options.md): the `lp` member

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Problem.html">
    <span class="label">Next</span>
    <span class="title">Problem</span>
  </a>
</div>
<!-- apiref:nav:end -->
