<!-- generated from mesh_methods_with_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mesh methods with constraints

[Back to Bounds and Constraints](../dfo_03_bounds_and_constraints.md)


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
