<!-- generated from the_pareto_archive.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Pareto archive

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
  std::cout << std::left << std::setw(22) << "pareto_archive_limit" << std::setw(16) << "archive size" << std::setw(4) << "" << "x range\n";
  for (std::int64_t limit : {256, 20, 5}) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.pareto_archive_limit = limit;
    c.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    c.objective_weights = vec({0.5, 0.5});
    c.max_evaluations = 800;
    auto blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto a = x.select(0, 0);
      e.objectives = a.square().reshape({1}).unite({(a - 2.0).square().reshape({1})}, 0);
      return e;
    };
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({5.0}), c);
    const auto p = o.pareto_points.reshape({o.pareto_points.numel()}).to_vec<double>();
    
    double lo = 1e300, hi = -1e300;
    for (double v : p) { lo = std::min(lo, v); hi = std::max(hi, v); }
    std::cout << std::left << std::setw(22) << limit << std::setw(16) << p.size() << std::setw(4) << "" << std::fixed << std::setprecision(3) << lo << " .. " << hi << "\n";
  }
}
```

Output

```text
pareto_archive_limit  archive size        x range
256                   73                  -0.100 .. 1.940
20                    20                  -0.100 .. 1.940
5                     5                   -0.100 .. 1.940
```
The archive is a record of the search, not a computed front: it holds non-dominated points among those evaluated. With a tight limit it drops the most crowded entries, so a small archive still spans the front. To trace a front, solve with several weightings and merge the archives.
