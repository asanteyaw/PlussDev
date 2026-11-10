<!-- generated from reports_trace.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::reports::trace`

A progress hook that prints a line for the start, each iteration and the end, with the algorithm's message.

## Signature

```cpp
std::function<bool(const optim::OptimaProgressSnapshot &)> optim::reports::trace(std::ostream &os, optim::reports::ProgressTracePreset preset = {});
```

## What It Does

Prints `iter`, `evals`, `objective`, `viol`, `stationarity`, `backend`, `step`, optionally `krylov`, the stall category and the message of the step (for example `step accepted` or `optimality and feasibility tolerances satisfied`). The `ProgressTracePreset` chooses what to include: `include_start` and `include_done` for the initial and final lines, `include_message`, `include_krylov`, and the number of decimals `precision`. It never stops the run. After the last step the converged state is reported once more with the reason for stopping as the message, and the done line repeats it, so the final point appears in three lines.

## Parameters

`os`
: The stream.

`preset`
: What to print and with how many decimals.

## Returns

A hook function.

## Practical Examples

### Example 1

The full trace.

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
  options.sequential_qp.progress_hook = optim::reports::trace(std::cout, {.precision = 4});
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=0 evals=1 objective=5.0000 viol=0.0000 stationarity=4.0000 backend=10.0000 step=0.0000 krylov=0 stall=none msg="initialized"
iter=1 evals=5 objective=1.2500 viol=0.2500 stationarity=1.0000 backend=5.0000 step=0.2500 krylov=0 stall=none msg="step accepted"
iter=2 evals=6 objective=1.5125 viol=0.0125 stationarity=0.0500 backend=2.5000 step=1.0000 krylov=0 stall=none msg="step accepted"
iter=3 evals=7 objective=1.5278 viol=0.0000 stationarity=0.0058 backend=1.3687 step=1.0000 krylov=0 stall=none msg="step accepted"
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 krylov=0 stall=none msg="step accepted"
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
iter=4 evals=8 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=1.3697 step=1.0000 krylov=0 stall=none msg="optimality and feasibility tolerances satisfied"
```

### Example 2

Without the start and end lines or the message.

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

  optim::reports::ProgressTracePreset preset;
  preset.include_start = false;
  preset.include_done = false;
  preset.include_message = false;
  optim::OptimaOptions options;
  options.sequential_qp.progress_hook = optim::reports::trace(std::cout, preset);
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=1 evals=5 objective=1.250000 viol=0.250000 stationarity=1.000000 backend=5.000000 step=0.250000 krylov=0 stall=none
iter=2 evals=6 objective=1.512500 viol=0.012500 stationarity=0.050000 backend=2.500000 step=1.000000 krylov=0 stall=none
iter=3 evals=7 objective=1.527816 viol=0.000039 stationarity=0.005833 backend=1.368704 step=1.000000 krylov=0 stall=none
iter=4 evals=8 objective=1.527864 viol=0.000000 stationarity=0.000019 backend=1.369675 step=1.000000 krylov=0 stall=none
iter=4 evals=8 objective=1.527864 viol=0.000000 stationarity=0.000000 backend=1.369675 step=1.000000 krylov=0 stall=none
```

### Example 3

On the interior algorithm the lines show the barrier parameter shrinking in `backend`.

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

  optim::OptimaOptions options = optim::presets::internal_protocol();
  options.internal_protocol.progress_hook = optim::reports::trace(std::cout, {.include_message = false, .precision = 4});
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
iter=0 evals=2 objective=5.0000 viol=0.0000 stationarity=1.0000 backend=0.0100 step=0.0000 krylov=0 stall=none
iter=1 evals=6 objective=2.8125 viol=0.0000 stationarity=0.7475 backend=0.0100 step=0.1250 krylov=0 stall=none
iter=2 evals=8 objective=1.8842 viol=0.0000 stationarity=0.5980 backend=0.0100 step=0.1859 krylov=0 stall=none
iter=3 evals=9 objective=1.5049 viol=0.0188 stationarity=0.3992 backend=0.0100 step=0.1398 krylov=0 stall=none
iter=4 evals=10 objective=1.5563 viol=0.0000 stationarity=0.0085 backend=0.0010 step=1.0000 krylov=0 stall=none
iter=5 evals=11 objective=1.5288 viol=0.0000 stationarity=0.0000 backend=0.0000 step=1.0000 krylov=0 stall=none
iter=6 evals=12 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=0.0000 step=1.0000 krylov=0 stall=none
iter=7 evals=13 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=0.0000 step=0.9955 krylov=0 stall=none
iter=7 evals=13 objective=1.5279 viol=0.0000 stationarity=0.0000 backend=0.0000 step=0.9955 krylov=0 stall=none
```

## See Also

- [`reports::iter`](reports_iter.md): compact lines
- [`reports::iteration_only_trace`](reports_iteration_only_trace.md): iterations only

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reports_iteration_only_trace.html">
    <span class="label">Previous</span>
    <span class="title">iteration_only_trace</span>
  </a>
</div>
<!-- apiref:nav:end -->
