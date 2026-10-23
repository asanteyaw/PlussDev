<!-- generated from equality_constraints.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Equality constraints

[Back to Bounds and Constraints](../dfo_03_bounds_and_constraints.md)


Put them in `constraints_values`, whose result has an `inequality` and an `equality` part; fill either or both.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({3.0, 0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return x.square().sum(); };
  p.constraints_values = [](const pluss::Series& x) {
    dfo::DfoConstraintValues c;
    c.equality = (x.sum() - 3.0).reshape({1});
    return c;
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << "\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << ", " << r.minimizer.select(0, 2).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
```

Output

```text
solver    : PatternSearch
status    : Converged
minimizer : 1.0000, 1.0000, 0.9999
objective : 3.0000  (exact 3)
violation : 1.3e-07
evals     : 1544
```
An equality is harder than an inequality for a method that only compares values: a feasible point must lie on a surface of lower dimension, and a random poll almost never does. Pattern search handles it with an augmented Lagrangian, a penalty that grows with the violation. The violation therefore settles near 1e-7, not at zero; the constraint is met to a tolerance.
