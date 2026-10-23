// The ways a Septima run can end, each produced on purpose. `Converged`: the gradient, the step and the constraint
// violation are all within tolerance. `MaxIterations`: the iteration budget ran out first. `Stopped`: the progress hook
// returned false. `NoProgress`: the objective and the step stopped changing before the tolerances were met.
// `NonFiniteEvaluation`: the objective or its gradient became NaN or infinite. Invalid settings do not return a flag at
// all; the call throws, and the last block shows the message.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

const char* name_of(optim::SeptimaExitFlag flag) {
  switch (flag) {
    case optim::SeptimaExitFlag::Converged: return "Converged";
    case optim::SeptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::SeptimaExitFlag::Stopped: return "Stopped";
    case optim::SeptimaExitFlag::NoProgress: return "NoProgress";
    case optim::SeptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::SeptimaExitFlag::InvalidProblem: return "InvalidProblem";
  }
  return "?";
}

int main() {
  auto quadratic = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };
  const auto row = [](const char* what, const optim::SeptimaResult& r) {
    std::cout << std::left << std::setw(40) << what << std::setw(21) << name_of(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << r.message << "\n";
  };

  {
    optim::SeptimaSolveOptions o;
    o.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    row("L-BFGS on a quadratic", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // default Adam learning rate 0.001, default 200 iterations
    row("Adam with its default settings", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 0.1;
    o.progress_hook = [](const optim::SeptimaProgressSnapshot& s) { return s.iteration < 7; };  // true keeps going
    row("a hook that returns false at 7", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // a learning rate so small that nothing changes within the tolerances
    o.step.adam.learning_rate = 1e-18;
    o.max_no_progress_iterations = 5;
    row("a learning rate of 1e-18", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    // log of a negative number is NaN: a large first step leaves the domain.
    auto domain = [](const Trace& x) -> Trace { return optim::sum(optim::log(x)) * -1.0 + optim::sum(x * 0.1); };
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 100.0;
    o.step.nan_recovery_attempts = 0;
    row("a step into log's undefined region", optim::septima(domain, vec({1.0}), o));
  }

  std::cout << "\nInvalid settings throw:\n";
  try {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = -1.0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  try {
    optim::SeptimaSolveOptions o;
    o.max_iterations = 0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
