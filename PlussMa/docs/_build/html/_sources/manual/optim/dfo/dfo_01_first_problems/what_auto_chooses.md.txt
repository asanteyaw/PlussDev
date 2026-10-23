<!-- generated from what_auto_chooses.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What Auto chooses

[Back to First Problems](../dfo_01_first_problems.md)


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

#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series sphere(const pluss::Series& x) { return x.square().sum(); }

void run(const char* label, std::int64_t n, bool bounded, bool constrained, std::int64_t budget) {
  dfo::DfoProblem p;
  p.initial = filled(n, 1.5);
  p.objective = sphere;
  if (bounded) {
    p.lower_bounds = filled(n, -5.0);
    p.upper_bounds = filled(n, 5.0);
  }
  if (constrained) p.constraints = [](const pluss::Series& x) { return (x.sum() * -1.0 + 1.0).reshape({1}); };  // 1 - sum(x) <= 0
  dfo::DfoOptions o;
  o.max_evaluations = budget;
  const auto r = dfo::DFO::solve(p, o);
  std::cout << label << "\n    -> " << dfo::to_string(r.selected_solver) << ": " << r.selection_reason << "\n";
}

int main() {
  run("3 variables, no bounds", 3, false, false, 3000);
  run("12 variables, no bounds", 12, false, false, 3000);
  run("50 variables, no bounds", 50, false, false, 3000);
  run("3 variables, bounds", 3, true, false, 3000);
  run("50 variables, bounds", 50, true, false, 3000);
  run("3 variables, one constraint", 3, false, true, 3000);
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
