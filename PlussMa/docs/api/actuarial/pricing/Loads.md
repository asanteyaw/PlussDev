<!-- generated from Loads.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Loads`

The expense and profit assumptions bound into a pricing operator.

## Signature

```cpp
actuarial::Loads::Loads(double fixed_expense = 0.0, double variable_expense_ratio = 0.0, double profit_ratio = 0.0);
actuarial::Pricing actuarial::operators::price(actuarial::Loads loads = {});
```

A premium covers expected loss plus the cost of doing business. `Loads` splits that into a **fixed** amount per exposure unit and two **proportions of the final premium** (variable expense and profit). Because the ratios are shares of the premium, not of the pure premium, the indicated premium divides by what is left: {math}`P=\dfrac{\text{pure}+F}{1-v-\pi}`. All three members default to zero, which prices the pure premium alone. Build it positionally in the order below and give it to [`price`](price.md).

## Fields

### `fixed_expense`

`double`, default `0`. The expense per unit of exposure that does not vary with premium, finite and 0 or more. It is added to the pure premium before the ratios are applied.

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
  for (const double fixed : {0.0, 25.0, 50.0}) {
    const auto r = actuarial::operators::price(actuarial::Loads{fixed, 0.0, 0.0})(experience);
    out.writeln("fixed =", fixed, " indicated =", r.indicated_premium.items<double>());
  }
}
```

Output

```text
fixed = 0.000000  indicated = [520.000000, 575.000000, 540.000000, 611.111111]
fixed = 25.000000  indicated = [545.000000, 600.000000, 565.000000, 636.111111]
fixed = 50.000000  indicated = [570.000000, 625.000000, 590.000000, 661.111111]
```


### `variable_expense_ratio`

`double`, default `0`. The share of the premium spent on expenses that scale with it (commission, taxes), finite and 0 or more, with `variable_expense_ratio + profit_ratio < 1`. A ratio of 0.15 means premium is 1/0.85 of the cost it covers.

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
  for (const double v : {0.0, 0.15, 0.30}) {
    const auto r = actuarial::operators::price(actuarial::Loads{0.0, v, 0.0})(experience);
    out.writeln("variable =", v, " indicated =", r.indicated_premium.items<double>());
  }
  out.writeln("pure / (1 - 0.15) for cell 0 =", 520.0 / 0.85);
}
```

Output

```text
variable = 0.000000  indicated = [520.000000, 575.000000, 540.000000, 611.111111]
variable = 0.150000  indicated = [611.764706, 676.470588, 635.294118, 718.954248]
variable = 0.300000  indicated = [742.857143, 821.428571, 771.428571, 873.015873]
pure / (1 - 0.15) for cell 0 = 611.764706
```


### `profit_ratio`

`double`, default `0`. The share of the premium targeted as profit or contingency, finite and 0 or more. It enters exactly like the variable expense ratio; only their sum matters to the premium.

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
  const auto a = actuarial::operators::price(actuarial::Loads{0.0, 0.10, 0.05})(experience);
  const auto b = actuarial::operators::price(actuarial::Loads{0.0, 0.05, 0.10})(experience);
  out.writeln("10% + 5% =", a.indicated_premium.items<double>());
  out.writeln("5% + 10% =", b.indicated_premium.items<double>());
}
```

Output

```text
10% + 5% = [611.764706, 676.470588, 635.294118, 718.954248]
5% + 10% = [611.764706, 676.470588, 635.294118, 718.954248]
```


## Practical Examples

### Example 4: the three together

Fixed expense of 50 per exposure unit, 15% variable expense and 5% profit.

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
  out.writeln("pure      =", r.pure_premium.items<double>());
  out.writeln("indicated =", r.indicated_premium.items<double>());
  out.writeln("by hand, cell 0 =", (520.0 + 50.0) / (1.0 - 0.15 - 0.05));
}
```

Output

```text
pure      = [520.000000, 575.000000, 540.000000, 611.111111]
indicated = [712.500000, 781.250000, 737.500000, 826.388889]
by hand, cell 0 = 712.500000
```

### Example 5: the defaults

No arguments means no loads: the indicated premium is the pure premium.

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
  const actuarial::Loads none;
  out.writeln(none.fixed_expense, none.variable_expense_ratio, none.profit_ratio);
  const auto r = actuarial::operators::price(none)(experience);
  out.writeln(r.indicated_premium.items<double>());
}
```

Output

```text
0.000000 0.000000 0.000000
[520.000000, 575.000000, 540.000000, 611.111111]
```

### Example 6: the limits

Negative or non-finite values and ratios that sum to one or more are refused when the operator is built.

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
  report([&] { (void)actuarial::operators::price(actuarial::Loads{-1.0, 0.1, 0.1}); });
  report([&] { (void)actuarial::operators::price(actuarial::Loads{0.0, -0.1, 0.1}); });
  report([&] { (void)actuarial::operators::price(actuarial::Loads{0.0, 0.1, std::nan("")}); });
  report([&] { (void)actuarial::operators::price(actuarial::Loads{0.0, 0.6, 0.4}); });
}
```

Output

```text
operators::price: fixed_expense must be finite and >= 0
operators::price: variable_expense_ratio must be finite and >= 0
operators::price: profit_ratio must be finite and >= 0
operators::price: variable_expense_ratio + profit_ratio must be < 1
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`price`](price.md) binds the loads, and [`Pricing`](Pricing.md) is the operator it returns.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Experience.html">
    <span class="label">Previous</span>
    <span class="title">Experience</span>
  </a>
  <a class="next-link" href="PricingResult.html">
    <span class="label">Next</span>
    <span class="title">PricingResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
