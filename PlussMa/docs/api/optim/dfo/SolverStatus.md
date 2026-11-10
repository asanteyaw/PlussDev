<!-- generated from SolverStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::SolverStatus`

Why a Nelder-Mead run stopped.

`Converged` means the simplex became smaller than `position_tolerance` with objective values that agree to `value_tolerance`. `ReachedIterationBudget` and `ReachedEvaluationBudget` mean `max_iterations` or `max_evaluations` ran out first; the best vertex is still returned. The front door maps these onto [`DfoStatus`](DfoStatus.md).

## Fields

### `Converged`

`enumerator`, default `0`.

The simplex converged.

### `ReachedIterationBudget`

`enumerator`, default `1`.

`max_iterations` ran out.

### `ReachedEvaluationBudget`

`enumerator`, default `2`.

`max_evaluations` ran out.

## Guidance

Read from `NelderMeadOutcome::status`.

## Practical Examples

### Example 1

Each outcome.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::NelderMeadConfig few_iterations, few_evaluations;
  few_iterations.max_iterations = 5;
  few_evaluations.max_evaluations = 30;
  out.writeln("default       :", dfo::to_string(dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).status));
  out.writeln("5 iterations  :", dfo::to_string(dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), few_iterations).status));
  out.writeln("30 evaluations:", dfo::to_string(dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), few_evaluations).status));
}
```

Output

```text
default       : Converged
5 iterations  : ReachedIterationBudget
30 evaluations: ReachedEvaluationBudget
```

### Example 2

Acting on the status.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  switch (o.status) {
    case dfo::SolverStatus::Converged: out.writeln("converged"); break;
    case dfo::SolverStatus::ReachedIterationBudget: out.writeln("out of iterations"); break;
    case dfo::SolverStatus::ReachedEvaluationBudget: out.writeln("out of evaluations"); break;
  }
}
```

Output

```text
converged
```

### Example 3

The numeric values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(static_cast<int>(dfo::SolverStatus::Converged), static_cast<int>(dfo::SolverStatus::ReachedIterationBudget), static_cast<int>(dfo::SolverStatus::ReachedEvaluationBudget));
}
```

Output

```text
0 1 2
```

## See Also

- [`NelderMeadOutcome`](NelderMeadOutcome.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SolverDisplay.html">
    <span class="label">Previous</span>
    <span class="title">SolverDisplay</span>
  </a>
  <a class="next-link" href="SurrogateMode.html">
    <span class="label">Next</span>
    <span class="title">SurrogateMode</span>
  </a>
</div>
<!-- apiref:nav:end -->
