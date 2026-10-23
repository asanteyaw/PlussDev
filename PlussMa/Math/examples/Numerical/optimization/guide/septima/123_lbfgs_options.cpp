// The L-BFGS settings, on a ten-variable chained Rosenbrock function (every neighbouring pair of variables forms a
// Rosenbrock valley). The table varies the history size, the number of recent step pairs the method remembers, with and
// without the strong-Wolfe line search, and reports iterations and function evaluations. The last block varies the
// learning rate, which scales every step when there is no line search.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

const char* name_of(optim::SeptimaExitFlag flag) {
  switch (flag) {
    case optim::SeptimaExitFlag::Converged: return "Converged";
    case optim::SeptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::SeptimaExitFlag::Stopped: return "Stopped";
    case optim::SeptimaExitFlag::NoProgress: return "NoProgress";
    case optim::SeptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::SeptimaExitFlag::InvalidProblem: return "InvalidProblem";
  }
  return "?";
}

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const Trace& x) -> Trace {
    const Trace a = pluss::subsp::math::diff::index_select(x, 0, head), b = pluss::subsp::math::diff::index_select(x, 0, tail);
    return optim::sum(100.0 * optim::square(b - optim::square(a)) + optim::square(1.0 - a));
  };

  std::cout << std::left << std::setw(32) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  for (const bool wolfe : {false, true}) {
    for (const std::int64_t history : {1, 3, 10, 30}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 2000;
      options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
      options.step.lbfgs.history_size = history;
      if (wolfe) options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
      const auto r = optim::septima(chain, vec(start), options);
      std::cout << std::left << std::setw(32) << ((wolfe ? "strong Wolfe, history " : "no line search, history ") + std::to_string(history))
                << std::setw(14) << name_of(r.exit_flag) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations << std::scientific
                << std::setprecision(1) << r.objective_value << "\n";
    }
  }
  std::cout << "\nlearning rate, no line search, default history (20)\n";
  for (const double rate : {0.1, 0.5, 1.0, 2.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.learning_rate = rate;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << "  " << std::left << std::setw(8) << std::defaultfloat << rate << std::setw(22) << name_of(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
