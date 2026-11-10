<!-- generated from Discount.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Discount`

A discount assumption: either a level annual effective rate or an explicit curve of discount factors.

## Signature

```cpp
actuarial::Discount actuarial::discount::flat(double annual_effective_rate);
actuarial::Discount actuarial::discount::curve(const pluss::Series &factors);
bool actuarial::Discount::defined() const noexcept;
pluss::Series actuarial::Discount::factors(std::int64_t horizon, pluss::Platform platform = pluss::pCPU) const;
```

## Construction

A `Discount` is made by one of two factories and cannot be filled in afterwards. [`discount::flat`](flat.md) stores one annual effective rate `i`, whose factor for time `t` is {math}`v^t=(1+i)^{-t}`; [`discount::curve`](curve.md) stores the factors themselves, one per year from time 0, so that a yield curve enters a valuation unchanged. A default-constructed `Discount` is undefined and is refused by `operators::value`. The type is a small value: copying it copies the assumption.

A discount is one half of a [`Basis`](Basis.md): together with a [`Mortality`](../mortality/Mortality.md) it fixes the expected present value of any [`Cashflow`](Cashflow.md).

## Methods

| Method | What it does |
|---|---|
| [`defined`](Discount/defined.md) | Tells whether the discount was built by a factory. |
| [`factors`](Discount/factors.md) | The discount factors for times `0..horizon`. |

## Practical Examples

### Example 1: a flat rate and its factors

A flat rate of 4% generates the factors {math}`1.04^{-t}`.

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
  const actuarial::Discount discount = actuarial::discount::flat(0.04);
  out.writeln("factors for times 0..5 =", discount.factors(5).items<double>());
  out.writeln("by hand                =", 1.0, 1.0 / 1.04, 1.0 / (1.04 * 1.04));
}
```

Output

```text
factors for times 0..5 = [1.000000, 0.961538, 0.924556, 0.888996, 0.854804, 0.821927]
by hand                = 1.000000 0.961538 0.924556
```

### Example 2: an explicit curve

A curve states each factor, so a rate that varies with maturity is represented exactly. The curve below falls faster than a flat 4% in the later years.

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
  const actuarial::Discount curve = actuarial::discount::curve(pluss::make_series({1.0, 0.97, 0.93, 0.88}));
  out.writeln("curve factors for times 0..3 =", curve.factors(3).items<double>());
  out.writeln("flat 4% factors for 0..3     =", actuarial::discount::flat(0.04).factors(3).items<double>());
}
```

Output

```text
curve factors for times 0..3 = [1.000000, 0.970000, 0.930000, 0.880000]
flat 4% factors for 0..3     = [1.000000, 0.961538, 0.924556, 0.888996]
```

### Example 3: the same value from either form

A curve built from the flat factors values a benefit exactly as the flat rate does.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024}));
  std::vector<double> factors;
  for (int t = 0; t <= 5; ++t) factors.push_back(std::pow(1.04, -t));
  const auto annuity = [&](const actuarial::Discount &discount) {
    return actuarial::operators::value({mortality, discount})(actuarial::cashflow::annuity(5)).item<double>();
  };
  out.writeln("flat 4%      =", annuity(actuarial::discount::flat(0.04)));
  out.writeln("flat as curve=", annuity(actuarial::discount::curve(pluss::make_series(factors))));
}
```

Output

```text
flat 4%      = 4.520664
flat as curve= 4.520664
```

### Example 4: an undefined discount is refused

A default-constructed `Discount` carries no assumption.

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
  const actuarial::Discount none;
  out.writeln("defined:", none.defined());
  report([&] { (void)none.factors(3); });
}
```

Output

```text
defined: false
Discount is undefined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Basis`](Basis.md) pairs a discount with a mortality law, [`Cashflow`](Cashflow.md) is what gets discounted, and [`Valuation`](Valuation.md) is the operator that does it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Basis.html">
    <span class="label">Next</span>
    <span class="title">Basis</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Discount/defined
Discount/factors
```
<!-- apiref:toctree:end -->
