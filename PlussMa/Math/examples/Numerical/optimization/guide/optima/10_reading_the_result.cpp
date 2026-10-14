// What optima returns. The result carries the answer, the objective value there, how well every
// constraint is met, how much work was done, why the solver stopped, and the Lagrange multipliers.
// Problem: minimize (x0 - 2)^2 + (x1 - 1)^2 subject to x0 + x1 <= 2 and 0 <= x <= 1.8.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto mat = [&](std::vector<double> v, std::int64_t rows, std::int64_t cols) {
    return pluss::make_series(v, {rows, cols}).to(f64);
  };

  const auto target = optim::trace(vec({2.0, 1.0}), false);
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - target)); };
  const auto constraints = optim::constraints()
                               .ln_ineq(mat({1, 1}, 1, 2), vec({2.0}))
                               .lbs(vec({0.0, 0.0}))
                               .ubs(vec({1.8, 1.8}));
  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, constraints);

  std::cout << std::fixed << std::setprecision(4) << std::boolalpha;
  const auto x = r.params.at("x").to_vec<double>();
  std::cout << "answer           x = [" << x[0] << ", " << x[1] << "]\n";
  std::cout << "objective value    = " << r.evaluation.value.item<double>() << "\n";
  std::cout << "feasible           = " << r.evaluation.feasible << "\n";
  std::cout << "max violation      = " << r.evaluation.max_constraint_violation << "\n";
  std::cout << "converged          = " << r.converged << "\n";
  std::cout << "exit flag          = " << static_cast<int>(r.exit_flag) << " (Converged is 1)\n";
  std::cout << "iterations         = " << r.iteration << "\n";
  std::cout << "evaluations        = " << r.function_evaluations << "\n";
  std::cout << "stationarity below 1e-5 = " << (r.stationarity < 1e-5) << "\n";
  std::cout << "message            = " << r.message << "\n";

  const auto& m = r.multipliers;
  auto count = [](const pluss::Series& s) { return s.defined() ? s.numel() : 0; };
  std::cout << "linear inequality multiplier = " << m.linear_inequalities.to_vec<double>()[0] << "\n";
  std::cout << "multiplier counts: linear eq " << count(m.linear_equalities) << ", nonlinear eq "
            << count(m.nonlinear_equalities) << ", linear ineq " << count(m.linear_inequalities)
            << ", nonlinear ineq " << count(m.nonlinear_inequalities) << ", lower bounds "
            << count(m.lower_bounds) << ", upper bounds " << count(m.upper_bounds) << "\n";

  std::cout << "-- compact summary --\n";
  std::cout << std::defaultfloat << std::setprecision(6);
  optim::reports::compact_summary(std::cout, r) << "\n";
  return 0;
}
