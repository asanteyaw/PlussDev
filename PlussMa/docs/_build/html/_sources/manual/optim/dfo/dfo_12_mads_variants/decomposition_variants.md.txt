<!-- generated from decomposition_variants.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Variants for many variables: cooperative and partial-space MADS

[Back to Derivative-Free Optimization 12: The MADS Variants](../dfo_12_mads_variants.md)

Four specialised solvers are built on the black-box MADS engine. They are available as standalone functions only; the `DFO` facade does not select them. Each addresses one situation, and this page covers the two for **many variables**. Plain MADS polls `2n` directions each iteration, so its cost per step grows with the number of variables. Both variants here reduce that by working on *blocks* of variables at a time.

**Cooperative MADS** runs MADS on one block with the others held fixed, and cycles through the blocks for several rounds. **Partial-space decomposition** does the same with random subsets of the variables, reshuffled each round. Both keep a share of the budget (`polish_fraction`) for a final full-space run from the best point. They help exactly when the variables are weakly coupled, and the two programs show both sides: a separable function where blocks do not interfere and the decomposition wins by many orders of magnitude, and a chain function that couples every variable to its neighbours, where decomposition does worse than plain MADS.

## Program 1: Cooperative MADS

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
    std::cout << std::left << std::setw(30) << "plain MADS" << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
              << o.scalarized_minimum << std::defaultfloat << o.diagnostics.evaluations << "\n";
  }
  for (std::int64_t block : {3, 6, 12}) {
    dfo::CoopMadsConfig c;
    c.base.max_evaluations = 12000;
    c.base.max_iterations = 5000;
    c.subproblem_size = block;
    const auto o = dfo::coopmads_blackbox_minimizer(blackbox, filled(n, -1.0), c);
    std::cout << std::left << std::setw(30) << ("coop, block size " + std::to_string(block)) << std::setw(26) << dfo::to_string(o.status) << std::scientific
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

## Program 2: Partial-space decomposition

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
    std::cout << std::left << std::setw(38) << "plain MADS" << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
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
    std::cout << std::left << std::setw(38) << r.label << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2) << std::setw(12)
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

## Decisions and pitfalls

- **Use decomposition only for weakly coupled variables.** On a separable function all block sizes beat plain MADS by far (1e-20 to 1e-10 against 2e-4); on a chain function plain MADS beats every decomposition.
- **Small blocks are cheap to poll.** On the separable function the smallest block was best; on a coupled function larger subsets do better.
- **The polish run recovers only part of what coupling costs.** Raising `polish_fraction` moves the result toward plain MADS and gains nothing over it.
- **For strongly coupled variables use plain MADS or a population method.** Decomposition is a property of the method, not a defect.

Where next: [Failures and several basins](failures_and_several_basins.md) covers the other two variants.
