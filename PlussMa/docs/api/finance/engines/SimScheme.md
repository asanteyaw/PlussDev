<!-- generated from SimScheme.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `volatility::SimScheme`

The time-stepping scheme for the variance in simulated stochastic-volatility paths.

`Euler` steps the variance with the variance at the start of the step and floors it; `Milstein` and `Milstein2` add correction terms of first and second order in the noise; `QuadExp` is Andersen's quadratic-exponential scheme, which matches the first two moments of the exact transition and is accurate with few steps; `Transition` samples the exact (noncentral chi-square) transition of the variance. Higher-order schemes need fewer steps for the same bias, at more cost per step. The asset step is exact given the variance path in the schemes that need it.

## Fields

### `Euler`

`enumerator`, default `0`.

Euler step with a floor.

### `Milstein`

`enumerator`, default `1`.

Milstein correction.

### `Milstein2`

`enumerator`, default `2`.

Second-order Milstein.

### `QuadExp`

`enumerator`, default `3`.

Andersen's quadratic-exponential scheme. Each step is either quadratic (a squared normal) or exponential (a point mass at zero plus an exponential tail), depending on how spread the variance is. Both branches are driven by the same correlated normal shock of the step, the exponential one through its normal distribution function, so the variance keeps its correlation `ρ` with the price in every step, and, in a `Chord`, its correlation with the other assets' shocks.

### `Transition`

`enumerator`, default `4`.

Exact transition sampling.

## Guidance

Set as `MonteCarlo::scheme`.

## Practical Examples

### Example 1

The five schemes with few steps (4 steps, 400,000 paths, so a sampling error of about 0.03). Euler is the most biased at about +0.16 against the COS price 8.628; the other four are within about 0.07 of it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const auto scheme : {fin::volatility::SimScheme::Euler, fin::volatility::SimScheme::Milstein, fin::volatility::SimScheme::Milstein2, fin::volatility::SimScheme::QuadExp, fin::volatility::SimScheme::Transition}) {
    fin::pricer::MonteCarlo e;
    e.scheme = scheme;
    e.paths = 400000;
    e.steps = 4;
    e.seed = 9;
    out.writeln(static_cast<int>(scheme), ":", e.price(inst, mdl, mkt).price.items<double>()[0]);
  }
}
```

Output

```text
0 : 8.7851
1 : 8.6078
2 : 8.5963
3 : 8.6951
4 : 8.6727
```

### Example 2

Simulated variance paths under two schemes with the same shocks.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};

  field.controls.mc.paths = 20000;
  field.controls.mc.steps = 20;
  field.controls.mc.seed = 3;
  for (const auto scheme : {fin::volatility::SimScheme::Euler, fin::volatility::SimScheme::QuadExp}) {
    field.controls.mc.scheme = scheme;
    const Series v = field.simulate_paths().variance.select(1, 20);
    out.writeln(static_cast<int>(scheme), ": mean variance at T =", v.mean().item<double>(), ", exact mean =", 0.04);
  }
}
```

Output

```text
0 : mean variance at T = 0.03985 , exact mean = 0.04000
3 : mean variance at T = 0.03960 , exact mean = 0.04000
```

### Example 3

A scheme through a Field.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mod::Field field{.instrument = fin::instrument::make(fin::pricing::Call, 100.0, 1.0),
                   .market = fin::market::make(100.0, 0.02, 0.0),
                   .model = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}),
                   .controls = {.engine = mod::Engine::MonteCarlo, .risk = mod::Risk::PriceOnly}};

  field.controls.mc.paths = 100000;
  field.controls.mc.steps = 8;
  field.controls.mc.seed = 3;
  field.controls.mc.scheme = fin::volatility::SimScheme::Milstein;
  out.writeln("Milstein:", field.price().price.items<double>()[0]);
}
```

Output

```text
Milstein: 8.6441
```

## See Also

- [`MonteCarlo`](MonteCarlo.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RiskSpec.html">
    <span class="label">Previous</span>
    <span class="title">RiskSpec</span>
  </a>
  <a class="next-link" href="Spectral.html">
    <span class="label">Next</span>
    <span class="title">Spectral</span>
  </a>
</div>
<!-- apiref:nav:end -->
