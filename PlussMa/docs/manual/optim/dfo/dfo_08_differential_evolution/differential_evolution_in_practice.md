<!-- generated from differential_evolution_in_practice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Differential evolution in practice

[Back to Derivative-Free Optimization 8: The Differential Evolution Minimisers](../dfo_08_differential_evolution.md)

Differential evolution keeps a **population** of candidate points and improves it by recombination: each member is challenged by a trial point built from the differences of other members, and the better of the two survives. It uses only function values, it covers the whole search region at once, and it is the method of choice when a function has many local minima and you can afford thousands of evaluations. Two standalone minimisers exist: `differential_evolution_minimizer`, the classic scheme, and `differential_evolution_prmm_minimizer`, a self-adapting variant that carries a mutation factor and a crossover rate per member.

The programs cover what a user decides. A direct call shows the global search from a start in a local minimum's basin. The parameters (population size, mutation factor, crossover rate, best-mix) are swept, with the warning that one seed moves the result as much as most rows differ. Seeds and execution modes show reproducibility and the spread across seeds. The initial radius sets where the population starts. Stopping shows the early-stop test. The self-adapting variant is the one to try when you do not know good parameter values.

## Program 1: A direct call

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

int main() {
  dfo::DifferentialEvolutionConfig c;
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 40000;
  const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
  const auto x = o.minimizer.host<double>();
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << dfo::to_string(o.status) << "\n";
  std::cout << "generations : " << o.diagnostics.iterations << "\nevaluations : " << o.diagnostics.evaluations << "\nlast step   : " << o.diagnostics.last_step << "\n";
}
```

Output

```text
minimizer   : -0.00002, 0.00000, -0.00002
minimum     : 1.52e-07
status      : ReachedEvaluationBudget
generations : 199
evaluations : 40000
last step   : generation
```
The search is global, so it is run in a box. The start (3, 3, 3) lies in a local minimum's basin; the population covers the whole box and finds the global minimum at the origin. The run used its whole evaluation budget: with the early-stop test off (`check_every` 0, the default), only the budgets end a run. Program 5 turns the test on.

## Program 2: The parameters

```cpp
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
```

Output

```text
defaults ReachedEvaluationBudget 1.78e-01 evals 20000
population_size 30 ReachedEvaluationBudget 0.00e+00 evals 20010
population_size 600 ReachedEvaluationBudget 2.12e+00 evals 20400
mutation_factor 0.4 ReachedEvaluationBudget 2.31e-01 evals 20000
mutation_factor 1.2 ReachedEvaluationBudget 2.65e+00 evals 20000
crossover_rate 0.2 ReachedEvaluationBudget 4.71e-07 evals 20000
best_mix 0.7 ReachedEvaluationBudget 6.04e-14 evals 20000
```
Read this table with care. Each row is one run, and Program 3 shows that the seed alone moves the result by more than most rows differ. What does carry over is how the settings interact with a fixed budget:

- **Population size and generations share the budget.** With 20000 evaluations, 600 members get about 33 generations and 30 members get about 660. On this three-variable problem the small population is the better use of the budget; on a harder problem it would collapse onto a local minimum for lack of diversity. Match the population to the dimension (5 to 10 members per variable is the usual minimum) and make the budget large enough for tens to hundreds of generations.
- **Mutation factor** scales the steps: too large (1.2) and the population keeps jumping past the minimum, too small and it collapses early.
- **Crossover rate** controls how many coordinates change per trial. A low rate suits separable functions such as Rastrigin, where each coordinate can be improved on its own; a high one suits functions where variables interact.
- **`best_mix`** builds mutants around the best member. It speeds convergence when the budget is tight (the 0.7 row is the best here) and raises the risk of collapsing on a local minimum when the function is rugged.

## Program 3: Seeds and execution modes

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
                << (mode == dfo::PopulationSolverExecutionMode::Vectorized ? "Vectorized" : "Sequential") << std::setw(26) << dfo::to_string(o.status)
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

## Program 4: Where the population starts

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

## Program 5: Stopping

```cpp
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
```

Output

```text
check_every 0 (test off) ReachedIterationBudget 0.00e+00 evals 16040 iterations 400
check_every 10, tolerance 1e-8 Converged 3.94e-27 evals 6440 iterations 160
check_every 10, tolerance 1e-3 Converged 1.40e-21 evals 5240 iterations 130
max_generations 15 ReachedIterationBudget 1.26e-03 evals 640 iterations 15
max_evaluations 500 ReachedEvaluationBudget 1.26e-03 evals 520 iterations 12
```
With the test off the run uses all 400 generations. With it on (every 10 generations, relative improvement below the tolerance, and the population collapsed to a small spread) the run stops at 160 generations on this unimodal function with the answer already at 1e-26. The test never fires on a population that is still spread out, which protects a multimodal search from stopping on a plateau. Note the statuses: a generation limit reports `ReachedIterationBudget` and an evaluation limit `ReachedEvaluationBudget`.

## Program 6: The self-adapting variant

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

int main() {
  std::cout << std::left << std::setw(8) << "dim" << std::setw(16) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  for (std::int64_t n : {3, 6}) {
    {
      dfo::DifferentialEvolutionConfig c;
      c.lower_bounds = filled(n, -5.12);
      c.upper_bounds = filled(n, 5.12);
      c.max_evaluations = 40000;
      const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(n, 3.0), c);
      std::cout << std::left << std::setw(8) << n << std::setw(16) << "DE" << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
                << std::setw(12) << o.minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
    }
    {
      dfo::DifferentialEvolutionPrmmConfig c;
      c.lower_bounds = filled(n, -5.12);
      c.upper_bounds = filled(n, 5.12);
      c.max_evaluations = 40000;
      const auto o = dfo::differential_evolution_prmm_minimizer(rastrigin, filled(n, 3.0), c);
      std::cout << std::left << std::setw(8) << n << std::setw(16) << "DE-PRMM" << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
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

## Decisions and pitfalls

- **Population size and generations share the budget.** With 20 000 evaluations, 600 members get about 33 generations and 30 members about 660; match the population to the difficulty (several members per variable) and to what you can afford.
- **Report the spread over seeds, not one run.** On a multimodal function different seeds end at values that differ as much as most parameter rows do; run several and keep the best, or report the spread.
- **The mutation factor scales the steps.** Too large and the population keeps jumping past the minimum; too small and it collapses early.
- **A low crossover rate suits separable functions; a high one suits interacting variables.**
- **`best_mix` speeds convergence when the budget is tight** and raises the risk of collapsing on a local minimum when the function is rugged.
- **Turn the early-stop test on only when the function is unimodal enough.** With `check_every` 0 (the default), only the budgets end a run; the test never fires on a population that is still spread out, and stops a run at 160 generations instead of 400 when it does.
- **Try the self-adapting variant when you cannot tune.** It is what `Auto` chooses for unbounded problems of medium size and found the minimum exactly where the plain method was far away.

Where next: [Chapter 9](../dfo_09_particle_swarm.md) describes the other population method.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
