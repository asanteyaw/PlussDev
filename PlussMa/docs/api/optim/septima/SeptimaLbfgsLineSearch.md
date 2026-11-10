<!-- generated from SeptimaLbfgsLineSearch.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::SeptimaLbfgsLineSearch`

Whether L-BFGS steps are shortened by a line search.

`None` (the default) takes the L-BFGS step scaled by `learning_rate`. `StrongWolfe` searches for a step along the L-BFGS direction that satisfies the strong Wolfe conditions: a sufficient decrease of the objective (constant `wolfe_c1`) and a slope along the direction that has shrunk to at most `wolfe_c2` times its starting size. It starts at `learning_rate`, lengthens a step that is still descending steeply, and narrows a bracket around an acceptable step by interpolation, using at most `max_line_search_iterations` trial evaluations; if they run out, the best step that gave a sufficient decrease is taken. The search protects against steps that overshoot, as on a steep quartic, and chooses well-scaled steps on a curved valley such as the Rosenbrock function; it costs extra objective evaluations per iteration. When the problem has equality or inequality constraints and `StrongWolfe` is not selected, the augmented objective is not smooth enough for unguarded steps and a backtracking search on its value is used instead.

## Fields

### `None`

`enumerator`, default `0`.

Take the L-BFGS step as is (scaled by the learning rate).

### `StrongWolfe`

`enumerator`, default `1`.

Search for a step satisfying the strong Wolfe conditions.

## Guidance

Set as `LbfgsOptions::line_search`.

## Practical Examples

### Example 1

A steep objective. For `Σ xᵢ⁴ + (0.1 Σ xᵢ)²` from `(2, 2)` the unguarded first step overshoots; the line search finds a safe length.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto quartic = [](const Series &x) { return x.pow(4).sum() + (x * 0.1).sum().square(); };
  for (const optim::SeptimaLbfgsLineSearch search : {optim::SeptimaLbfgsLineSearch::None, optim::SeptimaLbfgsLineSearch::StrongWolfe}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.line_search = search;
    const optim::SeptimaResult result = optim::septima(quartic, pluss::series::make({2.0, 2.0}), options);
    out.writeln(search == optim::SeptimaLbfgsLineSearch::None ? "None       " : "StrongWolfe", ": iterations =", result.iteration, ", evaluations =", result.function_evaluations, ", objective =", result.objective_value);
  }
}
```

Output

```text
None        : iterations = 20 , evaluations = 20 , objective = 0.000000
StrongWolfe : iterations = 4 , evaluations = 22 , objective = 0.000000
```

### Example 2

A curved valley. Both reach the minimum of the Rosenbrock function at `(1, 1)`; the line search spends more evaluations per iteration and needs far fewer iterations.

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
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (1.0 - a).square() + 100.0 * (b - a.square()).square();
  };

  for (const optim::SeptimaLbfgsLineSearch search : {optim::SeptimaLbfgsLineSearch::None, optim::SeptimaLbfgsLineSearch::StrongWolfe}) {
    optim::SeptimaSolveOptions options;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.line_search = search;
    options.max_iterations = 2000;
    const optim::SeptimaResult result = optim::septima(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
    out.writeln(search == optim::SeptimaLbfgsLineSearch::None ? "None       " : "StrongWolfe", ": exit =", optim::septima_exit_flag_name(result.exit_flag), ", iterations =", result.iteration, ", x =", result.params[0].items<double>());
  }
}
```

Output

```text
None        : exit = Converged , iterations = 81 , x = [1.0000, 1.0000]
StrongWolfe : exit = Converged , iterations = 39 , x = [1.0000, 1.0000]
```

### Example 3

Limiting the search. The first step along the raw gradient of the Rosenbrock function is very long; with only 5 trial evaluations no trial is short enough to be accepted, and the run cannot move from its start.

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
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (1.0 - a).square() + 100.0 * (b - a.square()).square();
  };

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  options.step.lbfgs.max_line_search_iterations = 5;
  const optim::SeptimaResult result = optim::septima(rosenbrock, pluss::series::make({-1.2, 1.0}), options);
  out.writeln("exit =", optim::septima_exit_flag_name(result.exit_flag), ", x =", result.params[0].items<double>());
}
```

Output

```text
exit = NoProgress , x = [-1.2000, 1.0000]
```

## See Also

- [`LbfgsOptions`](LbfgsOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SeptimaExitFlag.html">
    <span class="label">Previous</span>
    <span class="title">SeptimaExitFlag</span>
  </a>
  <a class="next-link" href="SeptimaOptimizer.html">
    <span class="label">Next</span>
    <span class="title">SeptimaOptimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
