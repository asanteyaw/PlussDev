// Differential evolution called directly. A population of candidate points is kept; each generation every member is
// challenged by a trial point built from the differences of other members (scaled by `mutation_factor`) and mixed
// with the member by `crossover_rate`, and the trial replaces the member if it is better. `differential_evolution_minimizer(
// objective, initial, config)` returns a PopulationSolverOutcome: minimiser, minimum, status, and diagnostics
// (iterations = generations, evaluations, last step). Here: the 3-D Rastrigin function, which has a grid of local minima
// around its global minimum 0 at the origin, searched in the box [-5.12, 5.12]^3 from the start (3, 3, 3).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}

int main() {
  dfo::DifferentialEvolutionConfig c;
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 40000;
  const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
  const auto x = o.minimizer.host<double>();
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << dfo::to_string(o.status) << "\n";
  std::cout << "generations : " << o.diagnostics.iterations << "\nevaluations : " << o.diagnostics.evaluations << "\nlast step   : " << o.diagnostics.last_step << "\n";
}
