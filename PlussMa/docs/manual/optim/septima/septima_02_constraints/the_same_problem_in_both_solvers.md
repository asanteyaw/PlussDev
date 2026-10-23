<!-- generated from the_same_problem_in_both_solvers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The same problem in both solvers

[Back to Constraints](../septima_02_constraints.md)


Three parameters and one of every kind of constraint: bounds, a linear and a nonlinear equality, a linear and a nonlinear inequality. The coupling `a b = 0.3` makes the problem non-convex. Optima, which solves a quadratic program at every step, is the reference.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<Trace(const Pack&)> objective = [](const Pack& p) {
    return optim::square(p.at("a")) + 2.0 * optim::square(p.at("b")) + 3.0 * optim::square(p.at("c")) - 4.0 * p.at("a");
  };
  const auto start = optim::pack_scalars({{"a", scalar(0.5)}, {"b", scalar(0.5)}, {"c", scalar(0.5)}});

  std::cout << std::left << std::setw(26) << "solver" << std::setw(16) << "exit" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << std::setw(12) << "violation" << "a, b, c\n";
  const auto row = [](const char* label, const char* exit, std::int64_t evals, double value, double violation, double a, double b, double c) {
    std::cout << std::left << std::setw(26) << label << std::setw(16) << exit << std::setw(8) << evals << std::fixed << std::setprecision(5)
              << std::setw(12) << value << std::scientific << std::setprecision(1) << std::setw(12) << violation << std::fixed << std::setprecision(4) << a
              << ", " << b << ", " << c << "\n";
  };

  // Optima: constraints as the families it knows.
  const auto optima_constraints =
      optim::constraints()
          .lbs(optim::pack_scalars({{"a", scalar(0)}, {"b", scalar(0)}, {"c", scalar(0)}}))
          .ubs(optim::pack_scalars({{"a", scalar(1.5)}, {"b", scalar(2)}, {"c", scalar(2)}}))
          .ln_eq(pluss::make_series(std::vector<double>{0, 1, 1}, {1, 3}).to(f64), pluss::make_series(std::vector<double>{1.0}, {1}).to(f64))
          .ln_ineq(pluss::make_series(std::vector<double>{1, 1, 1}, {1, 3}).to(f64), pluss::make_series(std::vector<double>{2.5}, {1}).to(f64))
          .nl_eq([](const Pack& p) { return p.at("a") * p.at("b") - 0.3; })
          .nl_ineq([](const Pack& p) { return optim::square(p.at("c")) - 0.6; });
  const auto o = optim::optima(objective, start, {}, optima_constraints);
  row("Optima", "converged", o.function_evaluations, o.evaluation.value.item<double>(), o.evaluation.max_constraint_violation,
      o.params.at("a").item<double>(), o.params.at("b").item<double>(), o.params.at("c").item<double>());

  // Septima: the same requirements as bounds and residual functions over the list of blocks.
  optim::SeptimaConstraints c;
  c.lbs = {scalar(0.0), scalar(0.0), scalar(0.0)};
  c.ubs = {scalar(1.5), scalar(2.0), scalar(2.0)};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[1] + x[2] - 1.0, x[0] * x[1] - 0.3}; };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0] + x[1] + x[2] - 2.5, x[2].square() - 0.6}; };

  const auto name = [](const optim::SeptimaResult& r) {
    switch (r.exit_flag) {
      case optim::SeptimaExitFlag::Converged: return "converged";
      case optim::SeptimaExitFlag::MaxIterations: return "MaxIterations";
      case optim::SeptimaExitFlag::NoProgress: return "NoProgress";
      default: return "other";
    }
  };
  optim::SeptimaSolveOptions adam;  // the library's constraint schedule, default penalties
  adam.max_iterations = 5000;
  adam.gradient_tolerance = 1e-5;
  adam.step_tolerance = 1e-6;
  adam.step.adam.learning_rate = 0.02;
  const auto a = optim::septima(objective, start, adam, c);
  row("Septima, Adam", name(a), a.function_evaluations, a.objective_value, a.constraint_violation, a.params[0].item<double>(), a.params[1].item<double>(),
      a.params[2].item<double>());

  optim::SeptimaSolveOptions lbfgs = adam;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto l = optim::septima(objective, start, lbfgs, c);
  row("Septima, L-BFGS", name(l), l.function_evaluations, l.objective_value, l.constraint_violation, l.params[0].item<double>(), l.params[1].item<double>(),
      l.params[2].item<double>());
  return 0;
}
```

Output

```text
solver                    exit            evals   objective   violation   a, b, c
Optima                    converged       6       -1.65076    2.2e-10     1.3309, 0.2254, 0.7746
Septima, Adam             converged       854     -1.65075    8.3e-08     1.3309, 0.2254, 0.7746
Septima, L-BFGS           converged       242     -1.65076    5.0e-07     1.3310, 0.2254, 0.7746
```
Septima reaches the same solution as Optima, `(1.331, 0.2254, 0.7746)` with an objective of -1.65076, with the default settings. Adam needs 854 evaluations and ends with a violation of 8.3e-08; L-BFGS needs 242 evaluations and ends with 5.0e-07. Optima needs 6 evaluations and ends with 2.2e-10. So the answers agree, and the differences are in cost and in precision: Septima stops when the violation is within its tolerance (1e-6 by default), Optima solves each step to a far tighter one, and Optima uses second-order information that lets it finish in a handful of steps.

Choose by what the problem needs. For a small problem with a few nonlinear constraints that must hold to high precision, Optima is the better tool. For a problem with many parameters, mostly bounds and simple constraints, or complex parameters, or an objective you can differentiate but whose Hessian is out of reach, Septima gives the same answer with only gradients. Chapter 5 gives a decision table.
