<!-- generated from PricingResult.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::PricingResult`

The three linked series of one pricing run: pure premium, indicated premium and rate change.

## Signature

```cpp
actuarial::PricingResult actuarial::Pricing::operator()(const actuarial::Experience &experience) const;
pluss::Series actuarial::PricingResult::pure_premium;
pluss::Series actuarial::PricingResult::indicated_premium;
pluss::Series actuarial::PricingResult::rate_change;
```

Calling a [`Pricing`](Pricing.md) operator returns a `PricingResult`: one series per stage of the indication, all of the shape of the experience. Keeping the stages together means each can be audited against the one before: the pure premium is the data, the indicated premium adds the loads, and the rate change compares it with what is charged now. Read the fields directly.

## Fields

### `pure_premium`

`pluss::Series`. Losses per unit of exposure, {math}`\text{losses}/\text{exposure}`, with zero where the exposure is zero. It is the expected-loss part of the rate.

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
  const auto r = actuarial::operators::price()(experience);
  out.writeln("pure premium =", r.pure_premium.items<double>());
  out.writeln("by hand      =", 520000.0 / 1000.0, 690000.0 / 1200.0, 810000.0 / 1500.0, 1100000.0 / 1800.0);
}
```

Output

```text
pure premium = [520.000000, 575.000000, 540.000000, 611.111111]
by hand      = 520.000000 575.000000 540.000000 611.111111
```


### `indicated_premium`

`pluss::Series`. The rate the loads call for, {math}`(\text{pure}+F)/(1-v-\pi)`. With default loads it equals the pure premium.

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
  const actuarial::Loads loads{50.0, 0.15, 0.05};
  const auto r = actuarial::operators::price(loads)(experience);
  out.writeln("indicated =", r.indicated_premium.items<double>());
  for (double pure : r.pure_premium.items<double>()) out.write((pure + 50.0) / 0.80, " ");
  out.writeln();
}
```

Output

```text
indicated = [712.500000, 781.250000, 737.500000, 826.388889]
712.500000  781.250000  737.500000  826.388889
```


### `rate_change`

`pluss::Series`. The change needed to move the current premium to the indicated one, {math}`\text{indicated}/\text{current}-1`: positive where the rate is too low, negative where it is too high.

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
  const actuarial::Loads loads{50.0, 0.15, 0.05};
  const auto r = actuarial::operators::price(loads)(experience);
  out.writeln("rate change =", r.rate_change.items<double>());
  out.writeln("cell 0 by hand =", 712.5 / 700.0 - 1.0);
}
```

Output

```text
rate change = [0.017857, 0.085069, -0.003378, 0.087354]
cell 0 by hand = 0.017857
```


## Practical Examples

### Example 4: auditing a run

Print the three stages side by side for one cell.

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
  const actuarial::Loads loads{50.0, 0.15, 0.05};
  const auto r = actuarial::operators::price(loads)(experience);
  const auto pure = r.pure_premium.items<double>(), ind = r.indicated_premium.items<double>(), chg = r.rate_change.items<double>();
  for (std::size_t i = 0; i < pure.size(); ++i) out.writeln("cell", i, " pure", pure[i], " indicated", ind[i], " change", chg[i]);
}
```

Output

```text
cell 0  pure 520.000000  indicated 712.500000  change 0.017857
cell 1  pure 575.000000  indicated 781.250000  change 0.085069
cell 2  pure 540.000000  indicated 737.500000  change -0.003378
cell 3  pure 611.111111  indicated 826.388889  change 0.087354
```

### Example 5: the portfolio rate change

Weighting the indication by exposure gives an overall figure.

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
  const actuarial::Loads loads{50.0, 0.15, 0.05};
  const auto r = actuarial::operators::price(loads)(experience);
  const double needed = r.indicated_premium.mul(exposure).sum().item<double>();
  const double charged = premium.mul(exposure).sum().item<double>();
  out.writeln("overall rate change =", needed / charged - 1.0);
}
```

Output

```text
overall rate change = 0.049913
```

### Example 6: it is plain data

The result is an ordinary value: copy it, keep the fields.

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
  actuarial::PricingResult kept = actuarial::operators::price()(experience);
  out.writeln(kept.pure_premium.numel(), kept.indicated_premium.numel(), kept.rate_change.numel());
}
```

Output

```text
4 4 4
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Pricing`](Pricing.md) produces it from an [`Experience`](Experience.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Loads.html">
    <span class="label">Previous</span>
    <span class="title">Loads</span>
  </a>
  <a class="next-link" href="CredibilityInput.html">
    <span class="label">Next</span>
    <span class="title">CredibilityInput</span>
  </a>
</div>
<!-- apiref:nav:end -->
