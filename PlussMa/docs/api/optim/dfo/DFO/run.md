<!-- generated from run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4f182bc5482b -->

# `DFO::run`

[Back to `dfo::DFO`](../DFO.md)

Solves a problem with the options the object holds.

## Signature

```cpp
dfo::DfoResult dfo::DFO::run(const dfo::DfoProblem &problem) const;
```

## What It Does

Checks `problem`, chooses and runs a method, and returns the result. The object is not changed, so one `DFO` can solve many problems with the same settings.

## Parameters

`const dfo::DfoProblem &problem`
: The problem to solve.

## Returns

A [`DfoResult`](../DfoResult.md).

## Errors

- Missing start, missing objective and black box, both `constraints` and `constraints_values`, or bounds of the wrong length throw.

## Practical Examples

### Example 1

One solver, one problem.

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
  const dfo::DFO solver;
  const dfo::DfoResult r = solver.run(problem);
  out.writeln("minimizer =", r.minimizer.items<double>(), ", solver =", dfo::to_string(r.selected_solver));
}
```

Output

```text
minimizer = [3.0000, 3.0000] , solver = NelderMead
```

### Example 2

One solver, several problems. The same options (here a pattern-search choice) solve three different targets.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  dfo::DfoOptions options;
  options.solver = dfo::DfoSolverKind::PatternSearch;
  const dfo::DFO solver(options);
  for (const double target : {1.0, 2.0, 3.0}) {
    dfo::DfoProblem problem;
    problem.initial = pluss::series::make({0.0, 0.0});
    problem.objective = [target](const Series &x) { return (x - target).square().sum(); };
    const dfo::DfoResult r = solver.run(problem);
    out.writeln("target", target, ": minimizer =", r.minimizer.items<double>());
  }
}
```

Output

```text
target 1.0000 : minimizer = [1.0000, 1.0000]
target 2.0000 : minimizer = [2.0000, 2.0000]
target 3.0000 : minimizer = [3.0000, 3.0000]
```

### Example 3

A bad problem is reported by name. Giving both `constraints` and `constraints_values` is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

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
  problem.constraints = [](const Series &x) { return x.sum() - 2.0; };
  problem.constraints_values = [](const Series &x) { dfo::DfoConstraintValues c; c.inequality = x.sum() - 2.0; return c; };
  try {
    (void)dfo::DFO().run(problem);
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
DFO::solve: provide either constraints or constraints_values, not both
```

## See Also

- [`DFO`](../DFO.md): the class

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="build.html">
    <span class="label">Previous</span>
    <span class="title">build</span>
  </a>
  <a class="next-link" href="solve.html">
    <span class="label">Next</span>
    <span class="title">solve</span>
  </a>
</div>
<!-- apiref:nav:end -->
