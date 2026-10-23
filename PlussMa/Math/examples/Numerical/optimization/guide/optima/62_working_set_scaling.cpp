// The reason usually given to choose the working-set method: most of a large constraint list is inactive, so following
// only the active rows should take less work. Here two rows matter and `extra` rows never do. Sequential QP and the
// interior method carry every row into every iteration; the working-set method follows the rows that are active. The
// table gives iterations and function evaluations as `extra` grows, and the objective shows that all three find the
// same answer.
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
  const std::pair<const char*, optim::OptimaAlgorithm> algs[] = {
      {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  std::cout << std::left << std::setw(8) << "extra" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(7) << "evals" << std::setw(12) << "objective\n";
  for (int extra : {0, 100, 500, 2000}) {
    const auto rows = make_rows(extra);
    for (const auto& [name, alg] : algs) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      const auto r = optim::optima(objective, vec({0, 0, 0, 0, 0, 0}), o, optim::constraints().ln_ineq(rows.A, rows.b));
      std::cout << std::left << std::setw(8) << extra << std::setw(18) << name << std::setw(6) << static_cast<int>(r.exit_flag)
                << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(6)
                << r.evaluation.value.template item<double>() << "\n";
    }
  }
  return 0;
}
