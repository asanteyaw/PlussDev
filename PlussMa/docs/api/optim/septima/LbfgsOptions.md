<!-- generated from LbfgsOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaOptions::LbfgsOptions`

Settings of the L-BFGS optimizer: step scale, memory, curvature test and line search.

Read when `SeptimaOptions::optimizer` is `Lbfgs`, as `options.step.lbfgs`. `history_size` is the number of step pairs kept: more memory follows narrow curved valleys better at the cost of more work per step. `curvature_tolerance` is the smallest curvature `sᵀy` for which a pair is stored, which keeps the estimate positive definite. `learning_rate` scales the quasi-Newton step (1 is the natural step). With `line_search = StrongWolfe` the step is chosen to satisfy the strong Wolfe conditions. `tolerance_change` ends the run when the objective or step change is below it.

## Fields

### `learning_rate`

`double`, default `1.0`.

Scale of the quasi-Newton step.

### `history_size`

`std::int64_t`, default `20`.

How many step pairs the memory keeps.

### `curvature_tolerance`

`double`, default `1e-10`.

A pair with curvature `sᵀy` below this is not stored.

### `max_line_search_iterations`

`std::int64_t`, default `25`.

The most trial evaluations of the line search per iteration.

### `tolerance_change`

`double`, default `1e-9`.

The run ends when the objective or step change falls below this.

### `wolfe_c1`

`double`, default `1e-4`.

The sufficient-decrease constant of the line search.

### `wolfe_c2`

`double`, default `0.9`.

The curvature constant of the line search: a step is accepted when the slope along the direction has shrunk to at most this fraction of its starting size.

### `line_search`

`optim::SeptimaLbfgsLineSearch`, default `None`.

Whether steps are shortened by a line search.

## Guidance

Set as `SeptimaOptions::lbfgs`.

## Practical Examples

### Example 1

Memory. On the ten-variable chained Rosenbrock function a longer memory follows the valley in fewer iterations than a short one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  std::vector<double> head_index, tail_index, start;
  for (int i = 0; i < 9; ++i) { head_index.push_back(i); tail_index.push_back(i + 1); }
  for (int i = 0; i < 10; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const Series head = pluss::make_series(head_index, {9}).to(pluss::ScalarType::Int64);
  const Series tail = pluss::make_series(tail_index, {9}).to(pluss::ScalarType::Int64);
  const auto chain = [&](const Series &x) {
    const Series a = x.index_select(0, head), b = x.index_select(0, tail);
    return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
  };
  for (const std::int64_t memory : {2, 20}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.history_size = memory;
    options.max_iterations = 2000;
    const optim::SeptimaResult result = optim::septima(chain, pluss::make_series(start, {10}), options);
    out.writeln("history", memory, ": exit =", optim::septima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", objective =", result.objective_value);
  }
}
```

Output

```text
history 2 : exit = Converged , iterations = 201 , objective = 0.00000000
history 20 : exit = Converged , iterations = 108 , objective = 0.00000000
```

### Example 2

The learning rate scales the quasi-Newton step; on this quadratic a half step still converges, in about the same number of iterations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto objective = [](const Series &x) { return (x - 3.0).square().sum(); };

  for (const double rate : {1.0, 0.5}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.learning_rate = rate;
    options.max_iterations = 500;
    const optim::SeptimaResult result = optim::septima(objective, pluss::series::make({0.0, 0.0}), options);
    out.writeln("learning rate", rate, ": iterations =", result.iteration, ", exit =", optim::septima_exit_flag_name(result.exit_flag));
  }
}
```

Output

```text
learning rate 1.0000 : iterations = 3 , exit = Converged
learning rate 0.5000 : iterations = 2 , exit = Converged
```

### Example 3

Reading the defaults.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const optim::SeptimaOptions::LbfgsOptions defaults;
  out.writeln("history_size =", defaults.history_size, ", learning_rate =", defaults.learning_rate, ", max_line_search_iterations =", defaults.max_line_search_iterations);
}
```

Output

```text
history_size = 20 , learning_rate = 1.0000 , max_line_search_iterations = 25
```

## See Also

- [`SeptimaLbfgsLineSearch`](SeptimaLbfgsLineSearch.md): the line-search choice
- [`SeptimaOptions`](SeptimaOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="AdamState.html">
    <span class="label">Previous</span>
    <span class="title">AdamState</span>
  </a>
  <a class="next-link" href="Lsr1ExitFlag.html">
    <span class="label">Next</span>
    <span class="title">Lsr1ExitFlag</span>
  </a>
</div>
<!-- apiref:nav:end -->
