<!-- generated from seeds_and_execution_modes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Seeds and execution modes

[Back to The Differential Evolution Minimisers](../dfo_08_differential_evolution.md)


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
                << (mode == dfo::PopulationSolverExecutionMode::Vectorized ? "Vectorized" : "Sequential") << std::setw(26) << name(o.status)
                << std::scientific << std::setprecision(2) << std::setw(12) << o.minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
    }
  }
}
```

Output

```text
seed    mode              status                    minimum     evals
1       Vectorized        ReachedEvaluationBudget   9.84e-01    20000
2       Vectorized        ReachedEvaluationBudget   8.31e-01    20000
3       Vectorized        ReachedEvaluationBudget   9.74e-01    20000
1       Vectorized        ReachedEvaluationBudget   9.84e-01    20000
1       Sequential        ReachedEvaluationBudget   1.08e+00    20000
2       Sequential        ReachedEvaluationBudget   3.54e-02    20000
3       Sequential        ReachedEvaluationBudget   4.01e-01    20000
1       Sequential        ReachedEvaluationBudget   1.08e+00    20000
```
The same seed gives the same result (rows 1 and 4 of each mode agree exactly). Different seeds end at different values, from 0.04 to 1.1 in this run, a spread comparable to the whole of the previous table. For a multimodal function, report the spread over several seeds, or run several and keep the best. The sequential mode updates members one after another in the classic order; it gives a different random path, equally valid. The vectorized mode is the default.
