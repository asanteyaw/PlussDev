<!-- generated from reports_iter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::reports::iter`

A ready-made progress hook that prints one line per iteration; it works for both `optima` and `septima`.

## Signature

```cpp
optim::reports::IterReport optim::reports::iter(std::ostream &os, int precision = 6, std::int64_t every = 1);
```

## What It Does

`iter(os)` returns a callable to use as a `progress_hook`. Each iteration it writes one line with the iteration number, evaluation count, objective, constraint violation, stationarity, backend parameter and step size. `precision` is the number of decimals; `every` prints only every n-th iteration (the first is always printed). The hook never stops the run. The last line appears twice: after the final step the solver reports the converged state once more, right after its convergence test. The same object can be assigned to the hook of either solver, since it accepts both solvers' snapshots. For a hook that also prints start, done and message lines, use [`reports::trace`](reports_trace.md).

## Parameters

`os`
: The stream to write to.

`precision`
: Decimals.

`every`
: Print every n-th iteration.

## Returns

A callable usable as a `progress_hook`.

## Practical Examples

### Example 1

One line per iteration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.progress_hook = optim::reports::iter(std::cout, 4);
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=1 evals=5 objective=1.2500 viol=0.2500 stationarity=1.0000 backend=5.0000 step=0.2500 stall=none
iter=2 evals=6 objective=1.5125 viol=0.0125 stationarity=0.0500 backend=2.5000 step=1.0000 stall=none
iter=3 evals=7 objective=1.5278 viol=0.0000 stationarity=0.0058 backend=1.3687 step=1.0000 stall=none
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 stall=none
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 stall=none
```

### Example 2

Every second iteration, on the Rosenbrock valley where the run is long. Iteration 1 is always printed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const pluss::Series &x) {
    const pluss::Series a = x.select(0, 0), b = x.select(0, 1);
    return (1.0 - a).square() + 100.0 * (b - a.square()).square();
  };
  optim::OptimaOptions options;
  options.progress_hook = optim::reports::iter(std::cout, 3, 10);
  (void)optim::optima(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
}
```

Output

```text
iter=1 evals=4 objective=7.784 viol=0.000 stationarity=1.000 backend=5.000 step=0.250 stall=none
iter=10 evals=15 objective=2.104 viol=0.000 stationarity=0.042 backend=0.010 step=1.000 stall=none
iter=20 evals=28 objective=0.246 viol=0.000 stationarity=0.016 backend=0.010 step=1.000 stall=none
iter=30 evals=41 objective=0.000 viol=0.000 stationarity=0.001 backend=0.010 step=1.000 stall=none
```

### Example 3

Inside the algorithm's own options. Putting the hook in `sequential_qp` takes precedence over the top-level one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.progress_hook = optim::reports::iter(std::cout, 2);
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=1 evals=5 objective=1.25 viol=0.25 stationarity=1.00 backend=5.00 step=0.25 stall=none
iter=2 evals=6 objective=1.51 viol=0.01 stationarity=0.05 backend=2.50 step=1.00 stall=none
iter=3 evals=7 objective=1.53 viol=0.00 stationarity=0.01 backend=1.37 step=1.00 stall=none
iter=4 evals=8 objective=1.53 viol=0.00 stationarity=0.00 backend=1.37 step=1.00 stall=none
iter=4 evals=8 objective=1.53 viol=0.00 stationarity=0.00 backend=1.37 step=1.00 stall=none
```

## See Also

- [`reports::trace`](reports_trace.md): fuller lines
- [`reports::iteration_only_trace`](reports_iteration_only_trace.md): the Optima-only equivalent

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reports_detailed_summary.html">
    <span class="label">Previous</span>
    <span class="title">detailed_summary</span>
  </a>
  <a class="next-link" href="reports_iteration_only_trace.html">
    <span class="label">Next</span>
    <span class="title">iteration_only_trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
