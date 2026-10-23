<!-- generated from a_direct_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A direct call

[Back to The Black-Box MADS Minimisers](../dfo_11_mads_black_box.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  dfo::MadsBlackboxAdvancedConfig c;
  c.constraint_handling = dfo::ConstraintHandling::ExtremeBarrier;
  c.max_evaluations = 2000;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0), b = x.select(0, 1);
    e.objectives = ((a - 1.0).square() + b.square()).reshape({1}).unite({((a + 1.0).square() + b.square()).reshape({1})}, 0);
    e.constraints = (a - 0.5).reshape({1});
    return e;
  };
  const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({-0.5, 0.5}), c);
  const auto f = o.objective_vector.to_vec<double>();
  std::cout << std::fixed << std::setprecision(4) << "minimizer          : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objectives         : " << f[0] << ", " << f[1] << "\n";
  std::cout << "scalarized minimum : " << o.scalarized_minimum << "  (equal weights)\n";
  std::cout << "status             : " << name(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "evaluations        : " << d.evaluations << " (total " << d.total_evaluations << ", cache hits " << d.cache_hits << ")\n";
  std::cout << "objective / constraint dimension: " << d.objective_dimension << " / " << d.constraint_dimension << "\n";
  std::cout << "Pareto archive     : " << d.pareto_size << " points, " << o.pareto_points.size(0) << " x " << o.pareto_points.size(1) << " matrix\n";
}
```

Output

```text
minimizer          : 0.0000, 0.0000
objectives         : 1.0000, 1.0000
scalarized minimum : 1.0000  (equal weights)
status             : Converged
evaluations        : 281 (total 283, cache hits 2)
objective / constraint dimension: 2 / 1
Pareto archive     : 251 points, 251 x 2 matrix
```
Both objectives are read back at the minimiser, along with the scalar the solver minimised. The constraint is inactive here (the compromise sits at `x0 = 0`, inside `x0 <= 0.5`). The archive holds the non-dominated points the search met, as rows.
