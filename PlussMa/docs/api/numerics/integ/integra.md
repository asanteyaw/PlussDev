<!-- generated from integra.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::integra`

Integrates a function over an interval, choosing between quadrature and Monte Carlo itself.

## Signature

```cpp
integ::IntegraResult integ::integra(const integ::IntegrandFn &fn, double a, double b, const integ::IntegraOptions &options = {});
```

## What It Does

The convenience entry point when you do not want to choose. With the default `Auto` strategy it tries [`quadra`](quadra.md) first, which converges quickly and tightly for a smooth integrand, and only if quadrature fails to converge within its refinement budget does it run [`aleatoria`](aleatoria.md), whose convergence does not depend on smoothness. It then returns whichever answer has the smaller error estimate, and says which one it used. It cannot look inside `fn`; the fallback is triggered only by the observed failure of quadrature to converge. `Quadrature` and `MonteCarlo` force one engine.

The result is uniform: a value, an error estimate (the quadrature change or the Monte Carlo standard error), the evaluations spent, the strategy used and whether it converged. For many dimensions use `aleatoria` directly; `integra` is for one interval.

## Parameters

`const integ::IntegrandFn &fn`
: `Series (Series)`.

`double a, double b`
: The interval; either may be infinite.

`const integ::IntegraOptions &options`
: Strategy and the settings of each engine.

## Returns

An [`IntegraResult`](IntegraResult.md).

## Errors

- The errors of `quadra` and `aleatoria` apply.

## Practical Examples

### Example 1

A smooth integrand. Quadrature converges, so no Monte Carlo is run. `∫₀¹ cos x dx = sin 1 = 0.84147098`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const integ::IntegraResult r = integ::integra([](const Series &t) { return t.cos(); }, 0.0, 1.0);
  out.writeln("value =", r.value.item<double>(), ", strategy used =", static_cast<int>(r.strategy_used), "(0 is Quadrature), evaluations =", r.evaluations);
}
```

Output

```text
value = 0.8414709848 , strategy used = 0 (0 is Quadrature), evaluations = 774
```

### Example 2

An integrand quadrature cannot finish. A fine oscillation with a tight tolerance and a small budget does not converge, so `integra` falls back to Monte Carlo and keeps the better estimate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  integ::IntegraOptions options;
  options.quadrature.tolerance = 1e-14;
  options.quadrature.max_refinements = 1;
  options.quadrature.nodes = 9;
  options.monte_carlo.samples = 20000;
  options.monte_carlo.seed = 3;
  const integ::IntegraResult r = integ::integra([](const Series &t) { return (t * 200.0).sin().square(); }, 0.0, 1.0, options);
  out.writeln("value =", r.value.item<double>(), ", strategy used =", static_cast<int>(r.strategy_used), ", converged =", r.converged, ", error estimate =", r.error_estimate);
}
```

Output

```text
value = 0.504121 , strategy used = 1 , converged = true , error estimate = 0.002503
```

### Example 3

Forcing an engine. The same integral by each strategy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  for (const integ::IntegraStrategy strategy : {integ::IntegraStrategy::Quadrature, integ::IntegraStrategy::MonteCarlo}) {
    integ::IntegraOptions options;
    options.strategy = strategy;
    options.monte_carlo.samples = 50000;
    options.monte_carlo.seed = 1;
    const integ::IntegraResult r = integ::integra([](const Series &t) { return (t * -1.0).exp(); }, 0.0, 2.0, options);
    out.writeln(static_cast<int>(strategy), ": value =", r.value.item<double>(), ", error estimate =", r.error_estimate, ", evaluations =", r.evaluations);
  }
}
```

Output

```text
0 : value = 0.86466472 , error estimate = 0.00000000 , evaluations = 774
1 : value = 0.86816198 , error estimate = 0.00216593 , evaluations = 50000
```

## See Also

- [`IntegraOptions`](IntegraOptions.md): the settings
- [`quadra`](quadra.md): the quadrature engine
- [`aleatoria`](aleatoria.md): the Monte Carlo engine

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumulative_trapezoid.html">
    <span class="label">Previous</span>
    <span class="title">cumulative_trapezoid</span>
  </a>
  <a class="next-link" href="lebesgue.html">
    <span class="label">Next</span>
    <span class="title">lebesgue</span>
  </a>
</div>
<!-- apiref:nav:end -->
