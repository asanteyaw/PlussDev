<!-- generated from a_direct_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A direct call

[Back to The Differential Evolution Minimisers](../dfo_08_differential_evolution.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}
const char* name(dfo::PopulationSolverStatus s) {
  return s == dfo::PopulationSolverStatus::Converged ? "Converged"
         : s == dfo::PopulationSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}

int main() {
  dfo::DifferentialEvolutionConfig c;
  c.lower_bounds = filled(3, -5.12);
  c.upper_bounds = filled(3, 5.12);
  c.max_evaluations = 40000;
  const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
  const auto x = o.minimizer.to_vec<double>();
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << x[0] << ", " << x[1] << ", " << x[2] << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << name(o.status) << "\n";
  std::cout << "generations : " << o.diagnostics.iterations << "\nevaluations : " << o.diagnostics.evaluations << "\nlast step   : " << o.diagnostics.last_step << "\n";
}
```

Output

```text
minimizer   : -0.00002, 0.00000, -0.00002
minimum     : 1.52e-07
status      : ReachedEvaluationBudget
generations : 199
evaluations : 40000
last step   : generation
```
The search is global, so it is run in a box. The start (3, 3, 3) lies in a local minimum's basin; the population covers the whole box and finds the global minimum at the origin. The run used its whole evaluation budget: with the early-stop test off (`check_every` 0, the default), only the budgets end a run. Program 5 turns the test on.
