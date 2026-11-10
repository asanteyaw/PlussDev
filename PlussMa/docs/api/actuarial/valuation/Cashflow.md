<!-- generated from Cashflow.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Cashflow`

A contingent benefit: payments made if the life is alive at a time, and payments made at the end of the year of death.

## Signature

```cpp
actuarial::Cashflow actuarial::cashflow::make(const pluss::Series &survival_payments, const pluss::Series &death_payments);
actuarial::Cashflow actuarial::cashflow::annuity(std::int64_t years, double amount = 1.0, actuarial::PaymentTiming timing = actuarial::PaymentTiming::Start);
actuarial::Cashflow actuarial::cashflow::death(std::int64_t years, double benefit = 1.0);
actuarial::Cashflow actuarial::cashflow::survival(std::int64_t years, double benefit = 1.0);
actuarial::Cashflow actuarial::cashflow::endowment(std::int64_t years, double death_benefit = 1.0, double survival_benefit = 1.0);
bool actuarial::Cashflow::defined() const noexcept;
std::int64_t actuarial::Cashflow::horizon() const noexcept;
const pluss::Series &actuarial::Cashflow::survival_payments() const noexcept;
const pluss::Series &actuarial::Cashflow::death_payments() const noexcept;
```

## Construction

A `Cashflow` holds two aligned schedules over a horizon of `n` years. The **survival payments** have `n + 1` entries for times `0..n`: entry `t` is paid at time `t` if the life is alive then. The **death payments** have `n` entries for years `1..n`: entry `k` is paid at the end of year `k` if the life dies during that year. The expected present value of a cashflow on a [`Basis`](Basis.md) is

```{math}
\sum_{t=0}^{n} s_t\,v_t\,{}_tp_x\;+\;\sum_{k=1}^{n} d_k\,v_k\,{}_{k-1}p_x\,q_{x+k-1}.
```

The four named factories build the standard benefits and [`make`](make.md) builds any schedule. A default-constructed `Cashflow` is undefined and is refused by a valuation. Payments may be negative (a premium is a negative payment), so one cashflow can hold benefits and premiums together.

## Methods

| Method | What it does |
|---|---|
| [`defined`](Cashflow/defined.md) | Tells whether the cashflow was built by a factory. |
| [`horizon`](Cashflow/horizon.md) | The number of years `n` the cashflow spans. |
| [`survival_payments`](Cashflow/survival_payments.md) | The payments contingent on being alive, for times `0..n`. |
| [`death_payments`](Cashflow/death_payments.md) | The payments contingent on dying, for years `1..n`. |

## Practical Examples

### Example 1: the standard benefits

The four factories for a three-year term, shown as their two schedules.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  const auto show = [&](const char *name, const actuarial::Cashflow &c) {
    out.writeln(name, " survival", c.survival_payments().items<double>(), " death", c.death_payments().items<double>());
  };
  show("annuity-due     ", actuarial::cashflow::annuity(3));
  show("annuity-immediate", actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::End));
  show("term insurance  ", actuarial::cashflow::death(3, 10.0));
  show("pure endowment  ", actuarial::cashflow::survival(3, 7.0));
  show("endowment       ", actuarial::cashflow::endowment(3, 10.0, 4.0));
}
```

Output

```text
annuity-due       survival [1.0, 1.0, 1.0, 0.0]  death [0.0, 0.0, 0.0]
annuity-immediate  survival [0.0, 1.0, 1.0, 1.0]  death [0.0, 0.0, 0.0]
term insurance    survival [0.0, 0.0, 0.0, 0.0]  death [10.0, 10.0, 10.0]
pure endowment    survival [0.0, 0.0, 0.0, 7.0]  death [0.0, 0.0, 0.0]
endowment         survival [0.0, 0.0, 0.0, 4.0]  death [10.0, 10.0, 10.0]
```

### Example 2: valuing a cashflow

A cashflow is valued by binding a basis into a valuation. The program values the annuity-due, the term insurance and their sum, an endowment-like schedule, on one basis.

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
  out.writeln("annuity-due  =", value(actuarial::cashflow::annuity(5)).item<double>());
  out.writeln("term         =", value(actuarial::cashflow::death(5)).item<double>());
  out.writeln("pure         =", value(actuarial::cashflow::survival(5)).item<double>());
  out.writeln("endowment    =", value(actuarial::cashflow::endowment(5)).item<double>());
}
```

Output

```text
annuity-due  = 4.520664
term         = 0.067934
pure         = 0.758194
endowment    = 0.826128
```

### Example 3: any schedule

`make` takes the two schedules directly, so a benefit that varies by year, or benefits less premiums, is one cashflow. Here a policy pays 100 on death in year 1 and 50 in year 2, and costs a premium of 3 at the start of each year.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const actuarial::Cashflow policy = actuarial::cashflow::make(pluss::make_series({-3.0, -3.0, 0.0}), pluss::make_series({100.0, 50.0}));
  out.writeln("horizon", policy.horizon(), " value", value(policy).item<double>());
}
```

Output

```text
horizon 2  value -4.345044
```

### Example 4: an undefined cashflow is refused

A default-constructed cashflow reports itself undefined and its accessors throw.

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
  const actuarial::Cashflow none;
  out.writeln("defined:", none.defined(), " horizon:", none.horizon());
  report([&] { (void)value(none); });
}
```

Output

```text
defined: false  horizon: 0
valuation: cashflow must be defined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Valuation`](Valuation.md) values a cashflow, [`Basis`](Basis.md) supplies the assumptions, and [`PaymentTiming`](PaymentTiming.md) chooses when an annuity pays.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Basis.html">
    <span class="label">Previous</span>
    <span class="title">Basis</span>
  </a>
  <a class="next-link" href="PaymentTiming.html">
    <span class="label">Next</span>
    <span class="title">PaymentTiming</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Cashflow/death_payments
Cashflow/defined
Cashflow/horizon
Cashflow/survival_payments
```
<!-- apiref:toctree:end -->
