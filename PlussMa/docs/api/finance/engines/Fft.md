<!-- generated from Fft.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::Fft`

The Carr-Madan FFT engine for Heston and Bates: one transform prices every strike on a grid.

Dampens the call price by `e^{α k}` so that its Fourier transform exists, evaluates the model's characteristic function on a grid of `num_fft` points with step `du`, and inverts with one FFT, which yields the prices at all grid strikes at once. The strike spacing is `dk = 2π/(num_fft · du)` unless set. The integral is approximated by Simpson's or the trapezoidal rule. The `mode` chooses the algebraic form of the Heston characteristic function (the little-trap form is continuous in the branch of the complex logarithm and is the safe default). With `return_grid` the result carries the whole grid instead of just the requested options (see [`Result`](../field/Result.md)).

## Fields

### `vol_risk_premium`

`double`, default `0.0`.

Market price of volatility risk `λ`, which moves the variance dynamics to the pricing measure. Zero prices with the given parameters as they stand.

### `mode`

`pricing::HestonCfMode`, default `LittleTrap`.

`Original` or `LittleTrap` form of the characteristic function. `LittleTrap` avoids the branch-cut discontinuity at long maturities.

### `num_fft`

`std::int64_t`, default `16384`.

Points of the transform, positive (a power of two is fastest).

### `du`

`double`, default `0.1`.

Step of the integration variable `u`, positive. Smaller steps reach further in `u` at the cost of a coarser strike grid.

### `dk`

`double`, default `NaN`.

Step of the log-strike grid. NaN derives it as `2π/(num_fft · du)`.

### `alpha`

`double`, default `1.5`.

The damping factor `α` of the Carr-Madan transform, positive. Large enough to make the damped call integrable, small enough to stay stable.

### `quadrature`

`pricing::HestonFftQuadrature`, default `Simpson`.

`Simpson` or `Trapezoidal` weights for the integral.

### `return_grid`

`bool`, default `false`.

Return the price at every strike of the grid, with the strikes, instead of at the requested options.

## Guidance

`Controls::fft`; also usable directly through `Fft::price`.

## Practical Examples

### Example 1

The volatility risk premium moves the pricing measure: a positive premium lowers this call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {0.0, 0.5, 1.5}) {
    fin::pricer::Fft e;
    e.vol_risk_premium = v;
    out.writeln("vol_risk_premium", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
vol_risk_premium 0.000000 : price = 8.628108
vol_risk_premium 0.500000 : price = 8.100746
vol_risk_premium 1.500000 : price = 7.277513
```

### Example 2

The characteristic-function form: the two forms agree at moderate maturity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const auto mode : {fin::pricing::HestonCfMode::Original, fin::pricing::HestonCfMode::LittleTrap}) {
    fin::pricer::Fft e;
    e.mode = mode;
    out.writeln(mode == fin::pricing::HestonCfMode::Original ? "Original  " : "LittleTrap", ":", e.price(inst, mdl, mkt).price.items<double>()[0]);
  }
}
```

Output

```text
Original   : 8.628108103
LittleTrap : 8.628108103
```

### Example 3

The transform size.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {256, 1024, 4096, 16384}) {
    fin::pricer::Fft e;
    e.num_fft = v;
    out.writeln("num_fft", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
num_fft 256 : price = 8.629601421
num_fft 1024 : price = 8.628108103
num_fft 4096 : price = 8.628108103
num_fft 16384 : price = 8.628108103
```

### Example 4

The integration step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {0.05, 0.1, 0.25, 0.5}) {
    fin::pricer::Fft e;
    e.du = v;
    out.writeln("du", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
du 0.050000000 : price = 8.628108103
du 0.100000000 : price = 8.628108103
du 0.250000000 : price = 8.628107886
du 0.500000000 : price = 8.625423694
```

### Example 5

The strike grid step. The log-strike spacing of the returned grid follows `dk` (NaN gives `2π/(num_fft·du)`), while the price of the requested strike is not affected by it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const double dk : {std::numeric_limits<double>::quiet_NaN(), 0.005, 0.05}) {
    fin::pricer::Fft e;
    e.dk = dk;
    e.return_grid = true;
    const auto r = e.price(inst, mdl, mkt);
    const Series logk = r.strike_grid.log();
    out.writeln("dk", dk, ": grid log-strike step =", (logk.select(0, 1) - logk.select(0, 0)).item<double>());
  }
  fin::pricer::Fft plain;
  fin::pricer::Fft coarse;
  coarse.dk = 0.05;
  out.writeln("price with default dk:", plain.price(inst, mdl, mkt).price.items<double>()[0], ", with dk = 0.05:", coarse.price(inst, mdl, mkt).price.items<double>()[0]);
}
```

Output

```text
dk nan : grid log-strike step = 0.003834952
dk 0.005000000 : grid log-strike step = 0.005000000
dk 0.050000000 : grid log-strike step = 0.050000000
price with default dk: 8.628108103 , with dk = 0.05: 8.628108103
```

### Example 6

The damping factor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();
  for (const auto v : {0.5, 1.0, 1.5, 2.5}) {
    fin::pricer::Fft e;
    e.alpha = v;
    out.writeln("alpha", v, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
alpha 0.500000000 : price = 8.628103080
alpha 1.000000000 : price = 8.628108103
alpha 1.500000000 : price = 8.628108103
alpha 2.500000000 : price = 8.628108103
```

### Example 7

The quadrature rule.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  for (const auto q : {fin::pricing::HestonFftQuadrature::Simpson, fin::pricing::HestonFftQuadrature::Trapezoidal}) {
    fin::pricer::Fft e;
    e.quadrature = q;
    out.writeln(q == fin::pricing::HestonFftQuadrature::Simpson ? "Simpson    " : "Trapezoidal", ":", e.price(inst, mdl, mkt).price.items<double>()[0]);
  }
}
```

Output

```text
Simpson     : 8.628108103
Trapezoidal : 8.628108103
```

### Example 8

The grid. With `return_grid` the result is the price at every grid strike with the strikes beside it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  fin::pricer::Fft e;
  e.return_grid = true;
  const auto r = e.price(inst, mdl, mkt);
  out.writeln("grid size", r.strike_grid.numel(), ", price shape", r.price.sizes());
}
```

Output

```text
grid size 16384 , price shape [16384]
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="COS.html">
    <span class="label">Previous</span>
    <span class="title">COS</span>
  </a>
  <a class="next-link" href="HestonCfMode.html">
    <span class="label">Next</span>
    <span class="title">HestonCfMode</span>
  </a>
</div>
<!-- apiref:nav:end -->
