<!-- generated from value.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::value`

Binds a basis into a reusable valuation operator.

## Signature

```cpp
actuarial::Valuation actuarial::operators::value(actuarial::Basis basis);
```

## What It Does

Creates a [`Valuation`](Valuation.md) that holds `basis` and values any cashflow on it: each payment is multiplied by its discount factor and by the probability of being alive (survival payments) or of dying in the year (death payments), and the two legs are summed. The mortality and discount are checked here, once; the cashflow is checked each time the valuation is applied.

Create a valuation per basis and reuse it for every benefit, premium schedule or policy on that basis; to change an assumption, build another. The result of applying it is a rank-zero `Series`.

## Parameters

`actuarial::Basis basis`
: A basis whose mortality and discount are both defined.

## Returns

A `Valuation`.

## Errors

Throws `operators::value: mortality must be defined` or `operators::value: discount must be defined` for an incomplete basis. Applying the result throws `valuation: cashflow must be defined` for an undefined cashflow, `valuation: cashflow horizon exceeds mortality horizon` for a cashflow longer than the mortality, and `Discount::factors: curve must contain at least horizon + 1 values` when a discount curve is too short.

## Practical Examples

### Example 1: building and applying

A valuation of a five-year annuity-due.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  out.writeln("value =", value(actuarial::cashflow::annuity(5)).item<double>());
}
```

Output

```text
value = 4.520664
```

### Example 2: reuse across benefits

One valuation values a whole list of benefits.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const std::vector<std::pair<std::string, actuarial::Cashflow>> benefits = {
      {"annuity-due", actuarial::cashflow::annuity(5)}, {"term", actuarial::cashflow::death(5)}, {"pure endowment", actuarial::cashflow::survival(5)}, {"endowment", actuarial::cashflow::endowment(5)}};
  for (const auto &[name, cashflow] : benefits) out.writeln(name, value(cashflow).item<double>());
}
```

Output

```text
annuity-due 4.520664
term 0.067934
pure endowment 0.758194
endowment 0.826128
```

### Example 3: a discount curve on the basis

The same benefit on a flat rate and on a curve.

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
  const actuarial::Discount curve = actuarial::discount::curve(pluss::make_series({1.0, 0.975, 0.945, 0.91, 0.87, 0.83}));
  out.writeln("flat 4% =", actuarial::operators::value({mortality, actuarial::discount::flat(0.04)})(actuarial::cashflow::annuity(5)).item<double>());
  out.writeln("curve   =", actuarial::operators::value({mortality, curve})(actuarial::cashflow::annuity(5)).item<double>());
}
```

Output

```text
flat 4% = 4.520664
curve   = 4.588585
```

### Example 4: the value is a rank-zero series

Read it with `item`.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const pluss::Series v = value(actuarial::cashflow::annuity(5));
  out.writeln("rank", v.dim(), " value", v.item<double>());
}
```

Output

```text
rank 0  value 4.520664
```

### Example 5: the errors

An incomplete basis is refused when the valuation is built; an incompatible cashflow when it is applied.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  report([&] { (void)actuarial::operators::value({actuarial::Mortality{}, actuarial::discount::flat(0.04)}); });
  report([&] { (void)actuarial::operators::value({mortality, actuarial::Discount{}}); });
  report([&] { (void)value(actuarial::cashflow::annuity(9)); });
  report([&] { (void)value(actuarial::Cashflow{}); });
  report([&] { (void)actuarial::operators::value({mortality, actuarial::discount::curve(pluss::make_series({1.0, 0.96}))})(actuarial::cashflow::annuity(5)); });
}
```

Output

```text
operators::value: mortality must be defined
operators::value: discount must be defined
valuation: cashflow horizon exceeds mortality horizon
valuation: cashflow must be defined
Discount::factors: curve must contain at least horizon + 1 values
```

## See Also

[`Basis`](Basis.md) is the argument, [`Valuation`](Valuation.md) the result and [`Cashflow`](Cashflow.md) what it values.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="endowment.html">
    <span class="label">Previous</span>
    <span class="title">endowment</span>
  </a>
</div>
<!-- apiref:nav:end -->
