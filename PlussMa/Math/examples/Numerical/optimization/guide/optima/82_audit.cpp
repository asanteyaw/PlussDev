// A hand-derived gradient is the most common source of a wrong answer, and the solver cannot tell: it follows whatever
// gradient it is given. `audit_optima_derivatives` compares an analytic objective with the autodiff version of the same
// function at a point and reports how far apart they are. The program audits a correct gradient, then a gradient with
// one wrong sign, and then shows what the solver does when it is handed the wrong one without being asked to check.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }

optim::ConstrainedProblem rosenbrock(double sign_of_gy) {
  optim::ConstrainedProblem prob;
  prob.objective.autodiff = [](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  };
  prob.objective.analytic = [sign_of_gy](const optim::ParameterPack<pluss::Series>& p) {
    const double x = p.at("x").item<double>(), y = p.at("y").item<double>();
    const double value = 100.0 * (y - x * x) * (y - x * x) + (1.0 - x) * (1.0 - x);
    return std::pair{scalar(value), pair(-400.0 * x * (y - x * x) - 2.0 * (1.0 - x), sign_of_gy * 200.0 * (y - x * x))};
  };
  prob.bounds.lower = pair(-2, -2);
  prob.bounds.upper = pair(2, 2);
  return prob;
}

int main() {
  const auto at = pair(-1.2, 1.0);
  for (double sign : {1.0, -1.0}) {
    const auto report = optim::audit_optima_derivatives(rosenbrock(sign), at);
    std::cout << (sign > 0 ? "correct gradient: " : "wrong sign in d/dy: ") << "passed = " << std::boolalpha << report.passed
              << ", largest gradient difference = " << std::scientific << std::setprecision(2)
              << report.objective.derivative_abs_diff << "\n";
  }

  // The same wrong gradient, handed to the solver without an audit. The analytic callback takes precedence.
  std::cout << "\nsolving with each gradient (autodiff alongside is ignored):\n";
  for (double sign : {1.0, -1.0}) {
    auto prob = rosenbrock(sign);
    prob.objective.autodiff = nullptr;
    const auto r = optim::optima(prob, at);
    std::cout << (sign > 0 ? "correct: " : "wrong:   ") << "exit flag " << static_cast<int>(r.exit_flag) << ", objective "
              << std::scientific << std::setprecision(2) << r.evaluation.value.item<double>() << ", " << r.iteration
              << " iterations\n";
  }
  return 0;
}
