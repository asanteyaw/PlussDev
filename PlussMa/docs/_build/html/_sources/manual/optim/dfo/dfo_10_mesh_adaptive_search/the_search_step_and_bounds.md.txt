<!-- generated from the_search_step_and_bounds.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The search step and bounds

[Back to Mesh Adaptive Direct Search](../dfo_10_mesh_adaptive_search.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(20) << "search_points" << "minimum for seeds 1, 2, 3 (evaluations in brackets)\n";
  for (std::int64_t points : {0, 8, 32}) {
    std::cout << std::left << std::setw(20) << points;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::MadsBlackboxConfig c;
      c.lower_bounds = filled(2, -5.12);
      c.upper_bounds = filled(2, 5.12);
      c.search_points = points;
      c.rng_seed = seed;
      c.max_iterations = 4000;
      c.max_evaluations = 20000;
      const auto o = dfo::orthomads_minimizer(rastrigin, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.minimum << " (" << std::defaultfloat << o.diagnostics.evaluations << ")   ";
    }
    std::cout << "\n";
  }
}
```

Output

```text
search_points       minimum for seeds 1, 2, 3 (evaluations in brackets)
0                   9.95e-01 (355)   9.95e-01 (355)   9.95e-01 (355)   
8                   9.95e-01 (1007)   9.95e-01 (858)   0.00e+00 (1209)   
32                  9.95e-01 (3559)   0.00e+00 (3691)   0.00e+00 (3140)   
```
Without a search step the method creeps from its start into the nearest local minimum (value 0.995) on every seed. The search step tries points across the frame, so it sometimes lands in a better basin: with 8 points one seed in three finds the global minimum, with 32 two in three. The price is evaluations, roughly three times as many for 8 points and ten times for 32. Use it when a local minimum is not good enough and the budget allows. With bounds, every trial point stays inside the box.
