<!-- generated from RiskSpec.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::RiskSpec`

Whether an engine's `price` method also computes the Greeks (the engine-level twin of `module::Risk`).

`PriceOnly` returns just the price; `All` returns the price and the five Greeks. A `Field` translates `controls.risk` into this; when you call an engine directly you pass it as the fourth argument of `price`, and the default depends on the engine: Analytic, Lattice, Spectral, Pde and the Black-Scholes COS default to `All`; Fft, Monte Carlo and the Heston/Bates COS default to `PriceOnly`.

## Fields

### `PriceOnly`

`enumerator`, default `0`.

Price only.

### `All`

`enumerator`, default `1`.

Price and the five Greeks.

## Guidance

`Controls::riskspec`; also usable directly through `RiskSpec::price`.

## Practical Examples

### Example 1

Direct engine call with and without Greeks.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();

  fin::pricer::Analytic e;
  const auto all = e.price(inst, mdl, mkt, fin::pricer::RiskSpec::All);
  const auto only = e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly);
  out.writeln("All: delta", all.greeks.delta.items<double>(), "; PriceOnly: delta defined", only.greeks.delta.defined());
}
```

Output

```text
All: delta [0.57926] ; PriceOnly: delta defined false
```

### Example 2

The Fourier engines default to PriceOnly: ask for the Greeks explicitly.

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
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  fin::pricer::COS e;
  out.writeln("default: delta defined", e.price(inst, mdl, mkt).greeks.delta.defined(), "; All: delta", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::All).greeks.delta.items<double>());
}
```

Output

```text
default: delta defined false ; All: delta [0.64469]
```

### Example 3

The price is the same either way.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.02, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::Heston, {1.5, 0.04, 0.3, -0.7, 0.04}).as<fin::model::Heston>();

  fin::pricer::COS e;
  const double p = e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0];
  const double q = e.price(inst, mdl, mkt, fin::pricer::RiskSpec::All).price.items<double>()[0];
  out.writeln("PriceOnly", p, ", All", q);
}
```

Output

```text
PriceOnly 8.6281081031 , All 8.6281081031
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RandMethod.html">
    <span class="label">Previous</span>
    <span class="title">RandMethod</span>
  </a>
  <a class="next-link" href="SimScheme.html">
    <span class="label">Next</span>
    <span class="title">SimScheme</span>
  </a>
</div>
<!-- apiref:nav:end -->
