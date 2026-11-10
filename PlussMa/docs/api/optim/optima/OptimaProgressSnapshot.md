<!-- generated from OptimaProgressSnapshot.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::OptimaProgressSnapshot`

What a progress hook receives: the state of the run at one moment.

Every algorithm calls its `progress_hook` with a snapshot at the start, after each iteration and at the end. `stage` says which (`Start`, `Iteration` or `Done`), `algorithm` names the running method, `params` and `evaluation` are the current point and its evaluation, and `iteration`, `function_evaluations`, `step_size`, `backend_parameter`, `stationarity`, `complementarity`, `krylov_iterations` and `message` are the counters and measures also seen in the trace. `stall_category`, `stall_measure` and `stall_reference` carry the diagnosis, and `best_feasible` the best feasible point so far. The hook returns `true` to stop the run. Printing helpers [`reports::iter`](reports_iter.md) and [`reports::trace`](reports_trace.md) are hooks built on it.

## Fields

### `stage`

`optim::OptimaProgressStage`, default `Start`.

`Start`, `Iteration` or `Done`.

### `algorithm`

`std::string`, default `empty`.

Short name of the running algorithm.

### `params`

`optim::ParameterPack<pluss::Series>`, default `empty`.

The current point.

### `evaluation`

`optim::ProblemEvaluation`, default `empty`.

The problem at the current point.

### `iteration`

`std::int64_t`, default `0`.

Iterations so far.

### `function_evaluations`

`std::int64_t`, default `0`.

Objective evaluations so far.

### `step_size`

`double`, default `0`.

The last step fraction.

### `backend_parameter`

`double`, default `0`.

Penalty, merit or barrier value.

### `stationarity`

`double`, default `0`.

Optimality measure.

### `complementarity`

`double`, default `0`.

Complementarity residual.

### `krylov_iterations`

`std::int64_t`, default `0`.

Iterative-solve iterations.

### `message`

`std::string`, default `empty`.

What just happened.

### `stall_category`

`optim::OptimaStallCategory`, default `None`.

Diagnosis.

### `stall_measure`

`double`, default `0`.

Number behind the diagnosis.

### `stall_reference`

`double`, default `0`.

Its reference.

### `best_feasible`

`std::optional<optim::BestFeasiblePoint>`, default `none`.

Best feasible point so far.

## Guidance

Passed to every `progress_hook`.

## Practical Examples

### Example 1

A custom hook that records the objective at each iteration.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  std::vector<double> history;
  optim::OptimaOptions options;
  options.sequential_qp.progress_hook = [&history](const optim::OptimaProgressSnapshot &s) {
    if (s.stage == optim::OptimaProgressStage::Iteration) history.push_back(s.evaluation.value.item<double>());
    return false;
  };
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("objective history =", history);
}
```

Output

```text
objective history = [1.2500, 1.5125, 1.5278, 1.5279, 1.5279]
```

### Example 2

Stopping on a condition: the hook returns `true` once the violation is below 0.1 and at least two iterations have run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  options.sequential_qp.progress_hook = [](const optim::OptimaProgressSnapshot &s) { return s.evaluation.max_constraint_violation < 1e-1 && s.iteration >= 2; };
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  out.writeln("exit =", optim::optima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration);
}
```

Output

```text
exit = Stopped , iterations = 2
```

### Example 3

The stage separates the start, the iterations and the end.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  options.sequential_qp.progress_hook = [&out](const optim::OptimaProgressSnapshot &s) {
    out.writeln(optim::optima_progress_stage_name(s.stage), "iteration", s.iteration, "message:", s.message);
    return false;
  };
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
}
```

Output

```text
start iteration 0 message: initialized
iter iteration 1 message: step accepted
iter iteration 2 message: step accepted
iter iteration 3 message: step accepted
iter iteration 4 message: step accepted
iter iteration 4 message: optimality and feasibility tolerances satisfied
done iteration 4 message: optimality and feasibility tolerances satisfied
```

## See Also

- [`OptimaOptions`](OptimaOptions.md): holds the hook
- [`reports::trace`](reports_trace.md): a ready-made hook

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="OptimaProblemScaling.html">
    <span class="label">Previous</span>
    <span class="title">OptimaProblemScaling</span>
  </a>
  <a class="next-link" href="OptimaStallCategory.html">
    <span class="label">Next</span>
    <span class="title">OptimaStallCategory</span>
  </a>
</div>
<!-- apiref:nav:end -->
