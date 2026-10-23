<!-- generated from weighted_sum_or_chebyshev.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Weighted sum or Chebyshev

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
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0);
    e.objectives = a.reshape({1}).unite({(a.square() * -1.0 + 1.0).reshape({1})}, 0);
    return e;
  };
  struct Row { const char* label; dfo::MultiObjectiveScalarization s; };
  const Row rows[] = {{"WeightedSum", dfo::MultiObjectiveScalarization::WeightedSum}, {"Chebyshev", dfo::MultiObjectiveScalarization::Chebyshev}};
  std::cout << std::left << std::setw(14) << "scalarization" << std::setw(12) << "x" << std::setw(12) << "f1" << std::setw(12) << "f2" << "evals\n";
  for (const auto& r : rows) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.lower_bounds = vec({0.0});
    c.upper_bounds = vec({1.0});
    c.multiobjective_scalarization = r.s;
    c.objective_weights = (r.s == dfo::MultiObjectiveScalarization::WeightedSum) ? vec({0.6, 0.4}) : vec({1.0, 1.0});
    c.objective_reference = vec({0.0, 0.0});
    c.max_evaluations = 3000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({0.8}), c);
    const auto f = o.objective_vector.to_vec<double>();
    std::cout << std::left << std::setw(14) << r.label << std::fixed << std::setprecision(4) << std::setw(12) << o.minimizer.select(0, 0).item<double>()
              << std::setw(12) << f[0] << std::setw(12) << f[1] << o.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
scalarization x           f1          f2          evals
WeightedSum   1.0000      1.0000      0.0000      29
Chebyshev     0.6180      0.6180      0.6180      87
```
On a non-convex front the weighted sum can only reach an end of the trade-off, here the end at `x = 1`, and Chebyshev finds the balanced compromise at `x = 0.618`. When the front is convex both rules reach interior points and the weighted sum is cheaper. Choose Chebyshev when you want a particular balance or suspect the trade-off is not convex.
