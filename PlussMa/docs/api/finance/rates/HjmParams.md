<!-- generated from HjmParams.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::HjmParams`

Parameters of the one-factor Heath-Jarrow-Morton forward-rate model: the starting level, the tenor grid and the volatility at each tenor.

The forward curve is followed on a grid of times to maturity (`tenors`), starting flat at `r0`, with one volatility per tenor and one shared source of noise. The dynamics are the arbitrage-free Musiela form: the curve is transported along the tenor axis and carries the no-arbitrage drift `σ(τ)∫₀^τ σ`, so that a flat starting curve prices zero-coupon bonds at `e^{−r₀T}`. The simulation step must not exceed the smallest tenor spacing.

## Fields

### `r0`

`double`, default `0.03`.

The initial forward rate at every tenor: a flat curve.

### `tenors`

`std::vector<double>`, default `{0.25, 0.5, 1, 2, 3, 5}`.

The grid in years: positive, strictly increasing.

### `vol`

`std::vector<double>`, default `{0.012, 0.011, 0.010, 0.009, 0.008, 0.007}`.

Volatility at each tenor, non-negative, one per tenor.

## Guidance

Passed to the `hjm` path and bond functions.

## Practical Examples

### Example 1

The initial level: the first row of every forward curve is flat at `r0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  for (const double r0 : {0.01, 0.03, 0.06}) {
    rt::HjmParams p;
    p.r0 = r0;
    out.writeln("r0", r0, ": curve at t = 0 =", rt::hjm_paths(1, 2, 0.01, p).forward.select(0, 0).select(0, 0).items<double>());
  }
}
```

Output

```text
r0 0.010 : curve at t = 0 = [0.010, 0.010, 0.010, 0.010, 0.010, 0.010]
r0 0.030 : curve at t = 0 = [0.030, 0.030, 0.030, 0.030, 0.030, 0.030]
r0 0.060 : curve at t = 0 = [0.060, 0.060, 0.060, 0.060, 0.060, 0.060]
```

### Example 2

The tenor grid: three tenors give three columns of the forward array. The grid must be strictly increasing.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  rt::HjmParams p;
  p.tenors = {0.5, 1.0, 3.0};
  p.vol = {0.012, 0.010, 0.008};
  out.writeln("forward shape =", rt::hjm_paths(10, 4, 0.05, p).forward.sizes());
  p.tenors = {1.0, 0.5, 3.0};
  try {
    (void)(rt::hjm_paths(10, 4, 0.05, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
forward shape = [11, 4, 3]
subsp::math::finance::rates::hjm_paths: tenors must be strictly increasing
```

### Example 3

The volatilities: scaling them scales the spread of the curve after a year.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const double scale : {0.5, 1.0, 2.0}) {
    rt::HjmParams p;
    for (double &v : p.vol) v *= scale;
    const rt::HjmPaths sim = rt::hjm_paths(52, 20000, 1.0 / 52.0, p);
    out.writeln("scale", scale, ": sd of the 1-year forward after a year =", sim.forward.select(0, 52).select(1, 2).std().item<double>());
  }
}
```

Output

```text
scale 0.500000 : sd of the 1-year forward after a year = 0.004772
scale 1.000000 : sd of the 1-year forward after a year = 0.009544
scale 2.000000 : sd of the 1-year forward after a year = 0.019088
```

### Example 4

Zero volatility: nothing is random and nothing drifts; the flat curve stays flat.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

static double bs(fin::pricing::OptionType type, double S, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price.items<double>()[0];
}
static pluss::Series bsv(fin::pricing::OptionType type, const pluss::Series &spots, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static pluss::Series bst(fin::pricing::OptionType type, const pluss::Series &spots, const pluss::Series &tau, double K, double r, double q, double vol) {
  fin::module::Field f{.instrument = {.type = type, .strike = pluss::make_series({K}), .maturity = tau}, .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static fin::hedging::DeltaHedgeResult hedge(const pluss::Series &path, fin::pricing::OptionType type, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(100.0, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic}};
  return fin::hedging::delta_hedge(f, path);
}
static double iv(double price, double S, double K, double r, double q, double T, const fin::pricing::ImpliedVolOptions &o = {}) {
  fin::module::Field f{.instrument = fin::instrument::make(fin::pricing::Call, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.implied_vol(pluss::make_series({price}), o).items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  rt::HjmParams p;
  p.vol = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const rt::HjmPaths sim = rt::hjm_paths(52, 4, 1.0 / 52.0, p);
  out.writeln("largest deviation from r0 after a year:", (sim.forward.select(0, 52) - 0.03).abs().max().item<double>());
}
```

Output

```text
largest deviation from r0 after a year: 0.000000000000
```

### Example 5

Mismatched lengths are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  rt::HjmParams p;
  p.tenors = {0.5, 1.0};
  p.vol = {0.012};
  try {
    (void)(rt::hjm_paths(10, 4, 0.05, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::hjm_paths: tenors and vol size must match
```

## See Also

- [`SimOptions`](SimOptions.md): the simulation settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CirParams.html">
    <span class="label">Previous</span>
    <span class="title">CirParams</span>
  </a>
  <a class="next-link" href="HjmPaths.html">
    <span class="label">Next</span>
    <span class="title">HjmPaths</span>
  </a>
</div>
<!-- apiref:nav:end -->
