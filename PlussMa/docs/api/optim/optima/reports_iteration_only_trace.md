<!-- generated from reports_iteration_only_trace.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::reports::iteration_only_trace`

A progress hook that prints only iteration lines, without start, end, message or Krylov counts.

## Signature

```cpp
std::function<bool(const optim::OptimaProgressSnapshot &)> optim::reports::iteration_only_trace(std::ostream &os, int precision = 6);
```

## What It Does

The shortest trace: `iter`, `evals`, `objective`, `viol`, `stationarity`, `backend`, `step` and the stall category per iteration, with `precision` decimals. [`reports::iter`](reports_iter.md) is built on it and adds the `every` thinning and `septima` support.

## Parameters

`os`
: The stream.

`precision`
: Decimals.

## Returns

A hook function.

## Practical Examples

### Example 1

Four decimals.

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
  options.sequential_qp.progress_hook = optim::reports::iteration_only_trace(std::cout, 4);
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

Two decimals, on the penalty algorithm.

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
  options.algorithm = optim::OptimaAlgorithm::MeritMethod;
  options.penalty.max_iterations = 4;
  options.penalty.progress_hook = optim::reports::iteration_only_trace(std::cout, 2);
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=1 evals=2 objective=3.20 viol=0.00 stationarity=3.20 backend=10.00 step=0.10 stall=none
iter=2 evals=5 objective=1.80 viol=0.00 stationarity=2.40 backend=10.00 step=0.25 stall=none
iter=3 evals=9 objective=1.38 viol=0.13 stationarity=0.33 backend=10.00 step=0.12 stall=none
iter=4 evals=11 objective=1.40 viol=0.11 stationarity=0.12 backend=10.00 step=0.50 stall=iteration-limit
```

### Example 3

Combined with an early stop: the trace never stops the run, so wrap it.

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

  const auto trace = optim::reports::iteration_only_trace(std::cout, 3);
  optim::OptimaOptions options;
  options.progress_hook = [trace](const optim::OptimaProgressSnapshot &s) { trace(s); return s.iteration >= 2; };
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", evaluations =", result.function_evaluations);
}
```

Output

```text
iter=1 evals=5 objective=1.250 viol=0.250 stationarity=1.000 backend=5.000 step=0.250 stall=none
iter=2 evals=6 objective=1.513 viol=0.012 stationarity=0.050 backend=2.500 step=1.000 stall=none
exit = Stopped , iterations = 2 , evaluations = 6
```

## See Also

- [`reports::iter`](reports_iter.md): adds thinning
- [`reports::trace`](reports_trace.md): fuller lines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reports_iter.html">
    <span class="label">Previous</span>
    <span class="title">iter</span>
  </a>
  <a class="next-link" href="reports_trace.html">
    <span class="label">Next</span>
    <span class="title">trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
