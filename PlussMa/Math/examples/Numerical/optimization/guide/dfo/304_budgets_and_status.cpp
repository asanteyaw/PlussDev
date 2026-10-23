// Budgets and the status they produce. A solve ends in exactly one of five states. This program gives the same problem
// (the 2-D Rosenbrock valley) three different budgets and shows each ending: converged, out of evaluations, out of
// iterations. `evaluations` counts calls of the objective, which is the cost that matters when the objective is slow.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

void run(const char* label, dfo::DfoOptions o) {
  o.solver = dfo::DfoSolverKind::NelderMead;
  const auto r = dfo::solve_dfo(rosenbrock, vec({-1.2, 1.0}), o);
  std::cout << std::left << std::setw(26) << label << std::setw(26) << dfo::to_string(r.status) << std::scientific
            << std::setprecision(2) << std::setw(11) << r.minimum << std::defaultfloat << "evals " << std::setw(6)
            << r.diagnostics.evaluations << "iters " << r.diagnostics.iterations << "\n";
}

int main() {
  dfo::DfoOptions generous;
  run("default budgets", generous);

  dfo::DfoOptions few_evals;
  few_evals.max_evaluations = 60;
  run("max_evaluations = 60", few_evals);

  dfo::DfoOptions few_iters;
  few_iters.max_iterations = 25;
  run("max_iterations = 25", few_iters);

  dfo::DfoOptions loose;
  loose.relative_improvement_tolerance = 1e-3;
  run("tolerance 1e-3", loose);
}
