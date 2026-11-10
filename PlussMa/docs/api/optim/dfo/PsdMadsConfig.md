<!-- generated from PsdMadsConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PsdMadsConfig`

Settings of partial-space decomposition MADS: the base configuration plus the random subsets.

`base` is the [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md) used for each subset. In each of `rounds` rounds, `subproblems_per_round` random subsets are drawn with sizes between `min_subproblem_size` and `max_subproblem_size` (0 means all variables), each optimised with the others fixed. `polish_fraction` of the budget is kept for a final run over all variables.

## Fields

### `base`

`dfo::MadsBlackboxAdvancedConfig`, default `{}`.

The MADS settings used for each subset.

### `rounds`

`std::int64_t`, default `5`.

Rounds of random subsets.

### `polish_fraction`

`double`, default `0.25`.

Share of the budget kept for a final full-space run; 0 for none.

### `min_subproblem_size`

`std::int64_t`, default `2`.

Smallest subset.

### `max_subproblem_size`

`std::int64_t`, default `0`.

Largest subset; 0 for all variables.

### `subproblems_per_round`

`std::int64_t`, default `4`.

Subsets drawn per round.

### `randomized_subproblems`

`bool`, default `true`.

Draw the subsets at random.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the draws.

## Guidance

Passed to `psdmads_blackbox_minimizer`.

## Practical Examples

### Example 1

The subset sizes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto black_box = [](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = (x - 1.0).square().sum().reshape({1}); return e; };
  for (const std::int64_t hi : {3, 6, 12}) {
    dfo::PsdMadsConfig config;
    config.base.max_evaluations = 8000;
    config.max_subproblem_size = hi;
    out.writeln("largest subset", hi, ": minimum =", dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(12, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
largest subset 3 : minimum = 0.00000000
largest subset 6 : minimum = 0.00000000
largest subset 12 : minimum = 0.00000000
```

### Example 2

Rounds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto black_box = [](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = (x - 1.0).square().sum().reshape({1}); return e; };
  for (const std::int64_t rounds : {1, 5}) {
    dfo::PsdMadsConfig config;
    config.base.max_evaluations = 8000;
    config.rounds = rounds;
    out.writeln("rounds", rounds, ": minimum =", dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(12, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
rounds 1 : minimum = 0.00000064
rounds 5 : minimum = 0.00000000
```

### Example 3

The polish.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto black_box = [](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = (x - 1.0).square().sum().reshape({1}); return e; };
  for (const double polish : {0.0, 0.3}) {
    dfo::PsdMadsConfig config;
    config.base.max_evaluations = 8000;
    config.polish_fraction = polish;
    out.writeln("polish", polish, ": minimum =", dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(12, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
polish 0.00000000 : minimum = 0.00000000
polish 0.30000000 : minimum = 0.00000000
```

## See Also

- [`psdmads_blackbox_minimizer`](psdmads_blackbox_minimizer.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PopulationSolverStatus.html">
    <span class="label">Previous</span>
    <span class="title">PopulationSolverStatus</span>
  </a>
  <a class="next-link" href="SolverDisplay.html">
    <span class="label">Next</span>
    <span class="title">SolverDisplay</span>
  </a>
</div>
<!-- apiref:nav:end -->
