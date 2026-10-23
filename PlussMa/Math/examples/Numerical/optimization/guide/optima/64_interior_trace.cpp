// The interior method (`InternalProtocol`) replaces the inequalities by a barrier: a term that grows without
// limit as a constraint is approached, weighted by the barrier parameter. It solves a sequence of barrier
// problems with the weight driven toward zero, and each step keeps every slack variable and bound strictly
// positive. The progress hook shows the three quantities that tell the story: the barrier parameter (it falls
// by a factor each time the current barrier problem is solved well enough), the complementarity (the product of
// slack and multiplier, which must go to zero at a solution) and the stationarity.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The same four-variable problem as in chapter 3: one nonlinear inequality, one nonlinear equality, bounds.
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto hs71 = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto hs71_constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "barrier" << std::setw(16) << "complementarity"
            << std::setw(14) << "stationarity" << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.backend_parameter
              << std::setw(16) << s.complementarity << std::setw(14) << s.stationarity << std::setw(12)
              << s.evaluation.max_constraint_violation << std::fixed << std::setprecision(5) << std::setw(12)
              << s.evaluation.value.template item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(hs71, four(1, 5, 5, 1), options, hs71_constraints());
  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ", " << r.iteration << " iterations, "
            << r.function_evaluations << " evaluations\n";
  return 0;
}
