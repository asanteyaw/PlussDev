<!-- generated from Lattice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricer::Lattice`

The binomial-tree engine for Black-Scholes, with European, Bermudan and American exercise.

A binomial tree with `steps` time steps. The price converges to the continuous-time value as the steps grow, at rate `O(1/steps)` with the oscillation typical of trees. A tree needs the risk-neutral probability to lie in (0, 1), which fails when the volatility is too small for the carry at too few steps; the field then asks for more steps.

## Fields

### `steps`

`std::int64_t`, default `1000`.

Number of time steps, at least 1. More steps are more accurate and slower, in proportion to `steps²`.

## Guidance

`Controls::lattice`; also usable directly through `Lattice::price`.

## Practical Examples

### Example 1

Convergence to the closed form for a European option.

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
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();

  for (const std::int64_t steps : {10, 100, 1000, 10000}) {
    fin::pricer::Lattice e;
    e.steps = steps;
    out.writeln("steps", steps, ": price =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
  out.writeln("closed form (Analytic engine):", fin::pricer::Analytic{}.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
}
```

Output

```text
steps 10 : price = 8.720683
steps 100 : price = 8.896250
steps 1000 : price = 8.914056
steps 10000 : price = 8.915839
closed form (Analytic engine): 8.916037
```

### Example 2

The American put by the tree.

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
  const auto inst = fin::instrument::american(fin::pricing::Put, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.05, 0.0);
  const auto mdl = fin::model::make(fin::model::Kind::BlackScholes, {0.2}).as<fin::model::BlackScholes>();

  for (const std::int64_t steps : {50, 500, 5000}) {
    fin::pricer::Lattice e;
    e.steps = steps;
    out.writeln("steps", steps, ": American put =", e.price(inst, mdl, mkt, fin::pricer::RiskSpec::PriceOnly).price.items<double>()[0]);
  }
}
```

Output

```text
steps 50 : American put = 6.07373
steps 500 : American put = 6.08881
steps 5000 : American put = 6.09022
```

### Example 3

Too few steps for the volatility and carry: refused with advice.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace mod = fin::module;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto inst = fin::instrument::make(fin::pricing::Call, 100.0, 1.0);
  const auto mkt = fin::market::make(100.0, 0.5, 0.0);
  const fin::model::BlackScholes mdl{.volatility = pluss::make_series({0.01})};
  fin::pricer::Lattice e;
  e.steps = 2;
  try {
    (void)(e.price(inst, mdl, mkt));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::pricing::lattice_price: the risk-neutral probability left (0,1); use more steps or a larger volatility
```

## See Also

- [`Controls`](../field/Controls.md): holds one of each
- [`Engine`](../field/Engine.md): the engines

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="HestonFftQuadrature.html">
    <span class="label">Previous</span>
    <span class="title">HestonFftQuadrature</span>
  </a>
  <a class="next-link" href="MonteCarlo.html">
    <span class="label">Next</span>
    <span class="title">MonteCarlo</span>
  </a>
</div>
<!-- apiref:nav:end -->
