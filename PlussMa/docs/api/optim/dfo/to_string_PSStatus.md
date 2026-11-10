<!-- generated from to_string_PSStatus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::to_string(PSStatus)`

Returns the name of a pattern-search status.

## Signature

```cpp
std::string dfo::to_string(dfo::PSStatus status);
```

## What It Does

Returns the enumerator name as text, for example `"Converged"` or `"InfeasibleStationary"`.

## Parameters

`dfo::PSStatus status`
: The status to name.

## Returns

The enumerator name.

## Practical Examples

### Example 1

Naming values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(dfo::to_string(dfo::PSStatus::Converged), dfo::to_string(dfo::PSStatus::ReachedTimeBudget), dfo::to_string(dfo::PSStatus::InfeasibleStationary));
}
```

Output

```text
Converged ReachedTimeBudget InfeasibleStationary
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

  out.writeln("status =", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0})).status));
}
```

Output

```text
status = ReachedIterationBudget
```

### Example 3

In a table of budgets.

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

  for (const std::int64_t n : {30, 3000}) {
    dfo::PSConfig config;
    config.max_evaluations = n;
    out.writeln("budget", n, ":", dfo::to_string(dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).status));
  }
}
```

Output

```text
budget 30 : ReachedEvaluationBudget
budget 3000 : ReachedIterationBudget
```

## See Also

- [`PSStatus`](PSStatus.md): the values

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string_BlackboxSolverStatus.html">
    <span class="label">Previous</span>
    <span class="title">to_string(BlackboxSolverStatus)</span>
  </a>
  <a class="next-link" href="to_string_PopulationSolverStatus.html">
    <span class="label">Next</span>
    <span class="title">to_string(PopulationSolverStatus)</span>
  </a>
</div>
<!-- apiref:nav:end -->
