<!-- generated from several_objectives_and_constraint_policies.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Several objectives and constraint policies

[Back to Derivative-Free Optimization 4: Black Boxes](../dfo_04_black_boxes.md)

Black boxes often return more than one number. When they return several *objectives*, the solver minimises one scalar built from them with weights you give, and also keeps the **Pareto set**: the evaluated points that no other evaluated point beats on every objective. Changing the weights slides the answer along the trade-off between the objectives, exactly as the formula says. The Pareto archive is a by-product of the search, not a computed front: it covers the points the search happened to visit.

When a black box returns constraint values, the **constraint-handling policy** says what to do with a point that violates them: `Ignore` (report the values, do not use them), `ExtremeBarrier` (discard any violating point: safe, but it can never cross infeasible ground) or `ProgressiveBarrier` (accept a violating point at a penalty). The penalty's size, relative to how strongly the objective pulls toward the infeasible side, decides whether the second and third give the same answer.

## Program 3: Two objectives

The black box returns a vector of objectives. The solver minimises one scalar built from them and also keeps the Pareto set, the points that no other evaluated point beats on every objective. The weights state how much each objective matters.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(12) << "weight w" << std::setw(12) << "x" << std::setw(12) << "expected" << std::setw(12)
            << std::setw(12) << "f1" << std::setw(12) << "f2" << "pareto points\n";
  for (double w : {0.2, 0.5, 0.8}) {
    dfo::DfoProblem p;
    p.initial = vec({0.0});
    p.blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto f1 = (x - 1.0).square().sum().reshape({1});
      const auto f2 = (x + 1.0).square().sum().reshape({1});
      e.objectives = f1.unite({f2}, 0);
      return e;
    };
    dfo::DfoOptions o;
    o.mads.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    o.mads.objective_weights = vec({w, 1.0 - w});
    const auto r = dfo::DFO::solve(p, o);
    const auto f = r.objective_vector.host<double>();
    std::cout << std::fixed << std::setprecision(3) << std::left << std::setw(12) << w << std::setw(12)
              << r.minimizer.select(0, 0).item<double>() << std::setw(12) << 2.0 * w - 1.0 << std::setw(12) << std::setw(12) << f[0] << std::setw(12) << f[1] << r.pareto_points.size(0) << "\n";
  }
}
```

Output

```text
weight w    x           expected    f1          f2          pareto points
0.200       -0.600      -0.600      2.560       0.160       71
0.500       0.000       0.000       1.000       1.000       61
0.800       0.600       0.600       0.160       2.560       71
```
The weights slide the answer along the trade-off between the two objectives, exactly as the formula says. `result.pareto_points` and `result.pareto_objectives` hold the non-dominated points (as rows) and their objective vectors. They are an archive of the search, not a computed front: they cover the region the search visited, which is densest near the chosen compromise. To map a whole front, run several weightings and gather their archives.

## Program 4: Constraint handling

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  struct Mode { const char* name; dfo::ConstraintHandling h; double penalty; };
  const Mode modes[] = {{"Ignore", dfo::ConstraintHandling::Ignore, 100.0},
                        {"ExtremeBarrier", dfo::ConstraintHandling::ExtremeBarrier, 100.0},
                        {"Progressive, 100", dfo::ConstraintHandling::ProgressiveBarrier, 100.0},
                        {"Progressive, 1", dfo::ConstraintHandling::ProgressiveBarrier, 1.0}};
  std::cout << std::left << std::setw(20) << "handling" << std::setw(12) << "objective" << std::setw(14) << "violation" << "evals\n";
  for (const auto& m : modes) {
    dfo::DfoProblem p;
    p.initial = vec({0.0, 0.0});
    p.blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({2.0, 2.0})).square().sum().reshape({1});
      e.constraints = (x.square().sum() - 1.0).reshape({1});
      return e;
    };
    dfo::DfoOptions o;
    o.mads.constraint_handling = m.h;
    o.mads.progressive_barrier_penalty = m.penalty;
    const auto r = dfo::DFO::solve(p, o);
    const auto x = r.minimizer.host<double>();
    const double violation = std::max(0.0, x[0] * x[0] + x[1] * x[1] - 1.0);
    std::cout << std::left << std::setw(20) << m.name << std::fixed << std::setprecision(4) << std::setw(12) << r.minimum
              << std::scientific << std::setprecision(1) << std::setw(14) << violation << std::defaultfloat
              << r.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
handling            objective   violation     evals
Ignore              0.0000      7.0e+00       474
ExtremeBarrier      3.3432      0.0e+00       1000
Progressive, 100    3.3432      0.0e+00       1000
Progressive, 1      2.0000      1.0e+00       541
```
- `Ignore` reports the constraint values but does not use them. The answer is the unconstrained minimum and the violation is large.
- `ExtremeBarrier` discards any point that violates a constraint. Safe and simple, but it can never cross infeasible ground.
- `ProgressiveBarrier` accepts a violating point at a penalty. With the default 100, which outweighs the objective's pull toward the infeasible side (about 3.7 per unit of violation at the optimum), the answer is the same as with the extreme barrier. With a penalty of 1 the objective wins and the search settles outside the feasible set. The penalty is a modelling choice: keep it above the objective's steepest slope across the boundary.

Check feasibility yourself after a black-box solve: `max_constraint_violation` is filled by pattern search only, so evaluate the constraints at `minimizer`.

## Decisions and pitfalls

- **The weights state how much each objective matters.** Different weights give different answers on the trade-off; report the weights with the answer.
- **The Pareto archive covers visited points only.** It is a record of the search, not the true front; to map the front, run with several weightings.
- **Pick the constraint policy deliberately.** `Ignore` returns the unconstrained minimum; the barrier policies differ in whether they can cross infeasible ground.
- **A progressive-barrier penalty must outweigh the objective's pull.** With a penalty of 1 the objective wins and the answer is violating; with the default 100 it matches the extreme barrier.
- **Check feasibility yourself after a black-box solve.** `max_constraint_violation` is filled by pattern search only; evaluate the constraints at `minimizer`.

Where next: [Chapter 5](../dfo_05_worked_problems.md) works through a fit and the checks that verify an answer.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="failing_and_expensive_functions.html">
    <span class="label">Previous</span>
    <span class="title">Functions that fail and functions that are expensive</span>
  </a>
</div>
<!-- apiref:nav:end -->
