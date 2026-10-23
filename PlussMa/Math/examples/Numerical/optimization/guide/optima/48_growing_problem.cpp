// How the work grows with the number of variables. The problem is a smoothing fit: stay close to a target
// profile while no two neighbouring values differ by more than a fixed step and the values sum to a given
// total. It has n variables, 2 (n - 1) linear inequalities, one linear equality and bounds, and its
// solution has many active constraints. The table shows iterations and function evaluations for n = 8, 16
// and 32; evaluations are what cost money when the objective is expensive.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::left << std::setw(18) << "iters / evals";
  for (int n : {8, 16, 32}) std::cout << std::setw(14) << ("n = " + std::to_string(n));
  std::cout << "\n";

  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (int n : {8, 16, 32}) {
      std::vector<double> target(n), start(n), lo(n, 0.0), hi(n, 1.0);
      double total = 0.0;
      for (int i = 0; i < n; ++i) {
        target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
        start[i] = (i % 2 == 0) ? 0.9 : 0.1;
        total += target[i];
      }
      std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
      std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
      for (int i = 0; i + 1 < n; ++i) {
        diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
        diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
        diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
      }
      auto vec = [&](const std::vector<double>& v) {
        return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
      };
      const auto goal = optim::trace(vec(target), false);
      auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - goal)); };
      const auto constraints =
          optim::constraints()
              .ln_ineq(pluss::make_series(diff, {2 * (n - 1), n}).to(f64), vec(cap))
              .ln_eq(pluss::make_series(std::vector<double>(n, 1.0), {1, n}).to(f64), vec({total}))
              .lbs(vec(lo))
              .ubs(vec(hi));
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.penalty.max_iterations = 2000;
      options.sequential_qp.max_iterations = 500;
      options.working_set.max_iterations = 500;
      options.internal_protocol.max_iterations = 500;
      const auto r = optim::optima(objective, vec(start), options, constraints);
      std::cout << std::setw(14)
                << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations) +
                    (r.exit_flag == optim::OptimaExitFlag::Converged ? "" : " *"));
    }
    std::cout << "\n";
  }
  std::cout << "* did not converge\n";
  return 0;
}
