// The same constrained problem in both solvers. Three parameters a, b, c; minimize a^2 + 2 b^2 + 3 c^2 - 4 a subject to
// bounds 0 <= a <= 1.5, 0 <= b <= 2, 0 <= c <= 2, a linear equality b + c = 1, a nonlinear equality a b = 0.3, a linear
// inequality a + b + c <= 2.5 and a nonlinear inequality c^2 <= 0.6. Optima, which solves each step as a quadratic program, is the
// reference. Septima handles every constraint through penalties and multipliers, with Adam and with L-BFGS, both with the library's
// default constraint schedule. The table reports the objective, the worst constraint violation and the number of function
// evaluations.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> objective = [](const Pack& p) {
    return p.at("a").square() + 2.0 * p.at("b").square() + 3.0 * p.at("c").square() - 4.0 * p.at("a");
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
          .nl_ineq([](const Pack& p) { return p.at("c").square() - 0.6; });
  const auto o = optim::optima(objective, start, {}, optima_constraints);
  row("Optima", "converged", o.function_evaluations, o.evaluation.value.item<double>(), o.evaluation.max_constraint_violation,
      as_f64(o.params.at("a")), as_f64(o.params.at("b")), as_f64(o.params.at("c")));

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
  row("Septima, Adam", name(a), a.function_evaluations, a.objective_value, a.constraint_violation, as_f64(a.params[0]), as_f64(a.params[1]),
      as_f64(a.params[2]));

  optim::SeptimaSolveOptions lbfgs = adam;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto l = optim::septima(objective, start, lbfgs, c);
  row("Septima, L-BFGS", name(l), l.function_evaluations, l.objective_value, l.constraint_violation, as_f64(l.params[0]), as_f64(l.params[1]),
      as_f64(l.params[2]));
  return 0;
}
