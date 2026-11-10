<!-- generated from NelderMeadDiagnostics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::NelderMeadDiagnostics`

The counters of a Nelder-Mead run.

`iterations` counts simplex steps, `evaluations` counts objective calls, and `last_step` names the last move: `seed` (building the simplex), `reflect`, `expand`, `contract_outside`, `contract_inside`, `shrink` or `adjust_inner`.

## Fields

### `iterations`

`std::int64_t`, default `0`.

Simplex steps taken.

### `evaluations`

`std::int64_t`, default `0`.

Objective evaluations.

### `last_step`

`std::string`, default `empty`.

Name of the last move.

## Guidance

Read from `NelderMeadOutcome::diagnostics`.

## Practical Examples

### Example 1

After a converged run.

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
  out.writeln("iterations =", o.diagnostics.iterations, ", evaluations =", o.diagnostics.evaluations);
}
```

Output

```text
iterations = 85 , evaluations = 159
```

### Example 2

The last step after a short run.

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

  dfo::NelderMeadConfig config;
  config.max_iterations = 5;
  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln("last step =", o.diagnostics.last_step, ", iterations =", o.diagnostics.iterations);
}
```

Output

```text
last step = adjust_inner , iterations = 5
```

### Example 3

Evaluations per iteration. A simplex iteration costs one or two evaluations, a shrink costs `n`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  const dfo::NelderMeadOutcome o = dfo::nelder_mead_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}));
  out.writeln("evaluations per iteration =", static_cast<double>(o.diagnostics.evaluations) / static_cast<double>(o.diagnostics.iterations));
}
```

Output

```text
evaluations per iteration = 1.871
```

## See Also

- [`NelderMeadOutcome`](NelderMeadOutcome.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="NelderMeadConfig.html">
    <span class="label">Previous</span>
    <span class="title">NelderMeadConfig</span>
  </a>
  <a class="next-link" href="NelderMeadOutcome.html">
    <span class="label">Next</span>
    <span class="title">NelderMeadOutcome</span>
  </a>
</div>
<!-- apiref:nav:end -->
