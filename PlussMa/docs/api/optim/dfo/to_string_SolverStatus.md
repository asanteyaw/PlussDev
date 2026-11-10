<!-- generated from to_string_SolverStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::to_string(SolverStatus)`

Returns the name of a Nelder-Mead status.

## Signature

```cpp
std::string dfo::to_string(dfo::SolverStatus status);
```

## What It Does

Returns the enumerator name as text: `"Converged"`, `"ReachedIterationBudget"` or `"ReachedEvaluationBudget"`.

## Parameters

`dfo::SolverStatus status`
: The status to name.

## Returns

The enumerator name.

## Practical Examples

### Example 1

Naming each value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(dfo::to_string(dfo::SolverStatus::Converged), dfo::to_string(dfo::SolverStatus::ReachedIterationBudget), dfo::to_string(dfo::SolverStatus::ReachedEvaluationBudget));
}
```

Output

```text
Converged ReachedIterationBudget ReachedEvaluationBudget
```

### Example 2

Naming a result's status.

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
  out.writeln("status =", dfo::to_string(o.status));
}
```

Output

```text
status = Converged
```

### Example 3

In a table.

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

  for (const std::int64_t budget : {20, 2000}) {
    dfo::NelderMeadConfig config;
    config.max_evaluations = budget;
    out.writeln("budget", budget, ":", dfo::to_string(dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).status));
  }
}
```

Output

```text
budget 20 : ReachedEvaluationBudget
budget 2000 : Converged
```

## See Also

- [`SolverStatus`](SolverStatus.md): the values

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string_PopulationSolverStatus.html">
    <span class="label">Previous</span>
    <span class="title">to_string(PopulationSolverStatus)</span>
  </a>
  <a class="next-link" href="vnsmads_blackbox_minimizer.html">
    <span class="label">Next</span>
    <span class="title">vnsmads_blackbox_minimizer</span>
  </a>
</div>
<!-- apiref:nav:end -->
