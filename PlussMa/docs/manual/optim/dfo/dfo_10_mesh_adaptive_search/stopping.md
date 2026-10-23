<!-- generated from stopping.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Stopping

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
void run(const char* label, dfo::MadsBlackboxConfig c) {
  const auto o = dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
  std::cout << std::left << std::setw(36) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << "iterations " << o.diagnostics.iterations << "\n";
}

int main() {
  dfo::MadsBlackboxConfig base;
  base.max_iterations = 20000;
  base.max_evaluations = 100000;
  run("defaults (check_every 10, 1e-8)", base);
  auto off = base; off.check_every = 0;
  run("check_every 0 (frame test only)", off);
  auto loose = base; loose.relative_improvement_tolerance = 1e-4;
  run("relative tolerance 1e-4", loose);
  auto it = base; it.max_iterations = 100;
  run("max_iterations 100", it);
  auto ev = base; ev.max_evaluations = 300;
  run("max_evaluations 300", ev);
}
```

Output

```text
defaults (check_every 10, 1e-8)     Converged                 8.76e-23   19280   iterations 6050
check_every 0 (frame test only)     Converged                 1.79e-11   8822    iterations 2769
relative tolerance 1e-4             Converged                 7.89e-19   15636   iterations 4910
max_iterations 100                  ReachedIterationBudget    1.36e-01   327     iterations 100
max_evaluations 300                 ReachedEvaluationBudget   1.86e-01   300     iterations 92
```
By default both the improvement-window test and the frame test must hold, so the window test can only delay a stop, never cause one early. Switching it off (`check_every` 0) lets the frame alone end the run: here it stops after 8822 evaluations instead of 19280, at a value of 2e-11 instead of 9e-23. So the window test is what buys the last digits of accuracy on a function whose minimum value is 0, at roughly twice the cost. A looser relative tolerance sits between the two, and budgets end the run with their own status.
