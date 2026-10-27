// Sequential QP is a loop, and the loop can be watched. The progress hook receives one snapshot per iteration:
// the step length taken, the penalty weight that balances objective against constraint violation, the
// stationarity measure and the violation. Reading these four columns is how you tell a healthy run (violation
// falls to the tolerance while stationarity falls to the tolerance) from a stalled one. The hook returns false to
// let the run continue.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::SequentialQP;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "step" << std::setw(12) << "penalty"
            << std::setw(14) << "stationarity" << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.step_size
              << std::setw(12) << s.backend_parameter << std::setw(14) << s.stationarity << std::setw(12)
              << s.evaluation.max_constraint_violation << std::fixed << std::setprecision(5) << std::setw(12)
              << s.evaluation.value.item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints());
  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations, "
            << r.function_evaluations << " evaluations\n" << r.message << "\n";
  return 0;
}
