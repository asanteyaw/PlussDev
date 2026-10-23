<!-- generated from the_five_failure_flags.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The five failure flags

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


Five flags mean the solver could not deliver. Each is triggered here by a specific condition, and the result still carries the last point reached so you can inspect it.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

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
            << "), max violation " << r.evaluation.max_constraint_violation << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  // No point satisfies x0^2 + x1^2 + 1 <= 0, and none satisfies x0^2 + x1^2 + 1 = 0.
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  // NoFeasiblePoint: the interior method keeps bounds and linear rows strictly satisfied, so it first looks for
  // a point inside them. Here x0 + x1 <= -1 contradicts x >= 0, and it finds none.
  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  const auto impossible_linear = optim::constraints()
                                     .lbs(vec({0.0, 0.0}))
                                     .ln_ineq(pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(f64), vec({-1.0}));
  report("interior method, impossible linear rows", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_linear));

  // ConstraintStationary also covers a nonlinear inequality that no point satisfies.
  report("interior method, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), interior, impossible_inequality));

  // NoProgress: the recovery step could not reduce the violation, so the run gave up.
  report("sequential QP, impossible inequality", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  // ConstraintStationary: the violation stopped decreasing at a point that is still infeasible.
  report("interior method, impossible equality", optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  // NonFiniteEvaluation: the objective is not a number at the start (log of a negative value).
  report("log of a negative start",
         optim::optima([](const optim::Trace& x) { return optim::sum(optim::log(x)); }, vec({-1.0, 1.0})));

  // InvalidProblem: the objective returned two values instead of one.
  report("objective returns a vector", optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0})));
  return 0;
}
```

Output

```text
interior method, impossible linear rows: NoFeasiblePoint (-2), max violation 1
interior method, impossible inequality: ConstraintStationary (-7), max violation 1
sequential QP, impossible inequality: NoProgress (-6), max violation 1
interior method, impossible equality: ConstraintStationary (-7), max violation 1
log of a negative start: NonFiniteEvaluation (-4), max violation 0
objective returns a vector: InvalidProblem (-5), max violation 0
```
Impossible constraints produce different verdicts depending on their kind and the method. The interior method keeps bounds and linear rows strictly satisfied, so linear rows that contradict a bound (`x0 + x1 <= -1` with `x >= 0`) end the run before it starts with `NoFeasiblePoint`. A nonlinear inequality that no point satisfies, `x0^2 + x1^2 + 1 <= 0`, is different: the interior method gives it a slack variable and can start, so the verdict is `ConstraintStationary` once the violation can no longer be reduced (the gradient of its square vanishes), with the violation staying at `1`. Sequential QP tries to recover from the same inequality and fails, returning `NoProgress`. The impossible equality returns `ConstraintStationary` from the interior method for the same reason. A `log` of a negative start gives `NonFiniteEvaluation`, and an objective that returns a vector gives `InvalidProblem` with no exception thrown.
