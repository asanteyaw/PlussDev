// The knobs of `DifferentialEvolutionConfig`, one at a time, on the 3-D Rastrigin function in [-5.12, 5.12]^3 with a budget
// of 20000 evaluations. Each row changes a single setting from the default and shows the minimum reached and the
// evaluations used. One run per row, fixed seed: read the rows as an illustration, and see the next program for how much
// a seed alone changes the result.
//   population_size    members kept (default 200)
//   mutation_factor    scale of the difference vectors, F (default 0.8)
//   crossover_rate     probability that a coordinate comes from the trial, CR (default 0.9)
//   best_mix           0 = trial built around a random member; toward 1 = built around the best member (default 0)
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  cases.emplace_back("defaults", base);
  auto small = base; small.population_size = 30;
  cases.emplace_back("population_size 30", small);
  auto big = base; big.population_size = 600;
  cases.emplace_back("population_size 600", big);
  auto lowf = base; lowf.mutation_factor = 0.4;
  cases.emplace_back("mutation_factor 0.4", lowf);
  auto highf = base; highf.mutation_factor = 1.2;
  cases.emplace_back("mutation_factor 1.2", highf);
  auto lowcr = base; lowcr.crossover_rate = 0.2;
  cases.emplace_back("crossover_rate 0.2", lowcr);
  auto greedy = base; greedy.best_mix = 0.7;
  cases.emplace_back("best_mix 0.7", greedy);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.lower_bounds = filled(3, -5.12);
    c.upper_bounds = filled(3, 5.12);
    c.max_evaluations = 20000;
    const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
