// Two ways to turn several objectives into one. WeightedSum minimises sum_i w_i f_i. Chebyshev minimises the largest
// weighted gap to a reference point, max_i w_i (f_i - r_i). On a non-convex trade-off a weighted sum can only reach the
// end points of the front, never a compromise in the middle; Chebyshev can. Problem: f1 = x, f2 = 1 - x^2 on [0, 1], a
// non-convex front. With weights (0.6, 0.4) the weighted sum 0.6 x + 0.4 (1 - x^2) is concave, so its only minima are the two ends:
// x = 0 (value 0.4, the global one) and x = 1 (value 0.6, a local one); a local search ends at the end nearer its start, here
// x = 1 from the start 0.8. Chebyshev with reference (0, 0) and equal weights balances f1 = f2: x^2 + x - 1 = 0, x = 0.6180.
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
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0);
    e.objectives = a.reshape({1}).unite({(a.square() * -1.0 + 1.0).reshape({1})}, 0);
    return e;
  };
  struct Row { const char* label; dfo::MultiObjectiveScalarization s; };
  const Row rows[] = {{"WeightedSum", dfo::MultiObjectiveScalarization::WeightedSum}, {"Chebyshev", dfo::MultiObjectiveScalarization::Chebyshev}};
  std::cout << std::left << std::setw(14) << "scalarization" << std::setw(12) << "x" << std::setw(12) << "f1" << std::setw(12) << "f2" << "evals\n";
  for (const auto& r : rows) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.lower_bounds = vec({0.0});
    c.upper_bounds = vec({1.0});
    c.multiobjective_scalarization = r.s;
    c.objective_weights = (r.s == dfo::MultiObjectiveScalarization::WeightedSum) ? vec({0.6, 0.4}) : vec({1.0, 1.0});
    c.objective_reference = vec({0.0, 0.0});
    c.max_evaluations = 3000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({0.8}), c);
    const auto f = o.objective_vector.to_vec<double>();
    std::cout << std::left << std::setw(14) << r.label << std::fixed << std::setprecision(4) << std::setw(12) << o.minimizer.select(0, 0).item<double>()
              << std::setw(12) << f[0] << std::setw(12) << f[1] << o.diagnostics.evaluations << "\n";
  }
}
