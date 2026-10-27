<!-- generated from first_solves.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# First derivative-free solves

[Back to Derivative-Free Optimization 1: First Problems](../dfo_01_first_problems.md)

A derivative-free method needs only the *value* of your function: no gradient, no Jacobian, nothing that requires the function to be smooth. That makes it the tool for objectives that are the output of a simulation, a table lookup, a rule-based model or any code you cannot differentiate. The price is more evaluations, and answers that are accurate to a tolerance rather than certified by an optimality condition.

`solve_dfo(objective, start)` is the shortest route into the family. This page shows the three equivalent ways to call it (the shortcut, a `DfoProblem` with `DfoOptions` filled field by field, and a chained builder), and explains what the default `Auto` choice of method does: it looks at the *problem* (are there bounds, constraints, how many variables, is an evaluation function supplied), never at the objective values, and `selection_reason` always states which rule fired.

## Program 1: A first solve

`solve_dfo(objective, start)` is the shortest route. The objective may return a plain number or a Series.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  // f(x) = (x0 - 1)^2 + 2 (x1 + 2)^2 + 3 (x2 - 0.5)^2, minimum 0 at (1, -2, 0.5).
  auto objective = [](const pluss::Series& x) {
    const auto c = vec({1.0, -2.0, 0.5});
    const auto w = vec({1.0, 2.0, 3.0});
    return ((x - c).square() * w).sum();
  };
  const auto r = dfo::solve_dfo(objective, vec({2.5, -1.5, 3.0}));

  std::cout << "selected solver : " << dfo::to_string(r.selected_solver) << "\n";
  std::cout << "reason          : " << r.selection_reason << "\n";
  std::cout << "status          : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer       : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << ", " << r.minimizer.select(0, 2).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum         : " << r.minimum << "\n";
  std::cout << "evaluations     : " << r.diagnostics.evaluations << "\n";
  std::cout << "iterations      : " << r.diagnostics.iterations << "\n";
}
```

Output

```text
selected solver : NelderMead
reason          : small unconstrained problem; simplex local search is preferred
status          : Converged
minimizer       : 1.0001, -2.0000, 0.5000
minimum         : 9.30e-09
evaluations     : 146
iterations      : 80
```
`Converged` means the method's own stopping test was met, not that the point is the global minimum. Here the function is a bowl, so the two coincide; the minimizer is within 1e-4 of (1, -2, 0.5).

## Program 2: Three ways to call

`DFO::solve` takes a `DfoProblem` and `DfoOptions` you fill in field by field, which is the form to use once a problem has bounds or constraints. `DFO::build(start)` chains setters (`with_objective`, `with_bounds`, `with_solver`, `with_seed`, `with_max_evaluations`, and so on) and finishes with `.solve()`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series objective(const pluss::Series& x) { return (x - vec({1.0, -2.0})).square().sum() + 3.0; }

int main() {
  const auto start = vec({4.0, 4.0});

  dfo::DfoProblem problem;
  problem.initial = start;
  problem.objective = objective;

  std::vector<std::pair<const char*, dfo::DfoResult>> runs;
  runs.emplace_back("solve_dfo", dfo::solve_dfo(objective, start));
  runs.emplace_back("DFO::solve", dfo::DFO::solve(problem, dfo::DfoOptions{}));
  runs.emplace_back("DfoBuilder", dfo::DFO::build(start).with_objective(objective).solve());

  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const auto& [label, r] : runs) out.writeln(label, dfo::to_string(r.status), dfo::to_string(r.selected_solver), "minimum", r.minimum, "evals", r.diagnostics.evaluations);
}
```

Output

```text
solve_dfo Converged NelderMead minimum 3.000000 evals 92
DFO::solve Converged NelderMead minimum 3.000000 evals 92
DfoBuilder Converged NelderMead minimum 3.000000 evals 92
```
The three entry points do the same thing; choose by taste.

## Program 3: What Auto chooses

`Auto` looks at the problem, not at the objective values.

| Situation | Method |
|---|---|
| a `blackbox` evaluation function is supplied | `MadsAdvanced` |
| constraints supplied | `PatternSearch` |
| no bounds, up to 10 variables | `NelderMead` |
| no bounds, 40 or more variables | `ParticleSwarm` |
| no bounds, in between | `DifferentialEvolutionPrmm` |
| bounds, fewer than 40 variables | `DifferentialEvolution` |
| bounds, 40 or more variables | `ParticleSwarm` |

The thresholds (10 and 40) live in `DfoOptions::auto_policy`. `selection_reason` always states which rule fired.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series sphere(const pluss::Series& x) { return x.square().sum(); }

int main() {
  struct Case {
    const char* label;
    std::int64_t n;
    bool bounded, constrained;
  };
  const std::vector<Case> cases = {{"3 variables, no bounds", 3, false, false},  {"12 variables, no bounds", 12, false, false}, {"50 variables, no bounds", 50, false, false},
                                   {"3 variables, bounds", 3, true, false},      {"50 variables, bounds", 50, true, false},      {"3 variables, one constraint", 3, false, true}};

  OutputEngine out;
  for (const auto& c : cases) {
    dfo::DfoProblem p;
    p.initial = filled(c.n, 1.5);
    p.objective = sphere;
    if (c.bounded) {
      p.lower_bounds = filled(c.n, -5.0);
      p.upper_bounds = filled(c.n, 5.0);
    }
    if (c.constrained) p.constraints = [](const pluss::Series& x) { return (x.sum() * -1.0 + 1.0).reshape({1}); };  // 1 - sum(x) <= 0
    dfo::DfoOptions o;
    o.max_evaluations = 3000;
    const auto r = dfo::DFO::solve(p, o);
    out.writeln(c.label);
    out.writeln("    ->", dfo::to_string(r.selected_solver) + ":", r.selection_reason);
  }
}
```

Output

```text
3 variables, no bounds
    -> NelderMead: small unconstrained problem; simplex local search is preferred
12 variables, no bounds
    -> DifferentialEvolutionPrmm: unconstrained medium-dimensional problem; DE-PRMM balances exploration/exploitation
50 variables, no bounds
    -> ParticleSwarm: high-dimensional unconstrained problem; swarm search is preferred
3 variables, bounds
    -> DifferentialEvolution: boxed problem without explicit constraints; differential evolution is preferred
50 variables, bounds
    -> ParticleSwarm: high-dimensional boxed problem; swarm search with bounds is preferred
3 variables, one constraint
    -> PatternSearch: explicit nonlinear constraints detected
```
`Auto` is a sensible first guess, not a verdict. Chapter 2 compares the methods on several functions, and shows where the default is the wrong choice.

## Watching the iterations

Setting `display_iter` on the options (or calling `with_display_iter()` on the builder) makes the solver print one line per iteration to standard output, with the evaluations used so far, the best value, and the phase of the method. The run below is capped at 12 iterations so that the listing stays short.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const auto start = pluss::make_series(std::vector<double>{4.0, 4.0}, {2}).to(f64);
  const auto objective = [&](const pluss::Series& x) {
    return (x - pluss::make_series(std::vector<double>{1.0, -2.0}, {2}).to(f64)).square().sum() + 3.0;
  };

  dfo::DfoOptions options;
  options.display_iter = true;
  options.max_iterations = 12;
  const auto result = dfo::solve_dfo(objective, start, options);
  std::cout << "status " << dfo::to_string(result.status) << ", minimum " << result.minimum << ", " << result.diagnostics.evaluations << " evaluations\n";
  return 0;
}
```

Output

```text
iter=1 evals=3 f=48 phase=seed
iter=2 evals=5 f=45.25 phase=expand
iter=3 evals=7 f=41.4625 phase=expand
iter=4 evals=9 f=35.6756 phase=expand
iter=5 evals=11 f=26.9664 phase=expand
iter=6 evals=13 f=21.6872 phase=expand
iter=7 evals=15 f=17.9135 phase=reflect
iter=8 evals=17 f=17.9135 phase=adjust_inner
iter=9 evals=19 f=14.1006 phase=expand
iter=10 evals=21 f=13.2227 phase=reflect
iter=11 evals=23 f=6.10566 phase=expand
iter=12 evals=24 f=6.10566 phase=reflect
status ReachedIterationBudget, minimum 6.10566, 24 evaluations
```
The best value falls from 48 at the seed (iteration 1, three evaluations) to 6.1 by iteration 11, and the `phase` column shows the move the simplex method made at each step: `expand` while the simplex is still marching downhill, `reflect` and `adjust_inner` once it starts to turn and shrink. The run stops at the cap of 12 iterations with `ReachedIterationBudget` after 24 evaluations, a reminder that an iteration is not an evaluation: each iteration here costs about two function calls.

## Decisions and pitfalls

- **`Converged` is the method's own test, not a proof of the global minimum.** On a bowl the two coincide; on a function with many local minima they do not.
- **`Auto` is a sensible first guess, not a verdict.** It chooses by the shape of the problem; the next chapter compares the methods and shows where the default is the wrong choice.
- **Pick the call style by the problem.** The shortcut for an unconstrained function; `DfoProblem` and `DfoOptions` once there are bounds or constraints; the builder when you prefer chained setters. They do the same thing.
- **Read `selection_reason`.** It tells you which rule `Auto` applied, which is the first thing to check when a result surprises you.

Where next: [Budgets, statuses and repeatability](budgets_statuses_and_repeatability.md) explains how a solve ends.
