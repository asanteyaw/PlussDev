<!-- generated from automatic_analytic_and_audited_derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Automatic, analytic and audited derivatives

[Back to Optima 7: Derivatives](../optima_07_derivatives.md)

Every algorithm in `optima` needs derivatives, and you can supply them three ways: write the objective with plain `Series` operations and let reverse-mode automatic differentiation produce exact gradients; write it with `fwd_objective<N>` for forward mode, which carries the value and `N` partial derivatives in plain doubles; or write the gradient by hand and return it with the value. The algorithms do not care which: they see the same numbers, so the three forms change what *you* write, not what the solver does.

The catch with a hand-written gradient is that the solver trusts it. A wrong sign gives a wrong answer without any warning, because the solver follows whatever it is given. `audit_optima_derivatives` is the guard: it evaluates your analytic objective and the traced one at a point and reports the largest difference. The programs show the same problem solved the three ways, an audit that catches a deliberately wrong gradient (and what the run does without it), and the same checks for nonlinear constraints and their Jacobians.

## Program 1: The same problem, three ways

The Rosenbrock function on a box, solved with a traced objective, a hand-written gradient and a forward-mode objective.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }

optim::ConstrainedProblem with_box(optim::ProblemBuilder b) { return b.lbs(pair(-2, -2)).ubs(pair(2, 2)).build(); }

int main() {
  // 1. Reverse-mode autodiff: write the function once on pluss::Series values.
  auto traced = with_box(optim::problem().objective([](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  }));

  // 2. Analytic: return the value and a pack with the same blocks holding the gradient.
  auto analytic = with_box(optim::problem().objective_analytic([](const optim::ParameterPack<pluss::Series>& p) {
    const double x = as_f64(p.at("x")), y = as_f64(p.at("y"));
    const double value = 100.0 * (y - x * x) * (y - x * x) + (1.0 - x) * (1.0 - x);
    const double gx = -400.0 * x * (y - x * x) - 2.0 * (1.0 - x);
    const double gy = 200.0 * (y - x * x);
    return std::pair{scalar(value), pair(gx, gy)};
  }));

  // 3. Forward mode: the N parameters arrive as an array of Fwd<N> values and the result is one Fwd<N>.
  using F = optim::Fwd<2>;
  auto forward = with_box(optim::problem().objective_analytic(optim::fwd_objective<2>([](const std::array<F, 2>& p) -> F {
    return 100.0 * optim::square(p[1] - optim::square(p[0])) + optim::square(1.0 - p[0]);
  })));

  struct Mode { const char* label; const optim::ConstrainedProblem* problem; };
  const Mode modes[] = {{"autodiff", &traced}, {"analytic", &analytic}, {"fwd_objective", &forward}};
  std::cout << std::left << std::setw(16) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations"
            << std::setw(14) << "evaluations" << std::setw(14) << "objective" << "x, y\n";
  for (const auto& m : modes) {
    const auto r = optim::optima(*m.problem, pair(-1.2, 1.0));
    const auto x = as_f64(r.params.at("x"));
    const auto y = as_f64(r.params.at("y"));
    std::cout << std::left << std::setw(16) << m.label << std::setw(8) << static_cast<int>(r.exit_flag) << std::setw(12)
              << r.iteration << std::setw(14) << r.function_evaluations << std::scientific << std::setprecision(2)
              << std::setw(14) << r.evaluation.value.item<double>() << std::fixed << std::setprecision(6) << x << ", " << y
              << "\n";
  }
  return 0;
}
```

Output

```text
mode            exit    iterations  evaluations   objective     x, y
autodiff        1       35          45            5.38e-10      0.999977, 0.999954
analytic        1       35          45            5.38e-10      0.999977, 0.999954
fwd_objective   1       35          45            5.38e-10      0.999977, 0.999954
```
The three runs are identical to the printed digits: 35 iterations, 45 evaluations, an objective of 5.38e-10, at (0.999977, 0.999954). That is the point. The form changes what you write, not what the algorithm does. `analytic` returns a pair, the value and a pack with the same blocks as the parameters holding the gradient. `fwd_objective<N>` takes a function of an `std::array<Fwd<N>, N>`, with the parameters flattened in block order, and `N` must equal the total number of parameters (a mismatch is an error that names both numbers).

## Program 2: The audit

A wrong hand-written gradient is not caught by the solver, which follows whatever it is given. `audit_optima_derivatives` evaluates your analytic objective and the traced objective at a point and reports the largest difference. It needs both forms of the same function: the traced one is the reference.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }

optim::ConstrainedProblem rosenbrock(double sign_of_gy) {
  optim::ConstrainedProblem prob;
  prob.objective.autodiff = [](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  };
  prob.objective.analytic = [sign_of_gy](const optim::ParameterPack<pluss::Series>& p) {
    const double x = as_f64(p.at("x")), y = as_f64(p.at("y"));
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

## Program 3: Analytic constraints and their audit

Nonlinear constraints take the same forms. The analytic form returns the residuals and, for each block of parameters, a Jacobian of shape `[number of constraints, block size]`. A residual kind your problem does not have (here, equalities) may be left undefined. The problem minimizes `x^2 + y^2` subject to `x y >= 4`, written `4 - x y <= 0`.

```cpp
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

## Decisions and pitfalls

- **Audit every hand-written derivative.** A wrong gradient does not crash; it stops early or wanders. Audit it against the traced version before you trust it.
- **Keep the traced form around for the audit.** The audit needs both forms of the same function; the traced one is the reference. When only the analytic form exists there is nothing to compare with.
- **The three forms are equivalent in result.** Choose by what is easy to write and fast to run, not by accuracy.
- **Jacobians have a fixed shape.** For each parameter block the Jacobian is `[number of constraints, block size]`; a residual kind your problem does not have (for example equalities) may be left undefined.
- **Rounding differences are a pass.** The correct gradient differed from the traced one by about 1e-14; only a difference many orders larger is a failure.

Where next: [Derivative tools, call counts and long loops](derivative_tools_and_costs.md) covers the derivative machinery on its own and its costs.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="derivative_tools_and_costs.html">
    <span class="label">Next</span>
    <span class="title">Derivative tools, call counts and long loops</span>
  </a>
</div>
<!-- apiref:nav:end -->
