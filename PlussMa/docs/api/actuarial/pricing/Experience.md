<!-- generated from Experience.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Experience`

The observed portfolio a pricing run is computed from: losses, exposure and the premium in force, cell by cell.

## Signature

```cpp
actuarial::Experience::Experience(pluss::Series losses, pluss::Series exposure, pluss::Series current_premium);
actuarial::PricingResult actuarial::Pricing::operator()(const actuarial::Experience &experience) const;
```

An `Experience` holds three series of the **same shape**, one entry per rating cell (a class, a territory, a year). It is plain data: build it with the three series in order and pass it to a [`Pricing`](Pricing.md) operator. The pure premium of a cell is its losses per unit of exposure; the current premium is a **rate per unit of exposure** too, which is what makes the indicated rate change a ratio of like quantities. A one-dimensional series is the usual case and matrices of cells work the same way.

## Fields

### `losses`

`pluss::Series`. The incurred or otherwise selected loss of each cell, finite and 0 or more. The pure premium is proportional to it.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const actuarial::Experience experience{losses, exposure, premium};
  const auto base = actuarial::operators::price()(experience);
  const actuarial::Experience doubled{losses.mul(2.0), exposure, premium};
  out.writeln("pure premium         =", base.pure_premium.items<double>());
  out.writeln("with losses doubled  =", actuarial::operators::price()(doubled).pure_premium.items<double>());
}
```

Output

```text
pure premium         = [520.000000, 575.000000, 540.000000, 611.111111]
with losses doubled  = [1040.000000, 1150.000000, 1080.000000, 1222.222222]
```


### `exposure`

`pluss::Series`. The volume the losses arose over (car-years, payroll, policies), finite and 0 or more, one value per cell. The pure premium is losses per unit of it. A cell with zero exposure has pure premium zero rather than an error, so empty cells do not stop a run.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const actuarial::Experience experience{losses, exposure, premium};
  const actuarial::Experience half{losses, exposure.mul(0.5), premium};
  out.writeln("pure premium         =", actuarial::operators::price()(experience).pure_premium.items<double>());
  out.writeln("with half exposure   =", actuarial::operators::price()(half).pure_premium.items<double>());
  const actuarial::Experience empty{losses, pluss::make_series({1000.0, 0.0, 1500.0, 1800.0}), premium};
  out.writeln("one cell with no exposure =", actuarial::operators::price()(empty).pure_premium.items<double>());
}
```

Output

```text
pure premium         = [520.000000, 575.000000, 540.000000, 611.111111]
with half exposure   = [1040.000000, 1150.000000, 1080.000000, 1222.222222]
one cell with no exposure = [520.000000, 0.000000, 540.000000, 611.111111]
```


### `current_premium`

`pluss::Series`. The premium rate now charged in each cell, per unit of exposure, finite and strictly positive. It is the denominator of the indicated rate change and affects nothing else: the pure and indicated premiums do not depend on it.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const actuarial::Experience experience{losses, exposure, premium};
  const actuarial::Experience dearer{losses, exposure, premium.mul(2.0)};
  const auto a = actuarial::operators::price()(experience);
  const auto b = actuarial::operators::price()(dearer);
  out.writeln("indicated, now =", a.indicated_premium.items<double>());
  out.writeln("indicated, 2x  =", b.indicated_premium.items<double>());
  out.writeln("rate change, now =", a.rate_change.items<double>());
  out.writeln("rate change, 2x  =", b.rate_change.items<double>());
}
```

Output

```text
indicated, now = [520.000000, 575.000000, 540.000000, 611.111111]
indicated, 2x  = [520.000000, 575.000000, 540.000000, 611.111111]
rate change, now = [-0.257143, -0.201389, -0.270270, -0.195906]
rate change, 2x  = [-0.628571, -0.600694, -0.635135, -0.597953]
```


## Practical Examples

### Example 4: a rating grid

Cells need not be a vector: a 2 by 2 grid of territories by year prices in one call and returns the grid.

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
  const actuarial::Experience grid{
      pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0}).reshape({2, 2}),
      pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0}).reshape({2, 2}),
      pluss::make_series({700.0, 720.0, 740.0, 760.0}).reshape({2, 2})};
  const auto result = actuarial::operators::price()(grid);
  out.writeln("shape:", result.pure_premium.size(0), "x", result.pure_premium.size(1));
  out.writeln(result.pure_premium.reshape({4}).items<double>());
}
```

Output

```text
shape: 2 x 2
[520.000000, 575.000000, 540.000000, 611.111111]
```

### Example 5: the shapes must agree

Series of different lengths or with a bad value are refused when the operator is called.

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
  report([&] { (void)actuarial::operators::price()(actuarial::Experience{pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 2.0, 3.0}), pluss::make_series({1.0, 1.0})}); });
  report([&] { (void)actuarial::operators::price()(actuarial::Experience{pluss::make_series({-1.0, 2.0}), pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 1.0})}); });
  report([&] { (void)actuarial::operators::price()(actuarial::Experience{pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 2.0}), pluss::make_series({0.0, 1.0})}); });
  report([&] { (void)actuarial::operators::price()(actuarial::Experience{}); });
}
```

Output

```text
premium_curve: numerator and exposure must have same length
premium_curve: numerator must be >= 0
rate_change: current_premium must be strictly > 0
premium_curve: numerator must be defined
```

### Example 6: a single cell

A rank-zero experience prices one number.

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
  const actuarial::Experience one{pluss::make_series({100.0}).reshape({}), pluss::make_series({10.0}).reshape({}), pluss::make_series({8.0}).reshape({})};
  const auto r = actuarial::operators::price()(one);
  out.writeln(r.pure_premium.item<double>(), r.rate_change.item<double>());
}
```

Output

```text
10.000000 0.250000
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Pricing`](Pricing.md) consumes it, [`Loads`](Loads.md) are the other input, and [`PricingResult`](PricingResult.md) is what comes out.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Loads.html">
    <span class="label">Next</span>
    <span class="title">Loads</span>
  </a>
</div>
<!-- apiref:nav:end -->
