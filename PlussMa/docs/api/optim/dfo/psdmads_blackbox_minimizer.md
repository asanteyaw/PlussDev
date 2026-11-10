<!-- generated from psdmads_blackbox_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::psdmads_blackbox_minimizer`

Minimises a black box in many variables with partial-space decomposition MADS, which optimises random subsets of the variables.

## Signature

```cpp
dfo::BlackboxAdvancedOutcome dfo::psdmads_blackbox_minimizer(const std::function<dfo::BlackboxEvaluation(const pluss::Series &)> &blackbox, const pluss::Series &initial, const dfo::PsdMadsConfig &config = {});
template <class Fn> dfo::BlackboxAdvancedOutcome dfo::psdmads_blackbox_minimizer(Fn &&blackbox, const pluss::Series &initial, const dfo::PsdMadsConfig &config = {});
```

## What It Does

Like cooperative MADS it optimises subsets of the variables with the rest held fixed, but the subsets are random, of random size between `min_subproblem_size` and `max_subproblem_size`, and `subproblems_per_round` of them are drawn in each of `rounds` rounds. Because subsets overlap and vary, variables that interact are optimised together some of the time, which suits problems where neighbouring variables are coupled. A share `polish_fraction` of the budget is kept for a final full-space run.

## Parameters

`Fn &&blackbox`
: A callable returning a `BlackboxEvaluation`, a `Series` of objectives or a number.

`const pluss::Series &initial`
: The starting point.

`const dfo::PsdMadsConfig &config`
: The settings.

## Returns

A [`BlackboxAdvancedOutcome`](BlackboxAdvancedOutcome.md).

## Errors

- A black box that returns neither objectives nor a failure flag throws on the first call.

## Practical Examples

### Example 1

A coupled chain of sixteen variables. The objective `Σ (xᵢ − xᵢ₊₁)² + (x₀ − 1)²` has its minimum 0 at all ones.

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
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    const std::int64_t n = x.size(0);
    const Series a = x.slice(0, 0, n - 1, 1), b = x.slice(0, 1, n, 1);
    e.objectives = ((a - b).square().sum() + (x.select(0, 0) - 1.0).square()).reshape({1});
    return e;
  };
  dfo::PsdMadsConfig config;
  config.base.max_evaluations = 12000;
  config.min_subproblem_size = 4;
  config.max_subproblem_size = 8;
  const dfo::BlackboxAdvancedOutcome o = dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(16, -1.0)), config);
  out.writeln("minimum =", o.scalarized_minimum, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
minimum = 0.28574703 , evaluations = 12021
```

### Example 2

The subset size. A chain of coupled variables is hard for any decomposition: all three runs here stall near 0.3 from the start at −1, and the subset size changes the result only slightly.

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
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    const std::int64_t n = x.size(0);
    const Series a = x.slice(0, 0, n - 1, 1), b = x.slice(0, 1, n, 1);
    e.objectives = ((a - b).square().sum() + (x.select(0, 0) - 1.0).square()).reshape({1});
    return e;
  };
  for (const std::int64_t hi : {3, 8, 12}) {
    dfo::PsdMadsConfig config;
    config.base.max_evaluations = 12000;
    config.min_subproblem_size = 2;
    config.max_subproblem_size = hi;
    out.writeln("subsets of 2 to", hi, ": minimum =", dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(16, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
subsets of 2 to 3 : minimum = 0.26934761
subsets of 2 to 8 : minimum = 0.27842128
subsets of 2 to 12 : minimum = 0.30776208
```

### Example 3

Rounds and subsets per round.

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
  const auto black_box = [](const Series &x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x - 1.0).square().sum().reshape({1});
    return e;
  };
  for (const std::int64_t rounds : {2, 8}) {
    dfo::PsdMadsConfig config;
    config.base.max_evaluations = 8000;
    config.rounds = rounds;
    config.subproblems_per_round = 4;
    out.writeln("rounds", rounds, ": minimum =", dfo::psdmads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(12, -1.0)), config).scalarized_minimum);
  }
}
```

Output

```text
rounds 2 : minimum = 0.00000000
rounds 8 : minimum = 0.00000000
```

## See Also

- [`PsdMadsConfig`](PsdMadsConfig.md): the settings
- [`BlackboxEvaluation`](BlackboxEvaluation.md): what the black box returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pattern_search_minimizer.html">
    <span class="label">Previous</span>
    <span class="title">pattern_search_minimizer</span>
  </a>
  <a class="next-link" href="solve_dfo.html">
    <span class="label">Next</span>
    <span class="title">solve_dfo</span>
  </a>
</div>
<!-- apiref:nav:end -->
