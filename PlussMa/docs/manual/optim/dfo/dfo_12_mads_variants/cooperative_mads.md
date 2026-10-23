<!-- generated from cooperative_mads.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Cooperative MADS

[Back to The MADS Variants](../dfo_12_mads_variants.md)


Plain MADS polls 2n directions each iteration, so its cost per step grows with the number of variables. Cooperative MADS runs MADS on one block of variables at a time with the others held fixed, and cycles through the blocks for `cooperative_rounds` rounds.

| `CoopMadsConfig` | Meaning | Default |
|---|---|---|
| `base` | the engine settings | |
| `cooperative_rounds` | passes over all the blocks | 4 |
| `polish_fraction` | share of the budget kept for a final full-space run from the best point (0 = none) | 0.25 |
| `subproblem_size` | variables per block (0 = derived from `subproblems_per_round`) | 0 |
| `subproblems_per_round` | blocks per round (0 = enough to cover all variables) | 0 |
| `randomized_subproblems` | reshuffle which variables share a block each round | true |
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
  const std::int64_t n = 24;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 1.0).square().sum().reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(30) << "method" << std::setw(26) << "status" << std::setw(12) << "minimum" << "evals\n";
  {
    dfo::MadsBlackboxAdvancedConfig c;
    c.max_evaluations = 12000;
    c.max_iterations = 5000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(30) << "plain MADS" << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
  for (std::int64_t block : {3, 6, 12}) {
    dfo::CoopMadsConfig c;
    c.base.max_evaluations = 12000;
    c.base.max_iterations = 5000;
    c.subproblem_size = block;
    const auto o = dfo::coopmads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(30) << ("coop, block size " + std::to_string(block)) << std::setw(26) << name(o.status) << std::scientific
              << std::setprecision(2) << std::setw(12) << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
method                        status                    minimum     evals
plain MADS                    ReachedEvaluationBudget   1.84e-04    12000
coop, block size 3            ReachedEvaluationBudget   9.24e-21    10466
coop, block size 6            ReachedEvaluationBudget   1.97e-15    12009
coop, block size 12           ReachedEvaluationBudget   5.47e-10    12009
```
On a separable function the blocks do not interfere, and all three block sizes reach a far better value than plain MADS for the same evaluation count (1e-20 to 1e-10 against 2e-4). Small blocks are best here because each block's polling is cheap.
