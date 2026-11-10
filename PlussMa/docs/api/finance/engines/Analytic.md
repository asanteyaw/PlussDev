<!-- generated from Analytic.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::Analytic`

The closed-form engine: the Black-Scholes formula, or the SABR expansion priced through Black or Bachelier.

Prices European exercise in closed form for Black-Scholes and SABR. It is exact for Black-Scholes and an asymptotic approximation for SABR (see [`Sabr`](../model/Sabr.md) for the formula choices). There is one setting. The engine is the default of `controls.engine`.

## Fields

### `with_greeks`

`bool`, default `true`.

Whether the Black-Scholes Greeks are computed in closed form along with the price when the risk setting asks for them.

## Guidance

`Controls::analytic`; also usable directly through `Analytic::price`.

## Practical Examples

### Example 1

Direct call on Black-Scholes: the price agrees with the closed-form formula evaluated by hand.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();

  fin::pricer::Analytic e;
  const double d1 = (0.02 + 0.5 * 0.04) / 0.2, d2 = d1 - 0.2;
  const auto N = [](double x) { return 0.5 * std::erfc(-x / std::sqrt(2.0)); };
  out.writeln("engine:", e.price(inst, mdl, mkt).price.items<double>()[0], ", closed form:", 100.0 * N(d1) - 100.0 * std::exp(-0.02) * N(d2));
}
```

Output

```text
engine: 8.91603728 , closed form: 8.91603728
```

### Example 2

SABR through the same engine.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 110.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Sabr, {0.2, 1.0, 0.4, -0.3}).as<fin::model::Sabr>();

  fin::pricer::Analytic e;
  out.writeln("SABR call, K = 110:", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
}
```

Output

```text
SABR call, K = 110: 4.838623
```

### Example 3

The `with_greeks` setting.

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
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();

  for (const bool flag : {true, false}) {
    fin::pricer::Analytic e;
    e.with_greeks = flag;
    const auto r = e.price(inst, mdl, mkt, fin::pricer::RiskSpec::All);
    out.writeln("with_greeks", flag, ": delta defined", r.greeks.delta.defined());
  }
}
```

Output

```text
with_greeks true : delta defined true
with_greeks false : delta defined false
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="COS.html">
    <span class="label">Next</span>
    <span class="title">COS</span>
  </a>
</div>
<!-- apiref:nav:end -->
