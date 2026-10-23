<!-- generated from variable_neighbourhood_mads.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Variable-neighbourhood MADS

[Back to The MADS Variants](../dfo_12_mads_variants.md)


| `VnsMadsConfig` | Meaning | Default |
|---|---|---|
| `base` | the engine settings | |
| `neighborhoods` | number of stages (the first is plain MADS) | 5 |
| `initial_kick_scale` | scale of the first random perturbation | 0.2 |
| `kick_growth` | factor applied to the scale at each later stage | 1.7 |
| `rng_seed` | | 5489 |

Each later stage starts from the best point so far plus a random perturbation, runs MADS, and keeps the better outcome. The perturbations grow, so later stages reach further.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  const double pi = 3.14159265358979323846;
  auto blackbox = [pi](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum().reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(26) << "neighborhoods" << "minimum for seeds 1, 2, 3\n";
  for (std::int64_t stages : {1, 3, 6, 12}) {
    std::cout << std::left << std::setw(26) << stages;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::VnsMadsConfig c;
      c.base.lower_bounds = filled(2, -5.12);
      c.base.upper_bounds = filled(2, 5.12);
      c.base.max_evaluations = 9000;
      c.base.max_iterations = 5000;
      c.base.rng_seed = seed;
      c.rng_seed = seed;
      c.neighborhoods = stages;
      c.initial_kick_scale = 1.0;
      const auto o = dfo::vnsmads_blackbox_minimizer(blackbox, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.scalarized_minimum << "   ";
    }
    std::cout << "\n";
  }
}
```

Output

```text
neighborhoods             minimum for seeds 1, 2, 3
1                         9.95e-01   9.95e-01   9.95e-01   
3                         9.95e-01   9.95e-01   0.00e+00   
6                         9.95e-01   9.95e-01   0.00e+00   
12                        0.00e+00   9.95e-01   0.00e+00   
```
A single stage (plain MADS) settles in the nearest local minimum, value 0.995, on all three seeds. More stages find the global minimum on more seeds: with 12 stages, two of three. The budget is divided among stages, so more stages mean shorter runs each; on a harder problem the balance matters. Compare with the search step of Chapter 10 and the population methods of Chapters 8 and 9, which also address several basins.
