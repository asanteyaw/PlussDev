<!-- generated from inequality_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Inequality constraints

[Back to Bounds and Constraints](../dfo_03_bounds_and_constraints.md)


`constraints` returns a vector with one entry per constraint.

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
  p.objective = [](const pluss::Series& x) { return (x - vec({2.0, 2.0})).square().sum(); };
  p.constraints = [](const pluss::Series& x) {
    const auto disc = (x.square().sum() - 1.0).reshape({1});
    const auto half = (x.sum() - 3.0).reshape({1});
    return disc.unite({half}, 0);
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << " (" << r.selection_reason << ")\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3.3431)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
```

Output

```text
solver    : PatternSearch (explicit nonlinear constraints detected)
status    : ReachedIterationBudget
minimizer : 0.7059, 0.7083
objective : 3.3432  (exact 3.3431)
violation : 0.0e+00
evals     : 814
```
The answer is on the boundary of the disc, as it must be, since the objective wants to go further out. `violation` is the largest positive constraint value of the returned point; 0 means feasible. The run stopped on its iteration budget while still creeping along the boundary, which is typical of constrained direct search: the last digits are slow, and the objective agrees with the exact value to 1e-4.
