// Particle swarm called directly. A swarm of particles moves through the box; each particle remembers its own best
// position and is pulled toward it (`cognitive`) and toward the best position any particle has found (`social`), with a
// momentum (`inertia`) that by default falls linearly from `inertia_max` to `inertia_min` over the run. `particle_swarm_minimizer(
// objective, initial, config)` returns a PopulationSolverOutcome. Here: the 3-D Rastrigin function in [-5.12, 5.12]^3
// from the start (3, 3, 3).
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
const char* name(dfo::PopulationSolverStatus s) {
  return s == dfo::PopulationSolverStatus::Converged ? "Converged"
         : s == dfo::PopulationSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}

int main() {
  dfo::ParticleSwarmConfig c;
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 20000;
  const auto o = dfo::particle_swarm_minimizer(rastrigin, filled(3, 3.0), c);
  const auto x = o.minimizer.to_vec<double>();
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << name(o.status) << "\n";
  std::cout << "generations : " << o.diagnostics.iterations << "\nevaluations : " << o.diagnostics.evaluations << "\nlast step   : " << o.diagnostics.last_step << "\n";
}
