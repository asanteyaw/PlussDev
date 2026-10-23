<!-- generated from the_self_adapting_variant.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The self-adapting variant

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
```

Output

```text
dim     method          status                    minimum     evals
3       DE              ReachedEvaluationBudget   1.52e-07    40000
3       DE-PRMM         ReachedIterationBudget    0.00e+00    39700
6       DE              ReachedEvaluationBudget   1.54e+01    40000
6       DE-PRMM         ReachedIterationBudget    0.00e+00    39700
```
`differential_evolution_prmm_minimizer` carries a mutation factor and a crossover rate per member, re-drawn at random with the probabilities `tau_mutation` and `tau_crossover` and kept in `[mutation_lower, mutation_upper]`. It protects the `elite_count` best members, chooses between two main strategies with `random_main_strategy_probability`, and reduces the population `reduction_passes` times as it converges. Its evaluation budget is `max_evaluations` (default 100000), and it derives its generation limit from that as `max_evaluations / (pmax × population_size)` (`pmax` defaults to 4), so it can end with `ReachedIterationBudget` just short of the evaluation budget, as in the table above.

On the 6-variable function plain differential evolution is far from the minimum after 40000 evaluations (about 15) and the self-adapting variant finds it exactly. When you do not know good values for the parameters, this is the variant to try first, and it is what `Auto` chooses for unbounded problems of medium size.
