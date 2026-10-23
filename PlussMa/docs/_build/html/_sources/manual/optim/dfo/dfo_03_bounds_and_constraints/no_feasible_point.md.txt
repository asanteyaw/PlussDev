<!-- generated from no_feasible_point.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# No feasible point

[Back to Bounds and Constraints](../dfo_03_bounds_and_constraints.md)


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
