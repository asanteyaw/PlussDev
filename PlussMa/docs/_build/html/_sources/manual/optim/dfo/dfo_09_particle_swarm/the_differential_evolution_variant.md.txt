<!-- generated from the_differential_evolution_variant.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The differential-evolution variant

[Back to The Particle Swarm Minimisers](../dfo_09_particle_swarm.md)


```cpp
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
```

Output

```text
dim   method        minimum for seeds 1, 2, 3
3     swarm         0.00e+00   0.00e+00   0.00e+00   
3     swarm-DV      3.59e-01   3.69e-01   7.13e-02   
6     swarm         9.95e-01   0.00e+00   1.28e-09   
6     swarm-DV      1.39e+01   1.80e+01   1.25e+01   
```
At 3 variables the classic swarm finds the global minimum exactly on every seed and the variant ends within 0.4 of it. At 6 variables, with the same 40000 evaluations, the classic swarm reaches essentially zero on two seeds and about 1 on the third, while the variant stops between 12 and 18. The variant moves each particle only when the move improves it and has no memory of a particle's own best position, so it behaves like differential evolution (Chapter 8, where plain DE also stops near 15 on this problem) rather than like a swarm. The classic swarm is the better first choice on rugged functions; try the variant as a second opinion.
