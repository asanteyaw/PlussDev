// The five flags that mean the solver could not deliver a solution. Each comes with the program
// state that triggers it, and the result still carries the last point reached.
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

  // NoFeasiblePoint: the interior-point method needs a strictly feasible start region and finds none.
  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;
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
