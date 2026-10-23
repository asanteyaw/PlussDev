// The knobs of `DifferentialEvolutionConfig`, one at a time, on the 3-D Rastrigin function in [-5.12, 5.12]^3 with a budget
// of 20000 evaluations. Each row changes a single setting from the default and shows the minimum reached and the
// evaluations used. One run per row, fixed seed: read the rows as an illustration, and see the next program for how much
// a seed alone changes the result.
//   population_size    members kept (default 200)
//   mutation_factor    scale of the difference vectors, F (default 0.8)
//   crossover_rate     probability that a coordinate comes from the trial, CR (default 0.9)
//   best_mix           0 = trial built around a random member; toward 1 = built around the best member (default 0)
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
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 20000;
  const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
  std::cout << std::left << std::setw(28) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::DifferentialEvolutionConfig base;
  run("defaults", base);
  auto small = base; small.population_size = 30;
  run("population_size 30", small);
  auto big = base; big.population_size = 600;
  run("population_size 600", big);
  auto lowf = base; lowf.mutation_factor = 0.4;
  run("mutation_factor 0.4", lowf);
  auto highf = base; highf.mutation_factor = 1.2;
  run("mutation_factor 1.2", highf);
  auto lowcr = base; lowcr.crossover_rate = 0.2;
  run("crossover_rate 0.2", lowcr);
  auto greedy = base; greedy.best_mix = 0.7;
  run("best_mix 0.7", greedy);
}
