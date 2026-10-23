<!-- generated from the_two_poll_schemes.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The two poll schemes

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
void row(const char* label, const dfo::BlackboxSolverOutcome& o) {
  std::cout << "  " << std::left << std::setw(34) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << "evals " << o.diagnostics.evaluations << "\n";
}

int main() {
  auto kink = [](const pluss::Series& x) { return (x - vec({1.0, -2.0, 0.5, 3.0})).abs().sum(); };
  dfo::MadsBlackboxConfig c;
  c.max_iterations = 4000;
  c.max_evaluations = 40000;
  std::cout << "Rosenbrock\n";
  row("orthomads_minimizer", dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c));
  row("ltmads_minimizer", dfo::ltmads_minimizer(rosenbrock, vec({-1.2, 1.0}), c));
  auto lt = c; lt.poll_scheme = dfo::MadsPollScheme::LtMadsRandom2N;
  row("mesh_adaptive..., LtMadsRandom2N", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}), lt));
  std::cout << "sum |x - c|, 4 variables\n";
  row("orthomads_minimizer", dfo::orthomads_minimizer(kink, vec({0, 0, 0, 0}), c));
  row("ltmads_minimizer", dfo::ltmads_minimizer(kink, vec({0, 0, 0, 0}), c));
}
```

Output

```text
Rosenbrock
  orthomads_minimizer               ReachedIterationBudget    3.20e-15   evals 12761
  ltmads_minimizer                  ReachedIterationBudget    3.43e-15   evals 12928
  mesh_adaptive..., LtMadsRandom2N  ReachedIterationBudget    3.43e-15   evals 12928
sum |x - c|, 4 variables
  orthomads_minimizer               Converged                 0.00e+00   evals 2012
  ltmads_minimizer                  Converged                 0.00e+00   evals 1784
```
Both schemes follow the curved Rosenbrock valley to about 1e-15 within the iteration limit, and both solve the non-smooth sum of absolute values exactly, where a simplex stalls (Chapter 2). The two are interchangeable in practice; the differences you see are the random path. `mesh_adaptive_direct_search_minimizer` with `LtMadsRandom2N` is exactly `ltmads_minimizer`.
