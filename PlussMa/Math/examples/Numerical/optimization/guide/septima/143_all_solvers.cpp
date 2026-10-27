// One unconstrained problem, every solver in the library that can take it: the ten-variable chained Rosenbrock function.
// Optima (a sequential quadratic program with a BFGS curvature model) is the reference; Septima's Adam and L-BFGS and the L-SR1
// trust-region optimizer follow. The table gives the exit status, the iterations, the function evaluations, the objective
// reached and the size of the gradient at the end, which together say how much work each did and how good its answer is.
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

  std::cout << std::left << std::setw(16) << "solver" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  const auto row = [](const char* label, const char* exit, std::int64_t iterations, std::int64_t evaluations, double value, double gradient) {
    std::cout << std::left << std::setw(16) << label << std::setw(16) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::scientific
              << std::setprecision(1) << std::setw(12) << value << gradient << "\n";
  };

  {
    const auto r = optim::optima(chain, vec(start));
    row("Optima", r.exit_flag == optim::OptimaExitFlag::Converged ? "converged" : "other", r.iteration, r.function_evaluations, r.evaluation.value.item<double>(),
        r.output.first_order_optimality);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, Adam", r.converged ? "converged" : "MaxIterations", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, L-BFGS", r.converged ? "converged" : "NoProgress", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    row("L-SR1", r.converged ? "converged" : "not converged", r.iteration, r.function_evaluations, r.objective_value, r.gradient_norm);
  }
  return 0;
}
