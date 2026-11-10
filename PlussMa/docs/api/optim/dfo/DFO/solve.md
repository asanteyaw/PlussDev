<!-- generated from solve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15135cf006a4 -->

# `DFO::solve`

[Back to `dfo::DFO`](../DFO.md)

Solves a problem in one call, without keeping an object.

## Signature

```cpp
static dfo::DfoResult dfo::DFO::solve(const dfo::DfoProblem &problem, const dfo::DfoOptions &options = {});
```

## What It Does

Equivalent to `DFO(options).run(problem)`. This is the usual way to call the front door when the problem is a filled-in [`DfoProblem`](../DfoProblem.md).

## Parameters

`const dfo::DfoProblem &problem`
: The problem to solve.

`const dfo::DfoOptions &options`
: Method choice and settings.

## Returns

A [`DfoResult`](../DfoResult.md).

## Practical Examples

### Example 1

The minimal call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.0, 0.0});
  problem.objective = f;
  const dfo::DfoResult r = dfo::DFO::solve(problem);
  out.writeln("minimizer =", r.minimizer.items<double>(), ", status =", dfo::to_string(r.status));
}
```

Output

```text
minimizer = [3.0000, 3.0000] , status = Converged
```

### Example 2

With options: a forced method and a small budget.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.0, 0.0});
  problem.objective = f;
  dfo::DfoOptions options;
  options.solver = dfo::DfoSolverKind::OrthoMads;
  options.max_evaluations = 40;
  const dfo::DfoResult r = dfo::DFO::solve(problem, options);
  out.writeln(dfo::to_string(r.selected_solver), ", status =", dfo::to_string(r.status), ", evaluations =", r.diagnostics.evaluations);
}
```

Output

```text
OrthoMads , status = ReachedEvaluationBudget , evaluations = 40
```

### Example 3

Constraints and bounds in the problem. The route is pattern search because of the constraint, and the bounds are respected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  dfo::DfoProblem problem;
  problem.initial = pluss::series::make({0.5, 0.5});
  problem.objective = f;
  problem.constraints = [](const Series &x) { return x.sum() - 2.0; };
  problem.lower_bounds = pluss::series::make({0.0, 0.0});
  problem.upper_bounds = pluss::series::make({5.0, 5.0});
  const dfo::DfoResult r = dfo::DFO::solve(problem);
  out.writeln(dfo::to_string(r.selected_solver), ", minimizer =", r.minimizer.items<double>());
}
```

Output

```text
PatternSearch , minimizer = [0.9954, 1.0045]
```

## See Also

- [`DFO`](../DFO.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="run.html">
    <span class="label">Previous</span>
    <span class="title">run</span>
  </a>
</div>
<!-- apiref:nav:end -->
