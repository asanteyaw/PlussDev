// An equality constraint, and how it is enforced. Be as close as possible to (2, 1) while the coordinates sum to 2: minimize
// 0.5 * |x - (2, 1)|^2 subject to x0 + x1 - 2 = 0. The answer is (1.5, 0.5) and the multiplier of the constraint is 0.5, because the
// objective's gradient there is (-0.5, -0.5), which is 0.5 times the gradient of the constraint. Septima enforces the equality with an
// augmented Lagrangian: a quadratic penalty with weight `equality_penalty`, plus a multiplier. The optimizer works on that augmented
// function; once it has solved it well enough, the multiplier is updated by the penalty times the violation, and if the violation did not
// shrink enough the penalty is raised. The table runs both optimizers with the library's defaults at three starting penalties, and then
// with the multiplier step overridden. `updates` is the number of multiplier updates, `penalty x` the factor by which the run raised the
// penalty, and `multiplier` the final estimate held in the result's state.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::optim;
using optim::Trace;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const Trace& x) -> Trace { return optim::sum(optim::square(x - optim::fixed(target))) * 0.5; };
  optim::SeptimaConstraints constraint;
  constraint.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 2.0}; };

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(9) << "penalty" << std::setw(11) << "dual step" << std::setw(15) << "exit" << std::setw(7) << "iters"
            << std::setw(9) << "updates" << std::setw(10) << "penalty x" << std::setw(12) << "violation" << std::setw(12) << "multiplier" << "x\n";
  struct Row { bool lbfgs; double penalty; double dual; };
  const Row rows[] = {{false, 1, 0}, {false, 10, 0}, {false, 100, 0}, {true, 1, 0}, {true, 10, 0}, {true, 100, 0}, {false, 10, 0.1}, {true, 10, 0.1}};
  for (const auto& row : rows) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 0.1;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.equality_penalty = row.penalty;
    options.step.dual_learning_rate = row.dual;  // 0 = the library default, the penalty itself
    const auto r = optim::septima(objective, vec({0.0, 0.0}), options, constraint);
    std::ostringstream x, dual;
    x << std::fixed << std::setprecision(3) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    if (row.dual > 0.0) dual << row.dual; else dual << "default";
    std::cout << std::left << std::setw(10) << (row.lbfgs ? "L-BFGS" : "Adam") << std::setw(9) << std::defaultfloat << row.penalty << std::setw(11) << dual.str()
              << std::setw(15) << (r.converged ? "Converged" : "MaxIterations") << std::setw(7) << r.iteration << std::setw(9) << r.state.dual_updates << std::setw(10)
              << r.state.penalty_scale << std::scientific << std::setprecision(1) << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(3)
              << std::setw(12) << r.state.lambda_equalities[0].item<double>() << x.str() << "\n";
  }
  return 0;
}
