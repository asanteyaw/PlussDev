<!-- generated from mesh_methods_and_infeasible_problems.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mesh methods with constraints, and infeasible problems

[Back to Derivative-Free Optimization 3: Bounds and Constraints](../dfo_03_bounds_and_constraints.md)

The mesh adaptive methods accept inequality constraints and handle them with a **progressive barrier**: a poll point that violates a constraint may be accepted when the violation is below a threshold that shrinks as the run proceeds, so the search approaches the boundary rather than jumping over it. The first program runs pattern search and the mesh methods on one constrained problem and compares them.

The second program asks the opposite question. If the constraints contradict each other, no method can succeed; what matters is whether it *says so*. With a roomy budget the run ends in `InfeasibleStationary`, having reached a point where no poll reduces the violation; with the default budget the run may instead be cut off by the budget before it can say so, which is why that flag is worth knowing.

## Program 4: Mesh methods with constraints

The mesh adaptive methods accept inequality constraints. They use a progressive barrier: a poll point that violates a constraint may be accepted when the violation is below a threshold that shrinks as the run proceeds, so the search approaches the boundary and does not jump over it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(16) << "solver" << std::setw(26) << "status" << std::setw(12) << "objective"
            << std::setw(12) << "violation" << "evals\n";
  for (auto k : {dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::OrthoMads, dfo::DfoSolverKind::LtMads}) {
    dfo::DfoProblem p;
    p.initial = vec({0.0, 0.0});
    p.objective = [](const pluss::Series& x) { return (x - vec({2.0, 2.0})).square().sum(); };
    p.constraints = [](const pluss::Series& x) { return (x.square().sum() - 1.0).reshape({1}); };
    dfo::DfoOptions o;
    o.solver = k;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(16) << dfo::to_string(k) << std::setw(26) << dfo::to_string(r.status) << std::fixed
              << std::setprecision(4) << std::setw(12) << r.minimum << std::scientific << std::setprecision(1) << std::setw(12)
              << r.diagnostics.max_constraint_violation << std::defaultfloat << r.diagnostics.evaluations << "\n";
  }
  std::cout << "exact objective 3.3431\n";
}
```

Output

```text
solver          status                    objective   violation   evals
PatternSearch   ReachedIterationBudget    3.3432      0.0e+00     814
OrthoMads       Converged                 3.3487      0.0e+00     214
LtMads          Converged                 3.3534      0.0e+00     271
exact objective 3.3431
```
All three return feasible points. The mesh methods use fewer evaluations and stop with `Converged` but end a little above the exact objective (3.3487 and 3.3534 against 3.3431): when the optimum is on a curved boundary, their polls land just inside it. If you need more digits, give the problem more budget, use `PatternSearch`, or enable `MadsAdvanced`'s search step (Chapter 4).

## Program 5: No feasible point

If the constraints contradict each other, no method can succeed; the question is whether it says so.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return x.square().sum(); };
  p.constraints = [](const pluss::Series& x) {
    const auto x0 = x.select(0, 0);
    return (x0 * -1.0 + 2.0).reshape({1}).unite({(x0 - 1.0).reshape({1})}, 0);  // 2 - x0 <= 0 and x0 - 1 <= 0
  };
  dfo::DfoOptions roomy;
  roomy.max_iterations = 4000;
  roomy.max_evaluations = 40000;
  const std::pair<const char*, dfo::DfoOptions> runs[] = {{"default budget", dfo::DfoOptions{}}, {"roomy budget", roomy}};
  for (const auto& [label, options] : runs) {
    const auto r = dfo::DFO::solve(p, options);
    std::cout << std::left << std::setw(16) << label << std::setw(24) << dfo::to_string(r.status) << std::scientific
              << std::setprecision(1) << "violation " << r.diagnostics.max_constraint_violation << std::fixed << std::setprecision(3)
              << "   x0 " << r.minimizer.select(0, 0).item<double>() << "\n";
  }
}
```

Output

```text
default budget  ReachedIterationBudget  violation 5.0e-01   x0 1.500
roomy budget    InfeasibleStationary    violation 5.0e-01   x0 1.500
```
With a roomy budget the run ends in `InfeasibleStationary`: it reached a point where no poll reduces the violation. The returned point is the least-violating one found (here `x0 = 1.5`, halfway between the two contradictory requirements, violating each by 0.5). With the default budget the run is cut off earlier, with a budget status, but the violation reported is the same. Whatever the status, **always check `diagnostics.max_constraint_violation` when you pass constraints**; a status of `Converged` together with a violation of 0 is what a good answer looks like.

## Decisions and pitfalls

- **Mesh methods end a little above the exact objective on a curved boundary.** Their polls land just inside it; give the problem more evaluations or polish with pattern search if you need more digits.
- **A returned point after an infeasible solve is the least-violating one, not a solution.** Always read the status and the violation before using it.
- **An infeasible verdict needs budget.** With the default budget a hopeless problem may end on a budget status; raise it if you suspect infeasibility and want a clear answer.
- **Mesh methods reject equalities.** They fail with an error rather than ignore them; use pattern search for equalities.

Where next: [Chapter 4](../dfo_04_black_boxes.md) covers black-box functions that fail, cost a lot or return several objectives.
