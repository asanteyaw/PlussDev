// Constraints take the same three forms as the objective. Here the nonlinear constraint is supplied by hand: a callback
// returns the residuals and, for each parameter block, a Jacobian of shape [constraints, block size]. The problem
// minimizes x^2 + y^2 subject to x*y >= 4 (written as 4 - x*y <= 0), whose solution is x = y = 2. The program solves it
// with the traced constraint and with the analytic one, audits the analytic Jacobian against the traced one, and then
// breaks one Jacobian entry to show the audit catching it.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }
pluss::Series row(double v) { return pluss::make_series(std::vector<double>{v}, {1, 1}).to(f64); }  // a [1, 1] Jacobian block

optim::ConstrainedProblem problem(bool analytic, double jacobian_x_scale) {
  optim::ConstrainedProblem prob;
  prob.objective.autodiff = [](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return p.at("x").square() + p.at("y").square();
  };
  // The traced form is always present: it is what the audit compares against.
  prob.nonlinear.inequality = [](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return 4.0 - p.at("x") * p.at("y");
  };
  if (analytic) {
    prob.nonlinear.analytic = [jacobian_x_scale](const optim::ParameterPack<pluss::Series>& p) {
      const double x = as_f64(p.at("x")), y = as_f64(p.at("y"));
      optim::NonlinearConstraintEvaluation e;
      e.inequality = pluss::make_series(std::vector<double>{4.0 - x * y}, {1}).to(f64);
      e.inequality_jacobian = optim::ParameterPack<pluss::Series>(
          std::vector<optim::ParameterPack<pluss::Series>::Block>{{"x", row(-y * jacobian_x_scale)}, {"y", row(-x)}});
      return e;
    };
  }
  prob.bounds.lower = pair(0.1, 0.1);
  prob.bounds.upper = pair(10, 10);
  return prob;
}

int main() {
  for (bool analytic : {false, true}) {
    const auto r = optim::optima(problem(analytic, 1.0), pair(1, 3));
    std::cout << (analytic ? "analytic Jacobian: " : "traced constraint:  ") << "exit flag " << static_cast<int>(r.exit_flag)
              << ", " << r.iteration << " iterations, x = " << std::fixed << std::setprecision(5) << as_f64(r.params.at("x"))
              << ", y = " << as_f64(r.params.at("y")) << ", violation " << std::scientific << std::setprecision(1)
              << r.evaluation.max_constraint_violation << "\n";
  }

  std::cout << "\naudit of the constraint callbacks at (1.5, 2.5):\n";
  for (double scale : {1.0, 2.0}) {
    const auto report = optim::audit_optima_derivatives(problem(true, scale), pair(1.5, 2.5));
    std::cout << (scale == 1.0 ? "correct Jacobian:       " : "d/dx entry doubled:    ") << "passed = " << std::boolalpha
              << report.nonlinear_inequalities.passed << ", largest Jacobian difference = " << std::scientific
              << std::setprecision(2) << report.nonlinear_inequalities.derivative_abs_diff << "\n";
  }
  return 0;
}
