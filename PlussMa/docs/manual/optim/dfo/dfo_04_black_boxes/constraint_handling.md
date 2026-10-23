<!-- generated from constraint_handling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Constraint handling

[Back to Black Boxes](../dfo_04_black_boxes.md)


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
    const auto x = r.minimizer.to_vec<double>();
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
ExtremeBarrier      3.3487      0.0e+00       214
Progressive, 100    3.3487      0.0e+00       214
Progressive, 1      2.0000      1.0e+00       309
```
- `Ignore` reports the constraint values but does not use them. The answer is the unconstrained minimum and the violation is large.
- `ExtremeBarrier` discards any point that violates a constraint. Safe and simple, but it can never cross infeasible ground.
- `ProgressiveBarrier` accepts a violating point at a penalty. With the default 100, which outweighs the objective's pull toward the infeasible side (about 3.7 per unit of violation at the optimum), the answer is the same as with the extreme barrier. With a penalty of 1 the objective wins and the search settles outside the feasible set. The penalty is a modelling choice: keep it above the objective's steepest slope across the boundary.

Check feasibility yourself after a black-box solve: `max_constraint_violation` is filled by pattern search only, so evaluate the constraints at `minimizer`.
