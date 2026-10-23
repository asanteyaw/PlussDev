<!-- generated from the_flags_that_mean_a_healthy_run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The flags that mean a healthy run

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


Four flags describe a run that behaved as designed: it finished, it ran out of its budget, you stopped it, or it reached a target you set. This program triggers each one and prints the flag, the `converged` convenience, and the iteration count.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;

std::string flag_name(optim::OptimaExitFlag flag) {
  switch (flag) {
    case optim::OptimaExitFlag::Converged: return "Converged";
    case optim::OptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::OptimaExitFlag::Stopped: return "Stopped";
    case optim::OptimaExitFlag::NoFeasiblePoint: return "NoFeasiblePoint";
    case optim::OptimaExitFlag::ObjectiveTargetReached: return "ObjectiveTargetReached";
    case optim::OptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::OptimaExitFlag::InvalidProblem: return "InvalidProblem";
    case optim::OptimaExitFlag::NoProgress: return "NoProgress";
    case optim::OptimaExitFlag::ConstraintStationary: return "ConstraintStationary";
  }
  return "?";
}

void report(const std::string& label, const optim::OptimaState& r) {
  std::cout << label << ": " << flag_name(r.exit_flag) << " (" << static_cast<int>(r.exit_flag)
            << "), converged=" << std::boolalpha << r.converged << ", iterations=" << r.iteration << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  // Converged: the usual outcome.
  report("finished normally", optim::optima(objective, vec({0.0, 0.0})));

  // MaxIterations: the iteration budget is spent before the tolerances are met.
  optim::OptimaOptions few;
  few.sequential_qp.max_iterations = 1;
  report("one iteration allowed", optim::optima(objective, vec({0.0, 0.0}), few, disc));

  // Stopped: the progress hook returned true, asking for an orderly stop.
  optim::OptimaOptions stop_early;
  int calls = 0;
  stop_early.progress_hook = [&](const optim::OptimaProgressSnapshot&) { return ++calls >= 2; };
  report("hook asked to stop", optim::optima(objective, vec({0.0, 0.0}), stop_early));

  // ObjectiveTargetReached: a feasible point at or below objective_limit ends the run at once.
  optim::OptimaOptions good_enough;
  good_enough.sequential_qp.objective_limit = 100.0;
  report("objective limit 100", optim::optima(objective, vec({0.0, 0.0}), good_enough));
  return 0;
}
```

Output

```text
finished normally: Converged (1), converged=true, iterations=1
one iteration allowed: MaxIterations (0), converged=false, iterations=1
hook asked to stop: Stopped (-1), converged=false, iterations=1
objective limit 100: ObjectiveTargetReached (-3), converged=false, iterations=0
```
Only the first run is `converged`. The second spent its single allowed iteration, so the flag is `MaxIterations` and the iterate is not a solution. The third stopped because the hook returned `true` on its second call; `Stopped` always means the caller asked. The fourth ended at iteration zero because the start already satisfied `objective_limit = 100`. A feasible point at or below the limit ends the run at once with `ObjectiveTargetReached`.
