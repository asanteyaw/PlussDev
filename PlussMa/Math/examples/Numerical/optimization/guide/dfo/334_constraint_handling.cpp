// How a black box reports constraints, and what the solver does with them. The black box returns `constraints` (feasible
// when every entry is <= 0) next to the objectives. `constraint_handling` chooses the policy:
//   Ignore             the constraint values are recorded but never used (the default for a plain black box);
//   ExtremeBarrier     a point that violates any constraint is thrown away;
//   ProgressiveBarrier a violating point is accepted at a penalty `progressive_barrier_penalty` per unit of violation, so
//                      the search can cross infeasible ground. The penalty must outweigh the objective's pull toward
//                      the infeasible side, or the search will settle outside the feasible set.
// Problem: minimise (x0-2)^2 + (x1-2)^2 inside the unit disc. The exact answer is 3.3431 on the boundary.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  struct Mode { const char* name; dfo::ConstraintHandling h; double penalty; };
  const Mode modes[] = {{"Ignore", dfo::ConstraintHandling::Ignore, 100.0},
                        {"ExtremeBarrier", dfo::ConstraintHandling::ExtremeBarrier, 100.0},
                        {"Progressive, 100", dfo::ConstraintHandling::ProgressiveBarrier, 100.0},
                        {"Progressive, 1", dfo::ConstraintHandling::ProgressiveBarrier, 1.0}};
  std::cout << std::left << std::setw(20) << "handling" << std::setw(12) << "objective" << std::setw(14) << "violation" << "evals\n";
  for (const auto& m : modes) {
    dfo::DfoProblem p;
    p.initial = vec({0.0, 0.0});
    p.blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({2.0, 2.0})).square().sum().reshape({1});
      e.constraints = (x.square().sum() - 1.0).reshape({1});
      return e;
    };
    dfo::DfoOptions o;
    o.mads.constraint_handling = m.h;
    o.mads.progressive_barrier_penalty = m.penalty;
    const auto r = dfo::DFO::solve(p, o);
    const auto x = r.minimizer.host<double>();
    const double violation = std::max(0.0, x[0] * x[0] + x[1] * x[1] - 1.0);
    std::cout << std::left << std::setw(20) << m.name << std::fixed << std::setprecision(4) << std::setw(12) << r.minimum
              << std::scientific << std::setprecision(1) << std::setw(14) << violation << std::defaultfloat
              << r.diagnostics.evaluations << "\n";
  }
}
