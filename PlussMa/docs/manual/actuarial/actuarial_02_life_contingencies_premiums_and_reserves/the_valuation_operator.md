<!-- generated from the_valuation_operator.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The Valuation Operator

[Back to Life Contingencies, Premiums and Reserves](../actuarial_02_life_contingencies_premiums_and_reserves.md)

The functions of the previous page fix the benefit and the way it is paid. Real policies are not so tidy: a benefit may rise each year, be returned with the premiums, or be paid on a schedule of the policyholder's own. The valuation surface separates what is assumed from what is paid, so that one calculation values every benefit.

An **assumption** is a `Basis`: a mortality law and a discount. A **benefit** is a `Cashflow`: payments that are made if the life is alive at a time, and payments that are made at the end of the year of death. A `Valuation` is created once from a basis and then applied to any cashflow; the result is the expected present value as a rank-zero `Series`:

```{math}
\text{EPV}=\sum_{t=0}^{n} s_t\,v_t\,{}_tp_x\;+\;\sum_{k=1}^{n} d_k\,v_k\,{}_{k-1}p_x\,q_{x+k-1},
```

where `s_t` are the survival payments at times `0..n`, `d_k` the death payments at the ends of years `1..n`, and `v_t` the discount factors.

## The pieces

```cpp
actuarial::Mortality actuarial::mortality::from_qx(const pluss::Series &qx);
actuarial::Mortality actuarial::mortality::from_px(const pluss::Series &px);
actuarial::Mortality actuarial::mortality::from_force(const pluss::Series &force);
actuarial::Mortality actuarial::mortality::from_lx(const pluss::Series &lx);
actuarial::Mortality actuarial::mortality::select_ultimate(const pluss::Series &select_qx, const pluss::Series &ultimate_qx, std::int64_t projection_years);
actuarial::Discount actuarial::discount::flat(double annual_effective_rate);
actuarial::Discount actuarial::discount::curve(const pluss::Series &factors);
actuarial::Basis actuarial::Basis{actuarial::Mortality mortality, actuarial::Discount discount};
actuarial::Cashflow actuarial::cashflow::annuity(std::int64_t years, double amount = 1.0, actuarial::PaymentTiming timing = actuarial::PaymentTiming::Start);
actuarial::Cashflow actuarial::cashflow::death(std::int64_t years, double benefit = 1.0);
actuarial::Cashflow actuarial::cashflow::survival(std::int64_t years, double benefit = 1.0);
actuarial::Cashflow actuarial::cashflow::endowment(std::int64_t years, double death_benefit = 1.0, double survival_benefit = 1.0);
actuarial::Cashflow actuarial::cashflow::make(const pluss::Series &survival_payments, const pluss::Series &death_payments);
actuarial::Valuation actuarial::operators::value(actuarial::Basis basis);
pluss::Series actuarial::Valuation::operator()(const actuarial::Cashflow &cashflow) const;
```

| Piece | Meaning |
|---|---|
| `Mortality` | the one-year probabilities `qx` and everything derived from them: `px()`, `force()`, `survival()`, `lifetime_pmf()`, `life_table(radix)`, `horizon()` |
| `mortality::from_qx`, `from_px`, `from_force`, `from_lx` | the same law from any one of its four representations |
| `mortality::select_ultimate` | a select table for the first durations followed by the ultimate rates |
| `discount::flat(i)` | a level annual effective rate; the rate must be above −1 |
| `discount::curve(factors)` | explicit factors `v_0 = 1, v_1, ...`; a horizon of `n` years needs `n + 1` values |
| `Basis` | a `Mortality` and a `Discount` together |
| `cashflow::annuity(years, amount, timing)` | `amount` at `0..years−1` (`PaymentTiming::Start`, annuity-due) or `1..years` (`End`, annuity-immediate), while alive |
| `cashflow::death(years, benefit)` | `benefit` at the end of the year of death within `years` |
| `cashflow::survival(years, benefit)` | `benefit` at time `years` if alive |
| `cashflow::endowment(years, death_benefit, survival_benefit)` | the death benefit during the term plus the survival benefit at the end |
| `cashflow::make(survival_payments, death_payments)` | any schedule: `n + 1` survival payments for times `0..n` and `n` death payments for years `1..n` |
| `operators::value(basis)` | a `Valuation` for the basis; apply it to a cashflow to get its present value |

The horizon of a cashflow may not exceed the horizon of the mortality; a cashflow shorter than the mortality is valued over its own years.

## One basis, every benefit

The mortality and the interest are those of the previous page. Binding them into one `Valuation` values the annuity, the insurance, the pure endowment and the endowment insurance, and each value is the number the function of the previous page returns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::Basis basis{actuarial::mortality::from_qx(qx), actuarial::discount::flat(i)};
  const actuarial::Valuation value = actuarial::operators::value(basis);

  out.writeln("annuity-due   =", value(actuarial::cashflow::annuity(10)).item<double>(), " function", actuarial::annuity_due(qx, i).item<double>());
  out.writeln("term          =", value(actuarial::cashflow::death(10)).item<double>(), " function", actuarial::term_insurance(qx, i).item<double>());
  out.writeln("pure          =", value(actuarial::cashflow::survival(10)).item<double>(), " function", actuarial::pure_endowment(qx, i).item<double>());
  out.writeln("endowment     =", value(actuarial::cashflow::endowment(10)).item<double>(), " function", actuarial::endowment_insurance(qx, i).item<double>());
}
```

Output

```text
annuity-due   = 8.005303  function 8.005303
term          = 0.109876  function 0.109876
pure          = 0.582228  function 0.582228
endowment     = 0.692104  function 0.692104
```

## Timing and the horizon

`PaymentTiming::Start` is an annuity-due and `End` an annuity-immediate. They differ by the payment at time 0 and the missing payment at time `n`: {math}`\ddot a_{x:\overline n\vert} - a_{x:\overline n\vert} = 1 - v^n\,{}_np_x`. A cashflow may also be shorter than the mortality it is valued on, and is then valued over its own years: the five-year annuity below uses only the first five probabilities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::Valuation value = actuarial::operators::value({actuarial::mortality::from_qx(qx), actuarial::discount::flat(i)});
  const double due = value(actuarial::cashflow::annuity(10, 1.0, actuarial::PaymentTiming::Start)).item<double>();
  const double immediate = value(actuarial::cashflow::annuity(10, 1.0, actuarial::PaymentTiming::End)).item<double>();
  const double v = 1.0 / (1.0 + i);
  const double survive = actuarial::survival_curve(qx).items<double>().back();
  out.writeln("annuity-due       =", due, " annuity-immediate =", immediate);
  out.writeln("difference        =", due - immediate, " 1 - v^n nPx       =", 1.0 - std::pow(v, 10.0) * survive);
  out.writeln("five-year annuity =", value(actuarial::cashflow::annuity(5)).item<double>(), " amount 100 =", value(actuarial::cashflow::annuity(5, 100.0)).item<double>());
}
```

Output

```text
annuity-due       = 8.005303  annuity-immediate = 7.587530
difference        = 0.417772  1 - v^n nPx       = 0.417772
five-year annuity = 4.532343  amount 100 = 453.234345
```

## The same mortality from four representations

A `Mortality` can be built from `qx`, `px`, the force or the life-table lives `lx`, because the four describe one law. The values agree to every digit, and the accessors move between them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::Mortality from_qx = actuarial::mortality::from_qx(qx);
  const actuarial::Mortality from_px = actuarial::mortality::from_px(life::px_qx(qx));
  const actuarial::Mortality from_force = actuarial::mortality::from_force(life::force_qx(qx));
  const actuarial::Mortality from_lx = actuarial::mortality::from_lx(life::lx_qx(qx, 100000.0));
  const auto annuity = [&](const actuarial::Mortality &m) {
    return actuarial::operators::value({m, actuarial::discount::flat(i)})(actuarial::cashflow::annuity(10)).item<double>();
  };
  out.writeln("annuity from qx =", annuity(from_qx), " px =", annuity(from_px), " force =", annuity(from_force), " lx =", annuity(from_lx));
  out.writeln("horizon", from_qx.horizon(), " survival has", from_qx.survival().numel(), "points; lifetime pmf has", from_qx.lifetime_pmf().numel(), "and sums to", from_qx.lifetime_pmf().sum().item<double>());
  out.writeln("life table of 1000 lives", from_qx.life_table(1000.0).items<double>());
}
```

Output

```text
annuity from qx = 8.005303  px = 8.005303  force = 8.005303  lx = 8.005303
horizon 10  survival has 11 points; lifetime pmf has 11 and sums to 1.000000
life table of 1000 lives [1000.000000, 989.727421, 978.767189, 967.081899, 954.633705, 941.384628, 927.296919, 912.333490, 896.458419, 879.637539, 861.839106]
```

## A select period

A select table gives higher or lower mortality in the first durations after a life is underwritten, and ultimate rates thereafter. `select_ultimate` joins them over `projection_years`: the select rates come first, and the ultimate vector takes over from its first entry at the first duration after the select period.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const actuarial::Mortality m = actuarial::mortality::select_ultimate(pluss::make_series({0.005, 0.007}), pluss::make_series({0.010, 0.012, 0.014, 0.016}), 5);
  out.writeln("qx =", m.qx().items<double>(), " horizon", m.horizon());
}
```

Output

```text
qx = [0.005000, 0.007000, 0.010000, 0.012000, 0.014000]  horizon 5
```

## The discount: flat or a curve

A flat rate generates the factors `v^t`. An explicit curve gives each factor, which is how a yield curve enters. A curve equal to the flat factors reproduces the flat answer. The sloping curve below discounts the early years at 2.4% and the late years at up to 6%, and values the annuity at 8.1940 against 8.0053 for the flat 4%, because the large early payments are discounted less.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  std::vector<double> flat(11), sloping(11);
  double accumulated = 1.0;
  flat[0] = sloping[0] = 1.0;
  for (int t = 1; t <= 10; ++t) {
    flat[t] = std::pow(1.0 / (1.0 + i), t);
    accumulated /= 1.0 + 0.02 + 0.004 * t;
    sloping[t] = accumulated;
  }
  const auto annuity = [&](const actuarial::Discount &discount) {
    return actuarial::operators::value({mortality, discount})(actuarial::cashflow::annuity(10)).item<double>();
  };
  out.writeln("flat rate     =", annuity(actuarial::discount::flat(i)));
  out.writeln("flat as curve =", annuity(actuarial::discount::curve(pluss::make_series(flat))));
  out.writeln("sloping curve =", annuity(actuarial::discount::curve(pluss::make_series(sloping))));
  out.writeln("factors of the flat rate:", actuarial::discount::flat(i).factors(5).items<double>());
}
```

Output

```text
flat rate     = 8.005303
flat as curve = 8.005303
sloping curve = 8.193960
factors of the flat rate: [1.000000, 0.961538, 0.924556, 0.888996, 0.854804, 0.821927]
```

## Any schedule: `cashflow::make`

`make(survival_payments, death_payments)` takes the survival payments at times `0..n` (one more than the years) and the death payments at the ends of years `1..n`. A death benefit that increases by one unit for each year is an increasing term insurance, which has no function of its own; here it is a ramp in `death_payments`. The program checks it against the sum written out from the survival curve.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
  const actuarial::Valuation value = actuarial::operators::value({actuarial::mortality::from_qx(qx), actuarial::discount::flat(i)});
  std::vector<double> ramp(10);
  for (int k = 0; k < 10; ++k) ramp[k] = k + 1.0;
  const actuarial::Cashflow increasing = actuarial::cashflow::make(pluss::series::zeros({11}), pluss::make_series(ramp));
  const auto p = actuarial::survival_curve(qx).items<double>();
  const auto q = qx.items<double>();
  double by_hand = 0.0;
  for (int k = 0; k < 10; ++k) by_hand += (k + 1.0) * std::pow(1.0 / (1.0 + i), k + 1) * p[k] * q[k];
  out.writeln("increasing term insurance =", value(increasing).item<double>(), " by the sum =", by_hand);
  out.writeln("level term insurance      =", value(actuarial::cashflow::death(10)).item<double>());
}
```

Output

```text
increasing term insurance = 0.624121  by the sum = 0.624121
level term insurance      = 0.109876
```

## Invalid inputs fail explicitly

The valuation checks that the pieces are defined and compatible before it computes: a rate at or below −1, a curve with too few factors, a cashflow longer than the mortality, survival and death payments of the wrong lengths, and non-finite payments are all refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace actuarial = pluss::ma::actuarial;
namespace life = actuarial::life_contingencies;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0});
  const pluss::Series qx = life::makeham_qx(ages, 0.0002, 0.0001, 1.08);
  const double i = 0.04;
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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const actuarial::Valuation value = actuarial::operators::value({mortality, actuarial::discount::flat(i)});
  report([&] { (void)actuarial::discount::flat(-1.0); });
  report([&] { (void)actuarial::operators::value({mortality, actuarial::discount::curve(pluss::make_series({1.0, 0.96}))})(actuarial::cashflow::annuity(5)); });
  report([&] { (void)value(actuarial::cashflow::annuity(12)); });
  report([&] { (void)actuarial::cashflow::make(pluss::make_series({1.0, 1.0}), pluss::make_series({1.0, 1.0})); });
  report([&] { (void)actuarial::cashflow::make(pluss::make_series({1.0, std::nan("")}), pluss::make_series({1.0})); });
  report([&] { (void)value(actuarial::Cashflow{}); });
}
```

Output

```text
discount::flat: rate must be > -1
Discount::factors: curve must contain at least horizon + 1 values
valuation: cashflow horizon exceeds mortality horizon
cashflow::make: survival_payments must have one more value than death_payments
cashflow::make: survival_payments must be finite
valuation: cashflow must be defined
```

## Where to go next

Level premiums and reserves are valuations of benefits and premium schedules built here ([Net Premiums](net_premiums.md), [Reserves](reserves.md)); the classical commutation columns are another route to the same present values ([Commutation Columns](commutation_columns.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="annuities_insurance_and_endowments.html">
    <span class="label">Previous</span>
    <span class="title">Annuities, insurance and endowments</span>
  </a>
  <a class="next-link" href="commutation_columns.html">
    <span class="label">Next</span>
    <span class="title">Commutation columns</span>
  </a>
</div>
<!-- apiref:nav:end -->
