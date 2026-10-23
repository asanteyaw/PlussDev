// DE-PRMM. The same idea as plain differential evolution with adaptive rates and a small elite. Each member carries its
// own mutation factor and crossover rate, which are re-drawn with probability controlled by `tau_mutation` and
// `tau_crossover` (kept inside [mutation_lower, mutation_upper]); `elite_count` best members are protected, `pmax`
// bounds the mutation strategy's best-member pool, `random_main_strategy_probability` chooses between its two main
// strategies and `reduction_passes` is the number of times the population is reduced near the end. Nothing needs tuning
// for a first run. Here it is compared with plain DE at the same budget on the 3-D and 6-D Rastrigin functions.
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
  std::cout << std::left << std::setw(8) << "dim" << std::setw(16) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  for (std::int64_t n : {3, 6}) {
    {
      dfo::DifferentialEvolutionConfig c;
      c.lower_bounds = filled(n, -5.12);
      c.upper_bounds = filled(n, 5.12);
      c.max_evaluations = 40000;
      const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(n, 3.0), c);
      std::cout << std::left << std::setw(8) << n << std::setw(16) << "DE" << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2)
                << std::setw(12) << o.minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
    }
    {
      dfo::DifferentialEvolutionPrmmConfig c;
      c.lower_bounds = filled(n, -5.12);
      c.upper_bounds = filled(n, 5.12);
      c.max_evaluations = 40000;
      const auto o = dfo::differential_evolution_prmm_minimizer(rastrigin, filled(n, 3.0), c);
      std::cout << std::left << std::setw(8) << n << std::setw(16) << "DE-PRMM" << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2)
                << std::setw(12) << o.minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
    }
  }
}
