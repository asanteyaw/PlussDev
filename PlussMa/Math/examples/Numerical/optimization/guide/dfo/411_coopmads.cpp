// Cooperative MADS (`coopmads_blackbox_minimizer`) for many variables. Plain MADS polls 2n directions around the best point, so
// its cost per iteration grows with the number of variables. Cooperative MADS splits the variables into blocks, runs MADS on
// one block at a time with the other variables held at their current values, and repeats for `cooperative_rounds` rounds.
// `subproblem_size` is the block size (0 = chosen from `subproblems_per_round`), `randomized_subproblems` reshuffles the
// blocks every round, and the evaluation budget of `base` is divided among all blocks and rounds. Problem: a separable
// quadratic sum_i (x_i - 1)^2 in 24 variables from (-1, ..., -1), 12000 evaluations in every row.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  const std::int64_t n = 24;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 1.0).square().sum().reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(30) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  {
    dfo::MadsBlackboxAdvancedConfig c;
    c.max_evaluations = 12000;
    c.max_iterations = 5000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(30) << "plain MADS" << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
  for (std::int64_t block : {3, 6, 12}) {
    dfo::CoopMadsConfig c;
    c.base.max_evaluations = 12000;
    c.base.max_iterations = 5000;
    c.subproblem_size = block;
    const auto o = dfo::coopmads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(30) << ("coop, block size " + std::to_string(block)) << std::setw(26) << dfo::to_string(o.status) << std::scientific
              << std::setprecision(2) << std::setw(12) << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
}
