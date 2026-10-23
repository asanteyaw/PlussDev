<!-- generated from the_settings_one_at_a_time.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The settings, one at a time

[Back to The Nelder–Mead Minimiser](../dfo_06_nelder_mead.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

void run(const char* label, dfo::NelderMeadConfig c) {
  auto f = [](const pluss::Series& x) {
    const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
    const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
    return d.square().sum() + s.square() * 0.8;
  };
  const auto o = dfo::nelder_mead_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
  std::cout << std::left << std::setw(30) << label << std::setw(24)
            << (o.status == dfo::SolverStatus::Converged ? "Converged"
                : o.status == dfo::SolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget")
            << std::scientific << std::setprecision(2) << std::setw(11) << o.minimum << std::defaultfloat << "evals "
            << o.diagnostics.evaluations << "\n";
}

int main() {
  dfo::NelderMeadConfig base;
  run("defaults", base);
  auto tight = base; tight.position_tolerance = 1e-8; tight.value_tolerance = 1e-8;
  run("tolerances 1e-8", tight);
  auto loose = base; loose.position_tolerance = 1e-2; loose.value_tolerance = 1e-2;
  run("tolerances 1e-2", loose);
  auto big = base; big.simplex_delta = 0.5;
  run("simplex_delta 0.5", big);
  auto tiny = base; tiny.simplex_delta = 0.001;
  run("simplex_delta 0.001", tiny);
  auto bold = base; bold.expansion = 3.0;
  run("expansion 3", bold);
  auto timid = base; timid.contraction = 0.25; timid.shrink = 0.25;
  run("contraction, shrink 0.25", timid);
  auto capped = base; capped.max_evaluations = 100;
  run("max_evaluations 100", capped);
}
```

Output

```text
defaults                      Converged               1.72e-09   evals 240
tolerances 1e-8               Converged               1.58e-17   evals 385
tolerances 1e-2               Converged               1.80e-05   evals 177
simplex_delta 0.5             Converged               3.33e-09   evals 192
simplex_delta 0.001           Converged               1.47e-08   evals 400
expansion 3                   Converged               2.20e-08   evals 318
contraction, shrink 0.25      Converged               3.16e-06   evals 478
max_evaluations 100           ReachedEvaluationBudget 2.09e+00   evals 100
```
What the rows show:

- **Tolerances** trade work for accuracy: 1e-8 costs about 60% more evaluations than the default and gains eight orders of magnitude in the objective; 1e-2 saves about a quarter.
- **Starting simplex.** A large simplex (0.5) explores more and here converged sooner; a tiny one (0.001) must grow before it can move, and took more than the default. Match the simplex to the scale of the problem: if the variables are of order 100, 5% is a step of 5.
- **Coefficients.** The defaults are the standard choice. Changing them rarely helps; the rows above are worse. Keep expansion above 1, contraction and shrink between 0 and 1.
- **Budget.** Stopping at 100 evaluations returns the best point seen with `ReachedEvaluationBudget`.
