<!-- generated from the_audit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The audit

[Back to Derivatives](../optima_07_derivatives.md)


A wrong hand-written gradient is not caught by the solver, which follows whatever it is given. `audit_optima_derivatives` evaluates your analytic objective and the traced objective at a point and reports the largest difference. It needs both forms of the same function: the traced one is the reference.

```cpp
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
```

Output

```text
correct gradient: passed = true, largest gradient difference = 2.84e-14
wrong sign in d/dy: passed = false, largest gradient difference = 1.76e+02

solving with each gradient (autodiff alongside is ignored):
correct: exit flag 1, objective 5.38e-10, 35 iterations
wrong:   exit flag -6, objective 4.42e+00, 6 iterations
```
The correct gradient differs from the traced one by 2.84e-14, rounding, and passes. With one sign wrong in the derivative with respect to `y`, the largest difference is 176 and the audit fails. The second half shows why it is worth running. The correct gradient converges in 35 iterations to an objective of 5.38e-10. The wrong one stops after 6 iterations with exit flag `-6` (`NoProgress`, chapter 2) at an objective of 4.42. The run does not crash or warn; it reports failure to progress, which looks like a hard problem rather than a bug in a derivative. Audit a new analytic derivative once, at a point that is not special (not zero, not a symmetric point where terms cancel), before you trust its runs. The tolerances are `DerivativeAuditOptions::absolute_tolerance` (1e-8) and `relative_tolerance` (1e-6).
