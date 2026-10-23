// The working-set method keeps a list of the constraints it currently treats as active: equalities that must
// hold, and inequalities that are pushing on the answer. Each iteration solves a quadratic step for the
// constraints in the list, adds a constraint when the step would violate it, and drops one whose multiplier has
// turned the wrong way. The list is why this method pays off when most constraints never matter. This program
// watches a run, then reads the active set off the final multipliers: a positive inequality multiplier is an
// active constraint, zero is inactive.
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

// min sum (x_j - 2)^4 + 0.1 x_j^2 over 6 variables (not quadratic, so every iteration is a genuine step), with `extra` linear inequalities on top of two that matter:
// sum x <= 6 and x_0 <= 0.5. The extra rows have generous right-hand sides and are never active.
struct Rows { pluss::Series A, b; };
Rows make_rows(int extra) {
  const int n = 6, m = 2 + extra;
  std::vector<double> A(static_cast<std::size_t>(m * n), 0.0), b(static_cast<std::size_t>(m), 0.0);
  for (int j = 0; j < n; ++j) A[j] = 1.0;
  b[0] = 6.0;
  A[n + 0] = 1.0;
  b[1] = 0.5;
  for (int k = 0; k < extra; ++k) {
    double l1 = 0.0;
    for (int j = 0; j < n; ++j) {
      const double a = std::sin(1.7 * (k + 1) + 0.9 * (j + 1));
      A[(2 + k) * n + j] = a;
      l1 += std::abs(a);
    }
    b[2 + k] = 3.0 * l1;
  }
  return {pluss::make_series(A, {m, n}).to(f64), pluss::make_series(b, {m}).to(f64)};
}
auto objective = [](const optim::Trace& x) { return optim::sum(optim::pow(x - 2.0, 4.0)) + 0.1 * optim::sum(optim::square(x)); };

int main() {
  const auto rows = make_rows(4);
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::WorkingSet;
  std::cout << std::right << std::setw(5) << "iter" << std::setw(12) << "step" << std::setw(14) << "stationarity"
            << std::setw(12) << "violation" << std::setw(12) << "objective\n";
  options.progress_hook = [](const optim::OptimaProgressSnapshot& s) {
    if (s.stage != optim::OptimaProgressStage::Iteration) return false;
    std::cout << std::setw(5) << s.iteration << std::scientific << std::setprecision(2) << std::setw(12) << s.step_size
              << std::setw(14) << s.stationarity << std::setw(12) << s.evaluation.max_constraint_violation << std::fixed
              << std::setprecision(5) << std::setw(12) << s.evaluation.value.template item<double>() << "\n";
    return false;
  };
  const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), options, optim::constraints().ln_ineq(rows.A, rows.b));

  std::cout << "\nexit flag " << static_cast<int>(r.exit_flag) << ": " << r.message.substr(0, r.message.find('\n')) << "\n";
  const auto x = r.params.at("x").to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "x = [";
  for (double v : x) std::cout << " " << v;
  std::cout << " ]\nmultipliers of the 6 inequality rows:";
  for (double v : r.multipliers.linear_inequalities.to_vec<double>()) std::cout << " " << v;
  std::cout << "\n";
  return 0;
}
