<!-- generated from partial_space_decomposition.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Partial-space decomposition

[Back to The MADS Variants](../dfo_12_mads_variants.md)


| `PsdMadsConfig` | Meaning | Default |
|---|---|---|
| `base` | the engine settings | |
| `rounds` | rounds | 5 |
| `polish_fraction` | share of the budget kept for a final full-space run from the best point (0 = none) | 0.25 |
| `subproblems_per_round` | random subsets per round | 4 |
| `min_subproblem_size`, `max_subproblem_size` | size range of the subsets (0 = half the variables) | 2, 0 |
| `randomized_subproblems` | reshuffle the variables for every subset | true |
| `rng_seed` | | 5489 |

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
  const std::int64_t n = 16;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.slice(0, 0, n - 1, 1), b = x.slice(0, 1, n, 1);
    e.objectives = ((a - b).square().sum() + (x.select(0, 0) - 1.0).square()).reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(38) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  {
    dfo::MadsBlackboxAdvancedConfig c;
    c.max_evaluations = 12000;
    c.max_iterations = 5000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(38) << "plain MADS" << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
  struct Row { const char* label; std::int64_t lo, hi, per, rounds; };
  const Row rows[] = {{"psd, subsets of 2..4, 3 per round", 2, 4, 3, 5}, {"psd, subsets of 4..8, 3 per round", 4, 8, 3, 5}, {"psd, subsets of 8..12, 3 per round", 8, 12, 3, 5}};
  for (const auto& r : rows) {
    dfo::PsdMadsConfig c;
    c.base.max_evaluations = 12000;
    c.base.max_iterations = 5000;
    c.min_subproblem_size = r.lo;
    c.max_subproblem_size = r.hi;
    c.subproblems_per_round = r.per;
    c.rounds = r.rounds;
    const auto o = dfo::psdmads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(38) << r.label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
method                                status                    minimum     evals
plain MADS                            ReachedEvaluationBudget   1.09e-01    12000
psd, subsets of 2..4, 3 per round     ReachedEvaluationBudget   3.55e-01    7965
psd, subsets of 4..8, 3 per round     ReachedEvaluationBudget   3.09e-01    11628
psd, subsets of 8..12, 3 per round    ReachedEvaluationBudget   2.24e-01    12016
```
The chain function couples every variable to its neighbours, which is the case decomposition handles worst: improving a subset while the rest stand still is limited by the coupling, and the final full-space run (`polish_fraction`) recovers only part of it. Plain MADS does better than every decomposition here, and larger subsets do better than small ones; raising `polish_fraction` moves the result toward plain MADS, as it must, and gains nothing over it. This is a property of decomposition, not a defect: use it when variables are weakly coupled (Program 1), and plain MADS or a population method (Chapters 8 and 9) when they are strongly coupled.
