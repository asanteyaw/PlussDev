// The smallest Septima problem: find the point where f(x) = sum((x - 3)^2) is smallest, started from the origin. The
// objective is written on `pluss::Series` values, which record the arithmetic so that the gradient can be taken automatically.
// The same problem is solved four ways to show that the defaults are a starting point, not a tuned recipe: Adam with
// its default learning rate of 0.001 for the default 200 iterations, the same for 5000 iterations, Adam with a learning
// rate of 0.1, and L-BFGS. Each row is the exit flag, iterations used, objective value, and the largest gradient
// component at the last point.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series start() { return pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64); }

int main() {
  auto objective = [](const pluss::Series& x) -> pluss::Series { return (x - 3.0).square().sum(); };

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(4);
  rows[0] = {"Adam, defaults", {}};
  rows[1] = {"Adam, defaults, 5000 iterations", {}};
  rows[1].options.max_iterations = 5000;
  rows[2] = {"Adam, learning rate 0.1", {}};
  rows[2].options.step.adam.learning_rate = 0.1;
  rows[3] = {"L-BFGS, defaults", {}};
  rows[3].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;

  std::cout << std::left << std::setw(34) << "setup" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(14)
            << "objective" << "largest gradient\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(objective, start(), row.options);
    std::cout << std::left << std::setw(34) << row.label << std::setw(16) << optim::septima_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(14) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
