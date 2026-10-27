// What stops a run, and why asking for more accuracy than the method can deliver changes the exit flag, not the answer.
// The chained Rosenbrock function of the previous program is solved with L-BFGS and three gradient tolerances. A run is
// converged when the gradient, the step and the constraint violation are all within tolerance; with no constraints that is
// the first two. When the gradient cannot get below a tolerance (L-BFGS without a line search stalls at a gradient of a few
// 1e-4 here), the run ends with `NoProgress` after `max_no_progress_iterations` flat iterations, at the same point it would
// have reached anyway. The second block shows the conditioning of the problem: a quadratic whose curvature differs by a
// factor of 10^4 between its two variables. Adam rescales each coordinate by the size of its own gradients, so a stretch along the
// axes costs it little; L-BFGS learns the curvature itself and needs five iterations.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const pluss::Series& x) -> pluss::Series {
    const pluss::Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };

  std::cout << std::left << std::setw(20) << "gradient tolerance" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(12) << "gradient" << "objective\n";
  for (const double tolerance : {1e-3, 1e-6, 1e-9}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.gradient_tolerance = tolerance;
    options.step_tolerance = 1e-8;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(20) << std::scientific << std::setprecision(0) << tolerance << std::setw(14)
              << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setprecision(1) << std::setw(12) << r.gradient_inf_norm
              << r.objective_value << "\n";
  }

  std::cout << "\nconditioning: 0.5 * (x0^2 + 10000 x1^2) from (1, 1)\n";
  auto stretched = [](const pluss::Series& x) -> pluss::Series { return (vec({1.0, 1e4}) * x.square()).sum() * 0.5; };
  struct Row { const char* label; bool lbfgs; double rate; };
  for (const Row row : {Row{"Adam, learning rate 0.01", false, 0.01}, Row{"Adam, learning rate 0.1", false, 0.1}, Row{"L-BFGS", true, 1.0}}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    else options.step.adam.learning_rate = row.rate;
    const auto r = optim::septima(stretched, vec({1.0, 1.0}), options);
    std::cout << "  " << std::left << std::setw(26) << row.label << (r.converged ? "Converged" : "not converged") << "  iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
