<!-- generated from the_parameters.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The parameters

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
```

Output

```text
defaults                    ReachedEvaluationBudget   1.78e-01   evals 20000
population_size 30          ReachedEvaluationBudget   0.00e+00   evals 20010
population_size 600         ReachedEvaluationBudget   2.12e+00   evals 20400
mutation_factor 0.4         ReachedEvaluationBudget   2.31e-01   evals 20000
mutation_factor 1.2         ReachedEvaluationBudget   2.65e+00   evals 20000
crossover_rate 0.2          ReachedEvaluationBudget   4.71e-07   evals 20000
best_mix 0.7                ReachedEvaluationBudget   6.04e-14   evals 20000
```
Read this table with care. Each row is one run, and Program 3 shows that the seed alone moves the result by more than most rows differ. What does carry over is how the settings interact with a fixed budget:

- **Population size and generations share the budget.** With 20000 evaluations, 600 members get about 33 generations and 30 members get about 660. On this three-variable problem the small population is the better use of the budget; on a harder problem it would collapse onto a local minimum for lack of diversity. Match the population to the dimension (5 to 10 members per variable is the usual minimum) and make the budget large enough for tens to hundreds of generations.
- **Mutation factor** scales the steps: too large (1.2) and the population keeps jumping past the minimum, too small and it collapses early.
- **Crossover rate** controls how many coordinates change per trial. A low rate suits separable functions such as Rastrigin, where each coordinate can be improved on its own; a high one suits functions where variables interact.
- **`best_mix`** builds mutants around the best member. It speeds convergence when the budget is tight (the 0.7 row is the best here) and raises the risk of collapsing on a local minimum when the function is rugged.
