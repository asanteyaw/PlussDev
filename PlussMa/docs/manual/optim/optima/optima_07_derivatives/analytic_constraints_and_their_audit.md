<!-- generated from analytic_constraints_and_their_audit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Analytic constraints and their audit

[Back to Derivatives](../optima_07_derivatives.md)


Nonlinear constraints take the same forms. The analytic form returns the residuals and, for each block of parameters, a Jacobian of shape `[number of constraints, block size]`. A residual kind your problem does not have (here, equalities) may be left undefined. The problem minimizes `x^2 + y^2` subject to `x y >= 4`, written `4 - x y <= 0`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }
pluss::Series row(double v) { return pluss::make_series(std::vector<double>{v}, {1, 1}).to(f64); }  // a [1, 1] Jacobian block

optim::ConstrainedProblem problem(bool analytic, double jacobian_x_scale) {
  optim::ConstrainedProblem prob;
  prob.objective.autodiff = [](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace {
    return optim::square(p.at("x")) + optim::square(p.at("y"));
  };
  // The traced form is always present: it is what the audit compares against.
  prob.nonlinear.inequality = [](const optim::ParameterPack<optim::Trace>& p) -> optim::Trace {
    return 4.0 - p.at("x") * p.at("y");
  };
  if (analytic) {
    prob.nonlinear.analytic = [jacobian_x_scale](const optim::ParameterPack<pluss::Series>& p) {
      const double x = p.at("x").item<double>(), y = p.at("y").item<double>();
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
              << ", " << r.iteration << " iterations, x = " << std::fixed << std::setprecision(5) << r.params.at("x").item<double>()
              << ", y = " << r.params.at("y").item<double>() << ", violation " << std::scientific << std::setprecision(1)
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
```

Output

```text
traced constraint:  exit flag 1, 5 iterations, x = 2.00000, y = 2.00000, violation 5.9e-08
analytic Jacobian: exit flag 1, 5 iterations, x = 2.00000, y = 2.00000, violation 5.9e-08

audit of the constraint callbacks at (1.5, 2.5):
correct Jacobian:       passed = true, largest Jacobian difference = 0.00e+00
d/dx entry doubled:    passed = false, largest Jacobian difference = 2.50e+00
```
The traced and the analytic constraint give the same run: 5 iterations, `x = y = 2`, a violation of 5.9e-08. The audit of the constraint callbacks compares the analytic Jacobian with the traced one, and with a doubled `d/dx` entry it reports a difference of 2.5 and fails. The traced constraint stays in the problem for this purpose; when only the analytic form is present there is nothing to audit against, and the report says so instead of passing.
