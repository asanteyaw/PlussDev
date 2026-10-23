// Three entry points, two poll schemes. The poll directions can be an orthogonal basis rotated at every iteration by
// a Halton-sequence reflection (OrthoMADS, `MadsPollScheme::Ortho2N`) or a random lower-triangular basis (LT-MADS,
// `MadsPollScheme::LtMadsRandom2N`). `orthomads_minimizer` and `ltmads_minimizer` are the same engine with the scheme
// chosen for you; `mesh_adaptive_direct_search_minimizer` takes the scheme from `config.poll_scheme`. Rosenbrock and a
// non-smooth function (the sum of absolute deviations), same budget.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
void row(const char* label, const dfo::BlackboxSolverOutcome& o) {
  std::cout << "  " << std::left << std::setw(34) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  auto kink = [](const pluss::Series& x) { return (x - vec({1.0, -2.0, 0.5, 3.0})).abs().sum(); };
  dfo::MadsBlackboxConfig c;
  c.max_iterations = 4000;
  c.max_evaluations = 40000;
  std::cout << "Rosenbrock\n";
  row("orthomads_minimizer", dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c));
  row("ltmads_minimizer", dfo::ltmads_minimizer(rosenbrock, vec({-1.2, 1.0}), c));
  auto lt = c; lt.poll_scheme = dfo::MadsPollScheme::LtMadsRandom2N;
  row("mesh_adaptive..., LtMadsRandom2N", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}), lt));
  std::cout << "sum |x - c|, 4 variables\n";
  row("orthomads_minimizer", dfo::orthomads_minimizer(kink, vec({0, 0, 0, 0}), c));
  row("ltmads_minimizer", dfo::ltmads_minimizer(kink, vec({0, 0, 0, 0}), c));
}
