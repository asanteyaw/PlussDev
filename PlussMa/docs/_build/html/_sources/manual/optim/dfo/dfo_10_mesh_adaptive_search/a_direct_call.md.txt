<!-- generated from a_direct_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A direct call

[Back to Mesh Adaptive Direct Search](../dfo_10_mesh_adaptive_search.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  const auto o = dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << name(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "iterations  : " << d.iterations << "\nevaluations : " << d.evaluations << "\nfinal mesh  : " << d.mesh_size
            << "\nfinal frame : " << d.frame_size << "\nlast phase  : " << d.last_phase << "\n";
}
```

Output

```text
minimizer   : 0.90775, 0.82350
minimum     : 8.54e-03
status      : ReachedEvaluationBudget
iterations  : 311
evaluations : 1000
final mesh  : 7.32e-06
final frame : 1.88e-03
last phase  : refine_mesh
```
The default budget is 500 evaluations per variable, so this two-variable run stops after 1000 evaluations with `ReachedEvaluationBudget`, still some way from the minimum. Read the final frame size: when it is much larger than `mesh_tolerance`, the run was cut off, not finished. Raise `max_evaluations` (Program 2 does) and continue.
