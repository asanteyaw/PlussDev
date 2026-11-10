<!-- generated from SurrogateMode.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::SurrogateMode`

Whether a cheap model orders the poll candidates of a mesh-adaptive search.

`Off` (the default) polls the candidates in the method's own order. `CandidateOrdering` fits a smooth model to the points already evaluated (once `surrogate_min_points` exist, keeping at most `surrogate_max_points`, with smoothing length `surrogate_bandwidth`) and uses it only to decide the **order** in which poll candidates are evaluated, most promising first. Every point is still evaluated by the true function, so the answer is never wrong, and an opportunistic poll that finds its improvement earlier stops sooner. The diagnostics count the reordered polls.

## Fields

### `Off`

`enumerator`, default `0`.

No surrogate.

### `CandidateOrdering`

`enumerator`, default `1`.

Order the poll candidates by a fitted model.

## Guidance

Set as `MadsBlackboxAdvancedConfig::surrogate_mode`.

## Practical Examples

### Example 1

A weighted quadratic in 8 and 12 variables, run to convergence, so the counts compare like with like.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  for (const std::int64_t dim : {8, 12}) {
    std::vector<double> w(static_cast<std::size_t>(dim));
    for (std::int64_t i = 0; i < dim; ++i) w[static_cast<std::size_t>(i)] = static_cast<double>(i + 1);
    const Series weights = pluss::series::make(w);
    for (const auto mode : {dfo::SurrogateMode::Off, dfo::SurrogateMode::CandidateOrdering}) {
      dfo::MadsBlackboxAdvancedConfig config;
      config.surrogate_mode = mode;
      config.max_iterations = 5000;
      config.max_evaluations = 20000;
      const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = ((x - 1.0).square() * weights).sum().reshape({1}); return e; };
      const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(static_cast<std::size_t>(dim), -1.0)), config);
      out.writeln("dim", dim, mode == dfo::SurrogateMode::Off ? "Off              " : "CandidateOrdering", ": evaluations =", o.diagnostics.evaluations, ", orderings =", o.diagnostics.surrogate_orderings);
    }
  }
}
```

Output

```text
dim 8 Off               : evaluations = 5205 , orderings = 0
dim 8 CandidateOrdering : evaluations = 4678 , orderings = 424
dim 12 Off               : evaluations = 13228 , orderings = 0
dim 12 CandidateOrdering : evaluations = 10398 , orderings = 644
```

### Example 2

The number of points needed before the model is used.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const Series weights = pluss::series::make({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  for (const std::int64_t min_points : {8, 60}) {
    dfo::MadsBlackboxAdvancedConfig config;
    config.surrogate_mode = dfo::SurrogateMode::CandidateOrdering;
    config.surrogate_min_points = min_points;
    config.max_evaluations = 5000;
    const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = ((x - 1.0).square() * weights).sum().reshape({1}); return e; };
    const dfo::BlackboxAdvancedOutcome o = dfo::mads_blackbox_minimizer(black_box, pluss::series::make(std::vector<double>(8, -1.0)), config);
    out.writeln("min points", min_points, ": orderings =", o.diagnostics.surrogate_orderings, ", evaluations =", o.diagnostics.evaluations);
  }
}
```

Output

```text
min points 8 : orderings = 425 , evaluations = 4678
min points 60 : orderings = 399 , evaluations = 4530
```

### Example 3

The answer does not depend on the surrogate: both runs find the minimum.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const Series weights = pluss::series::make({1.0, 2.0, 3.0, 4.0});
  for (const auto mode : {dfo::SurrogateMode::Off, dfo::SurrogateMode::CandidateOrdering}) {
    dfo::MadsBlackboxAdvancedConfig config;
    config.surrogate_mode = mode;
    config.max_evaluations = 5000;
    const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = ((x - 1.0).square() * weights).sum().reshape({1}); return e; };
    out.writeln(static_cast<int>(mode), ": minimum =", dfo::mads_blackbox_minimizer(black_box, pluss::series::make({-1.0, -1.0, -1.0, -1.0}), config).scalarized_minimum);
  }
}
```

Output

```text
0 : minimum = 0.0000000000
1 : minimum = 0.0000000000
```

## See Also

- [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SolverStatus.html">
    <span class="label">Previous</span>
    <span class="title">SolverStatus</span>
  </a>
  <a class="next-link" href="VnsMadsConfig.html">
    <span class="label">Next</span>
    <span class="title">VnsMadsConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
