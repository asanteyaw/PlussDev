// Stopping. Three things end a run: the generation limit (`max_generations`), the evaluation limit (`max_evaluations`),
// and the improvement test: every `check_every` generations, if the best value has improved by less than
// `relative_improvement_tolerance` (relatively) AND the population has collapsed (its spread is small), the run is declared
// Converged. Requiring the spread as well keeps a population that is still exploring from stopping on a lucky plateau.
// With check_every = 0 the test is off and only the budgets apply.
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

void run(const char* label, dfo::DifferentialEvolutionConfig c) {
  c.lower_bounds = filled(2, -5.12);
  c.upper_bounds = filled(2, 5.12);
  auto sphere = [](const pluss::Series& x) { return x.square().sum(); };
  const auto o = dfo::differential_evolution_minimizer(sphere, filled(2, 3.0), c);
  std::cout << std::left << std::setw(36) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << "generations " << o.diagnostics.iterations << "\n";
}

int main() {
  dfo::DifferentialEvolutionConfig base;
  base.population_size = 40;
  base.max_generations = 400;
  run("check_every 0 (test off)", base);
  auto every10 = base; every10.check_every = 10;
  run("check_every 10, tolerance 1e-8", every10);
  auto loose = every10; loose.relative_improvement_tolerance = 1e-3;
  run("check_every 10, tolerance 1e-3", loose);
  auto few = base; few.max_generations = 15;
  run("max_generations 15", few);
  auto ev = base; ev.max_evaluations = 500;
  run("max_evaluations 500", ev);
}
