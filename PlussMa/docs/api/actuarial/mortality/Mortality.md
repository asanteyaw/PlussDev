<!-- generated from Mortality.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Mortality`

A one-year mortality law over a finite horizon, with every derived quantity: survival, force, the distribution of the lifetime and the life table.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::from_qx(const pluss::Series &qx);
actuarial::Mortality actuarial::mortality::from_px(const pluss::Series &px);
actuarial::Mortality actuarial::mortality::from_force(const pluss::Series &force);
actuarial::Mortality actuarial::mortality::from_lx(const pluss::Series &lx);
bool actuarial::Mortality::defined() const noexcept;
std::int64_t actuarial::Mortality::horizon() const noexcept;
const pluss::Series &actuarial::Mortality::qx() const noexcept;
pluss::Series actuarial::Mortality::px() const;
pluss::Series actuarial::Mortality::force() const;
pluss::Series actuarial::Mortality::survival() const;
pluss::Series actuarial::Mortality::lifetime_pmf() const;
pluss::Series actuarial::Mortality::life_table(double radix = 100000.0) const;
```

## Construction

A `Mortality` is made by a factory and cannot be built or edited directly. The factories take the same law in four representations, and the value stores it as the one-year death probabilities {math}`q_{x+k}` for `k = 0, ..., n−1`, where `n` is the **horizon**: [`from_qx`](from_qx.md) takes them as they are, [`from_px`](from_px.md) takes the survival probabilities {math}`p=1-q`, [`from_force`](from_force.md) takes the forces of mortality {math}`\mu` with {math}`q=1-e^{-\mu}`, and [`from_lx`](from_lx.md) takes the lives of a life table. [`select_ultimate`](select_ultimate.md) joins a select table to ultimate rates, and [`gompertz`](gompertz.md) and [`makeham`](makeham.md) generate the law from a parametric force. A default-constructed `Mortality` is undefined.

A mortality law is half of a [`Basis`](../valuation/Basis.md); the other half is a [`Discount`](../valuation/Discount.md).

## Methods

| Method | What it does |
|---|---|
| [`defined`](Mortality/defined.md) | Tells whether the law was built by a factory. |
| [`horizon`](Mortality/horizon.md) | The number of years `n` the law covers. |
| [`qx`](Mortality/qx.md) | The one-year death probabilities, the stored form. |
| [`px`](Mortality/px.md) | The one-year survival probabilities. |
| [`force`](Mortality/force.md) | The forces of mortality {math}`\mu=-\ln(1-q)`. |
| [`survival`](Mortality/survival.md) | The probabilities of surviving `0..n` years. |
| [`lifetime_pmf`](Mortality/lifetime_pmf.md) | The probability that death falls in each year, and that the life survives the horizon. |
| [`life_table`](Mortality/life_table.md) | The expected lives of a radix cohort at each age. |

## Practical Examples

### Example 1: one law, four views

The same five years of mortality as `qx`, `px`, force and survival.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  out.writeln("qx       =", mortality.qx().items<double>());
  out.writeln("px       =", mortality.px().items<double>());
  out.writeln("force    =", mortality.force().items<double>());
  out.writeln("survival =", mortality.survival().items<double>());
}
```

Output

```text
qx       = [0.010000, 0.012000, 0.015000, 0.019000, 0.024000]
px       = [0.990000, 0.988000, 0.985000, 0.981000, 0.976000]
force    = [0.010050, 0.012073, 0.015114, 0.019183, 0.024293]
survival = [1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
```

### Example 2: the same law from any representation

A law built from `px`, from forces and from a life table is the law it was derived from.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Mortality from_px = actuarial::mortality::from_px(mortality.px());
  const actuarial::Mortality from_force = actuarial::mortality::from_force(mortality.force());
  const actuarial::Mortality from_lx = actuarial::mortality::from_lx(mortality.life_table());
  out.writeln("qx                =", mortality.qx().items<double>());
  out.writeln("recovered from px =", from_px.qx().items<double>());
  out.writeln("recovered from mu =", from_force.qx().items<double>());
  out.writeln("recovered from lx =", from_lx.qx().items<double>());
}
```

Output

```text
qx                = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
recovered from px = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
recovered from mu = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
recovered from lx = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
```

### Example 3: the life table and the distribution of the lifetime

A cohort of 1,000 lives and the probability that death falls in each year; the last mass is survival to the end of the horizon, and the masses sum to one.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  out.writeln("lives of 1000 =", mortality.life_table(1000.0).items<double>());
  out.writeln("lifetime pmf  =", mortality.lifetime_pmf().items<double>());
  out.writeln("sum           =", mortality.lifetime_pmf().sum().item<double>());
}
```

Output

```text
lives of 1000 = [1000.000000, 990.000000, 978.120000, 963.448200, 945.142684, 922.459260]
lifetime pmf  = [0.010000, 0.011880, 0.014672, 0.018306, 0.022683, 0.922459]
sum           = 1.000000
```

### Example 4: an undefined law is refused

A default-constructed law reports itself undefined and the computed views throw.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
namespace losses = actuarial::losses;
namespace models = actuarial::models;
namespace distr = pluss::subsp::stats::distr;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const auto report = [&](const auto &call) {
    try {
      call();
    } catch (const std::exception &e) {
      std::string text(e.what());
      if (const auto cut = text.find("Triggered"); cut != std::string::npos) text.erase(cut);
      while (!text.empty() && text.back() == '\n') text.pop_back();
      out.writeln(text);
    }
  };
  const actuarial::Mortality none;
  out.writeln("defined:", none.defined(), " horizon:", none.horizon());
  report([&] { (void)none.px(); });
  report([&] { (void)none.survival(); });
}
```

Output

```text
defined: false  horizon: 0
Mortality is undefined
Mortality is undefined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Basis`](../valuation/Basis.md) pairs a law with a discount, [`life_contingencies`](../life_contingencies.md) has the same conversions as free functions, and [`MortalityModel`](MortalityModel.md) generates a law from ages.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="MortalityModel.html">
    <span class="label">Next</span>
    <span class="title">MortalityModel</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Mortality/defined
Mortality/force
Mortality/horizon
Mortality/life_table
Mortality/lifetime_pmf
Mortality/px
Mortality/qx
Mortality/survival
```
<!-- apiref:toctree:end -->
