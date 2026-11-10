<!-- generated from CoopMadsConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::CoopMadsConfig`

Settings of cooperative MADS: the base MADS configuration plus the block structure.

`base` is the [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md) used for each block; its evaluation budget is divided among all blocks and rounds. `subproblem_size` is the number of variables in a block (0 chooses it from `subproblems_per_round`), `cooperative_rounds` the number of sweeps over the blocks, `randomized_subproblems` reshuffles which variables share a block every round, and `polish_fraction` is the share of the budget kept for a final run over all variables.

## Fields

### `base`

`dfo::MadsBlackboxAdvancedConfig`, default `{}`.

The MADS settings used for each block.

### `cooperative_rounds`

`std::int64_t`, default `4`.

Sweeps over the blocks.

### `polish_fraction`

`double`, default `0.25`.

Share of the budget kept for a final full-space run; 0 for none.

### `subproblem_size`

`std::int64_t`, default `0`.

Variables per block; 0 chooses from `subproblems_per_round`.

### `subproblems_per_round`

`std::int64_t`, default `0`.

Blocks per sweep.

### `randomized_subproblems`

`bool`, default `true`.

Reshuffle the blocks every round.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the shuffling.

## Guidance

Passed to `coopmads_blackbox_minimizer`.

## Practical Examples

### Example 1

The block size.

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
  for (const std::int64_t block : {3, 6, 12}) {
    dfo::CoopMadsConfig config;
    config.base.max_evaluations = 12000;
    config.subproblem_size = block;
    out.writeln("block", block, ": minimum =", dfo::coopmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(24, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
block 3 : minimum = 0.00000000
block 6 : minimum = 0.00000000
block 12 : minimum = 0.00000000
```

### Example 2

The number of rounds.

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
  for (const std::int64_t rounds : {1, 4, 8}) {
    dfo::CoopMadsConfig config;
    config.base.max_evaluations = 12000;
    config.subproblem_size = 6;
    config.cooperative_rounds = rounds;
    out.writeln("rounds", rounds, ": minimum =", dfo::coopmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(24, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
rounds 1 : minimum = 0.00000000
rounds 4 : minimum = 0.00000000
rounds 8 : minimum = 0.00000000
```

### Example 3

Fixed blocks against reshuffled blocks.

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
  for (const bool shuffle : {false, true}) {
    dfo::CoopMadsConfig config;
    config.base.max_evaluations = 12000;
    config.subproblem_size = 6;
    config.randomized_subproblems = shuffle;
    out.writeln("randomized", shuffle, ": minimum =", dfo::coopmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(24, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
randomized false : minimum = 0.00000000
randomized true : minimum = 0.00000000
```

## See Also

- [`coopmads_blackbox_minimizer`](coopmads_blackbox_minimizer.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConstraintHandling.html">
    <span class="label">Previous</span>
    <span class="title">ConstraintHandling</span>
  </a>
  <a class="next-link" href="DFO.html">
    <span class="label">Next</span>
    <span class="title">DFO</span>
  </a>
</div>
<!-- apiref:nav:end -->
