<!-- generated from vnsmads_blackbox_minimizer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::vnsmads_blackbox_minimizer`

Minimises a black box with several basins with variable-neighbourhood MADS, which kicks the best point and searches again.

## Signature

```cpp
dfo::BlackboxAdvancedOutcome dfo::vnsmads_blackbox_minimizer(const std::function<dfo::BlackboxEvaluation(const pluss::Series &)> &blackbox, const pluss::Series &initial, const dfo::VnsMadsConfig &config = {});
template <class Fn> dfo::BlackboxAdvancedOutcome dfo::vnsmads_blackbox_minimizer(Fn &&blackbox, const pluss::Series &initial, const dfo::VnsMadsConfig &config = {});
```

## What It Does

It runs MADS from the start, then repeatedly *kicks* the best point by a random perturbation (a normal draw of scale `initial_kick_scale`, multiplied by `kick_growth` at each stage) and runs MADS again from there, keeping the best outcome. `neighborhoods` is the number of stages; the evaluation budget of `base` is divided among them. The kicks let it leave a basin that plain MADS would settle in, which suits multimodal functions.

## Parameters

`Fn &&blackbox`
: A callable returning a `BlackboxEvaluation`, a `Series` of objectives or a number.

`const pluss::Series &initial`
: The starting point.

`const dfo::VnsMadsConfig &config`
: The settings.

## Returns

A [`BlackboxAdvancedOutcome`](BlackboxAdvancedOutcome.md).

## Errors

- A black box that returns neither objectives nor a failure flag throws on the first call.

## Practical Examples

### Example 1

The two-variable Rastrigin function in a box, from a start in a distant basin.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto black_box_value = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = black_box_value(x).reshape({1}); return e; };
  dfo::VnsMadsConfig config;
  config.base.lower_bounds = pluss::series::make({-5.12, -5.12});
  config.base.upper_bounds = pluss::series::make({5.12, 5.12});
  config.base.max_evaluations = 9000;
  config.neighborhoods = 6;
  config.initial_kick_scale = 1.0;
  const dfo::BlackboxAdvancedOutcome o = dfo::vnsmads_blackbox_minimizer(black_box, pluss::series::make({3.2, 2.7}), config);
  out.writeln("minimizer =", o.minimizer.items<double>(), ", minimum =", o.scalarized_minimum);
}
```

Output

```text
minimizer = [-0.00000, -0.00000] , minimum = 0.00000
```

### Example 2

Plain MADS settles in the first basin it reaches; VNS-MADS escapes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto black_box_value = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = black_box_value(x).reshape({1}); return e; };
  const Series lo = pluss::series::make({-5.12, -5.12}), hi = pluss::series::make({5.12, 5.12});
  dfo::MadsBlackboxAdvancedConfig plain;
  plain.lower_bounds = lo;
  plain.upper_bounds = hi;
  plain.max_evaluations = 9000;
  dfo::VnsMadsConfig vns;
  vns.base = plain;
  vns.neighborhoods = 6;
  vns.initial_kick_scale = 1.0;
  out.writeln("plain MADS:", dfo::mads_blackbox_minimizer(black_box, pluss::series::make({3.2, 2.7}), plain).scalarized_minimum);
  out.writeln("VNS-MADS  :", dfo::vnsmads_blackbox_minimizer(black_box, pluss::series::make({3.2, 2.7}), vns).scalarized_minimum);
}
```

Output

```text
plain MADS: 0.99496
VNS-MADS  : 0.00000
```

### Example 3

The number of stages.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto black_box_value = [](const Series &x) {
    const double pi = 3.14159265358979323846;
    return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();   // minimum 0 at the origin, many local minima
  };

  const auto black_box = [&](const Series &x) { dfo::BlackboxEvaluation e; e.objectives = black_box_value(x).reshape({1}); return e; };
  for (const std::int64_t stages : {1, 3, 12}) {
    dfo::VnsMadsConfig config;
    config.base.lower_bounds = pluss::series::make({-5.12, -5.12});
    config.base.upper_bounds = pluss::series::make({5.12, 5.12});
    config.base.max_evaluations = 9000;
    config.neighborhoods = stages;
    config.initial_kick_scale = 1.0;
    out.writeln("stages", stages, ": minimum =", dfo::vnsmads_blackbox_minimizer(black_box, pluss::series::make({3.2, 2.7}), config).scalarized_minimum);
  }
}
```

Output

```text
stages 1 : minimum = 0.99496
stages 3 : minimum = 0.99496
stages 12 : minimum = 0.00000
```

## See Also

- [`VnsMadsConfig`](VnsMadsConfig.md): the settings
- [`BlackboxEvaluation`](BlackboxEvaluation.md): what the black box returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string_SolverStatus.html">
    <span class="label">Previous</span>
    <span class="title">to_string(SolverStatus)</span>
  </a>
</div>
<!-- apiref:nav:end -->
