// The black-box minimiser called directly. `mads_blackbox_minimizer(blackbox, initial, config)` takes a function returning a
// BlackboxEvaluation (objectives, constraints, evaluation_ok, count_evaluation) and returns a BlackboxAdvancedOutcome:
// minimiser, objective_vector (all objectives at the minimiser), scalarized_minimum, status, the Pareto archive
// (`pareto_points`, `pareto_objectives`) and diagnostics. Problem: two objectives f1 = (x0-1)^2 + x1^2 and f2 = (x0+1)^2 + x1^2
// whose compromises lie on the segment x1 = 0, -1 <= x0 <= 1; one constraint x0 <= 0.5, written x0 - 0.5 <= 0.
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
  dfo::MadsBlackboxAdvancedConfig c;
  c.constraint_handling = dfo::ConstraintHandling::ExtremeBarrier;
  c.max_evaluations = 2000;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0), b = x.select(0, 1);
    e.objectives = ((a - 1.0).square() + b.square()).reshape({1}).unite({((a + 1.0).square() + b.square()).reshape({1})}, 0);
    e.constraints = (a - 0.5).reshape({1});
    return e;
  };
  const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({-0.5, 0.5}), c);
  const auto f = o.objective_vector.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "minimizer          : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objectives         : " << f[0] << ", " << f[1] << "\n";
  std::cout << "scalarized minimum : " << o.scalarized_minimum << "  (equal weights)\n";
  std::cout << "status             : " << name(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "evaluations        : " << d.evaluations << " (total " << d.total_evaluations << ", cache hits " << d.cache_hits << ")\n";
  std::cout << "objective / constraint dimension: " << d.objective_dimension << " / " << d.constraint_dimension << "\n";
  std::cout << "Pareto archive     : " << d.pareto_size << " points, " << o.pareto_points.size(0) << " x " << o.pareto_points.size(1) << " matrix\n";
}
