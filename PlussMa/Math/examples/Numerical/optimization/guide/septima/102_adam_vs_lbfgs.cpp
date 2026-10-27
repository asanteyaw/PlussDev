// The two optimizers on a curved valley. The Rosenbrock function (1 - x)^2 + 100 (y - x^2)^2 has its minimum at (1, 1)
// at the end of a long, narrow, curved valley, started from (-1.2, 1). Adam follows the valley with steps whose size is
// set by the learning rate; L-BFGS builds a picture of the curvature from recent steps and takes longer ones. The table
// reports iterations, function evaluations (a line search spends several per iteration), the objective reached and the
// point. Exit flags are listed too, because a run can end with `NoProgress` after reaching a very good point (see the
// chapter).
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<pluss::Series(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  };
  const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(3);
  rows[0] = {"Adam, learning rate 0.02", {}};
  rows[0].options.step.adam.learning_rate = 0.02;
  rows[1] = {"L-BFGS, no line search", {}};
  rows[1].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2] = {"L-BFGS, strong Wolfe", {}};
  rows[2].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2].options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  for (auto& row : rows) row.options.max_iterations = 3000;

  std::cout << std::left << std::setw(28) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals"
            << std::setw(12) << "objective" << "x, y\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(rosenbrock, start, row.options);
    std::cout << std::left << std::setw(28) << row.label << std::setw(14) << optim::septima_exit_flag_name(r.exit_flag) << std::setw(8) << r.iteration
              << std::setw(8) << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value
              << std::fixed << std::setprecision(5) << as_f64(r.params[0]) << ", " << as_f64(r.params[1]) << "\n";
  }
  return 0;
}
