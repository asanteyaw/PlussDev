// The second swarm, `particle_swarm_dv_minimizer`, mixes particle swarm with differential evolution: particles also exchange
// coordinates by crossover (`crossover_rate`), the difference between particles enters the update through `beta`, the
// inertia decays by `inertia_damp` each generation, and a particle that has not improved for `stagnation_limit` generations
// is re-seeded. Compared with the plain swarm on the 3-D and 6-D Rastrigin functions at the same budget (40000 evaluations,
// three seeds).
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
  std::cout << std::left << std::setw(6) << "dim" << std::setw(14) << "method" << "minimum for seeds 1, 2, 3\n";
  for (std::int64_t n : {3, 6}) {
    for (int which : {0, 1}) {
      std::cout << std::left << std::setw(6) << n << std::setw(14) << (which == 0 ? "swarm" : "swarm-DV");
      for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
        double minimum = 0.0;
        if (which == 0) {
          dfo::ParticleSwarmConfig c;
          c.lower_bounds = filled(n, -5.12); c.upper_bounds = filled(n, 5.12); c.max_evaluations = 40000; c.rng_seed = seed;
          minimum = dfo::particle_swarm_minimizer(rastrigin, filled(n, 3.0), c).minimum;
        } else {
          dfo::ParticleSwarmDvConfig c;
          c.lower_bounds = filled(n, -5.12); c.upper_bounds = filled(n, 5.12); c.max_evaluations = 40000; c.rng_seed = seed;
          minimum = dfo::particle_swarm_dv_minimizer(rastrigin, filled(n, 3.0), c).minimum;
        }
        std::cout << std::scientific << std::setprecision(2) << minimum << "   ";
      }
      std::cout << "\n";
    }
  }
}
