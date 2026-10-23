<!-- generated from where_the_population_starts.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Where the population starts

[Back to The Differential Evolution Minimisers](../dfo_08_differential_evolution.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <string>
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
  std::cout << std::left << std::setw(16) << "initial_radius" << "minimum for seeds 1, 2, 3\n";
  for (double radius : {0.5, 2.0, std::numeric_limits<double>::infinity()}) {
    std::cout << std::left << std::setw(16) << (std::isfinite(radius) ? std::to_string(radius).substr(0, 3) : std::string("whole box"));
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::DifferentialEvolutionConfig c;
      c.lower_bounds = filled(3, -5.12);
      c.upper_bounds = filled(3, 5.12);
      c.max_evaluations = 20000;
      c.initial_radius = radius;  // infinity = the default: the whole box
      c.rng_seed = seed;
      const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
      std::cout << std::scientific << std::setprecision(2) << o.minimum << "   ";
    }
    std::cout << "\n";
  }
}
```

Output

```text
initial_radius  minimum for seeds 1, 2, 3
0.5             4.59e-01   8.11e-01   4.87e-01   
2.0             5.16e-01   8.12e-01   8.49e-01   
whole box       9.84e-01   8.31e-01   9.74e-01   
```
On this problem the starting region has no consistent effect, because the population of 200 spreads quickly either way. It matters more for the swarm (next chapter), and it is the only setting that defines the starting region when there are no bounds, so choose it there to cover where you expect the answer.
