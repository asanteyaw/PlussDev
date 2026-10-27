// Budgets and the status they produce. A solve ends in exactly one of five states. This program gives the same problem
// (the 2-D Rosenbrock valley) three different budgets and shows each ending: converged, out of evaluations, out of
// iterations. `evaluations` counts calls of the objective, which is the cost that matters when the objective is slow.
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::vector<std::pair<const char*, dfo::DfoOptions>> cases;
  dfo::DfoOptions generous;
  cases.emplace_back("default budgets", generous);

  dfo::DfoOptions few_evals;
  few_evals.max_evaluations = 60;
  cases.emplace_back("max_evaluations = 60", few_evals);

  dfo::DfoOptions few_iters;
  few_iters.max_iterations = 25;
  cases.emplace_back("max_iterations = 25", few_iters);

  dfo::DfoOptions loose;
  loose.relative_improvement_tolerance = 1e-3;
  cases.emplace_back("tolerance 1e-3", loose);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, o] : cases) {
    o.solver = dfo::DfoSolverKind::NelderMead;
    const auto r = dfo::solve_dfo(rosenbrock, vec({-1.2, 1.0}), o);
    out.writeln(label, dfo::to_string(r.status), r.minimum, "evals", r.diagnostics.evaluations, "iterations", r.diagnostics.iterations);
  }
}
