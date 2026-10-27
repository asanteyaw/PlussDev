// Seeds and execution modes. The population is drawn from `rng_seed`, so a run is repeatable: the same seed gives the same
// answer. Different seeds give different answers; on a multimodal function like Rastrigin the spread across seeds is
// often larger than the effect of any single setting, which is why the rows of the previous program are only illustrative.
// `execution_mode` chooses how a generation is computed: Vectorized evaluates the whole population as array operations (the
// default); the other mode updates members one after another, in the order used by
// the classic formulation, and gives a different (equally valid) random path.
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
  std::cout << std::left << std::setw(8) << "seed" << std::setw(18) << "mode" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  for (auto mode : {dfo::PopulationSolverExecutionMode::Vectorized, dfo::PopulationSolverExecutionMode::OptimMasterCompat}) {
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL, 1ULL}) {
      dfo::DifferentialEvolutionConfig c;
      c.lower_bounds = filled(3, -5.12);
      c.upper_bounds = filled(3, 5.12);
      c.max_evaluations = 20000;
      c.rng_seed = seed;
      c.execution_mode = mode;
      const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
      std::cout << std::left << std::setw(8) << seed << std::setw(18)
                << (mode == dfo::PopulationSolverExecutionMode::Vectorized ? "Vectorized" : "Sequential") << std::setw(26) << dfo::to_string(o.status)
                << std::scientific << std::setprecision(2) << std::setw(12) << o.minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
    }
  }
}
