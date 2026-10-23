// Partial-space decomposition MADS (`psdmads_blackbox_minimizer`). Like cooperative MADS, it optimises subsets of the variables
// with the rest held fixed, but the subsets are random, of random size between `min_subproblem_size` and
// `max_subproblem_size`, and `subproblems_per_round` of them are drawn in each of `rounds` rounds. Because subsets overlap and
// vary, variables that interact are optimised together some of the time. Problem: a coupled chain
// sum_i (x_i - x_{i+1})^2 + (x_0 - 1)^2 in 16 variables (minimum 0 at all ones), 12000 evaluations in every row.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  const std::int64_t n = 16;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.slice(0, 0, n - 1, 1), b = x.slice(0, 1, n, 1);
    e.objectives = ((a - b).square().sum() + (x.select(0, 0) - 1.0).square()).reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(38) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  {
    dfo::MadsBlackboxAdvancedConfig c;
    c.max_evaluations = 12000;
    c.max_iterations = 5000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(38) << "plain MADS" << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
  struct Row { const char* label; std::int64_t lo, hi, per, rounds; };
  const Row rows[] = {{"psd, subsets of 2..4, 3 per round", 2, 4, 3, 5}, {"psd, subsets of 4..8, 3 per round", 4, 8, 3, 5}, {"psd, subsets of 8..12, 3 per round", 8, 12, 3, 5}};
  for (const auto& r : rows) {
    dfo::PsdMadsConfig c;
    c.base.max_evaluations = 12000;
    c.base.max_iterations = 5000;
    c.min_subproblem_size = r.lo;
    c.max_subproblem_size = r.hi;
    c.subproblems_per_round = r.per;
    c.rounds = r.rounds;
    const auto o = dfo::psdmads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(38) << r.label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
}
