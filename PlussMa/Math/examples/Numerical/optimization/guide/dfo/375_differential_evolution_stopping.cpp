// Stopping. Three things end a run: the generation limit (`max_generations`), the evaluation limit (`max_evaluations`),
// and the improvement test: every `check_every` generations, if the best value has improved by less than
// `relative_improvement_tolerance` (relatively) AND the population has collapsed (its spread is small), the run is declared
// Converged. Requiring the spread as well keeps a population that is still exploring from stopping on a lucky plateau.
// With check_every = 0 the test is off and only the budgets apply.
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}

int main() {
  std::vector<std::pair<const char*, dfo::DifferentialEvolutionConfig>> cases;
  dfo::DifferentialEvolutionConfig base;
  base.population_size = 40;
  base.max_generations = 400;
  cases.emplace_back("check_every 0 (test off)", base);
  auto every10 = base; every10.check_every = 10;
  cases.emplace_back("check_every 10, tolerance 1e-8", every10);
  auto loose = every10; loose.relative_improvement_tolerance = 1e-3;
  cases.emplace_back("check_every 10, tolerance 1e-3", loose);
  auto few = base; few.max_generations = 15;
  cases.emplace_back("max_generations 15", few);
  auto ev = base; ev.max_evaluations = 500;
  cases.emplace_back("max_evaluations 500", ev);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.lower_bounds = filled(2, -5.12);
    c.upper_bounds = filled(2, 5.12);
    auto sphere = [](const pluss::Series& x) { return x.square().sum(); };
    const auto o = dfo::differential_evolution_minimizer(sphere, filled(2, 3.0), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations, "iterations", o.diagnostics.iterations);
  }
}
