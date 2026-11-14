<!-- generated from net_premiums.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Net Premiums

[Back to Life Contingencies, Premiums and Reserves](../actuarial_02_life_contingencies_premiums_and_reserves.md)

A policy exchanges premiums for benefits. The **equivalence principle** sets the premium so that the exchange is fair on average: the expected present value of the premiums equals the expected present value of the benefits. A **net** premium counts only benefits and mortality and interest, with no expenses or margins. For a benefit with present value `B` paid for by a level premium `P` due at the start of each year the policy is in force for `m` years,

```{math}
P\;\ddot a_{x:\overline m|} \;=\; B
\qquad\Longrightarrow\qquad
P=\frac{B}{\ddot a_{x:\overline m|}} .
```

The numerator and the denominator are valuations, so a premium is a quotient of two present values on the same basis. The premium for a unit endowment insurance has a dedicated function; every other premium is a quotient built from the valuation operator.

```cpp
pluss::Series actuarial::endowment_premium(const pluss::Series &qx, double interest_rate);
pluss::Series actuarial::Valuation::operator()(const actuarial::Cashflow &cashflow) const;
```

| Argument | Meaning |
|---|---|
| `qx` | the annual mortality over the term, one value per year, in [0, 1] |
| `interest_rate` | the annual effective rate, above −1 |
| `cashflow` | the benefit and the annuity of premiums, as on [The Valuation Operator](the_valuation_operator.md) |

`endowment_premium` returns the level annual premium, payable at the start of each of the `n` years while the policy is in force, for a unit endowment insurance. The function returns a rank-zero `Series`.

## The endowment premium

The premium for the ten-year unit endowment insurance of the earlier pages is the benefit 0.692104 divided by the annuity-due 8.005303. The function does that in one call, and the quotient of the two valuations of the operator gives the same number.

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
  const double benefit = value(actuarial::cashflow::endowment(10)).item<double>();
  const double annuity = value(actuarial::cashflow::annuity(10)).item<double>();
  out.writeln("benefit =", benefit, " annuity-due =", annuity);
  out.writeln("endowment_premium =", actuarial::endowment_premium(qx, i).item<double>(), " benefit / annuity =", benefit / annuity);
}
```

Output

```text
benefit = 0.692104  annuity-due = 8.005303
endowment_premium = 0.086456  benefit / annuity = 0.086456
```

## Other benefits and other payment terms

Term insurance, pure endowment and endowment insurance differ in the benefit; limited-pay policies differ in the denominator, since the premiums stop after `m < n` years but the cover continues. The premium for an `m`-year payment term is the same benefit over a shorter annuity, so it is higher per year; the table below prices all four on the same basis.

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
  const double annuity10 = value(actuarial::cashflow::annuity(10)).item<double>();
  const double annuity5 = value(actuarial::cashflow::annuity(5)).item<double>();
  out.writeln("term insurance, 10-year premiums   =", value(actuarial::cashflow::death(10)).item<double>() / annuity10);
  out.writeln("pure endowment, 10-year premiums   =", value(actuarial::cashflow::survival(10)).item<double>() / annuity10);
  out.writeln("endowment insurance, 10 years      =", value(actuarial::cashflow::endowment(10)).item<double>() / annuity10);
  out.writeln("endowment insurance, 5-year premiums=", value(actuarial::cashflow::endowment(10)).item<double>() / annuity5);
}
```

Output

```text
term insurance, 10-year premiums   = 0.013725
pure endowment, 10-year premiums   = 0.072730
endowment insurance, 10 years      = 0.086456
endowment insurance, 5-year premiums= 0.152703
```

## The equivalence principle as one cashflow

The principle says that benefits less premiums have zero present value. That is itself a cashflow: the survival payments are `−P` at times `0..n−1`, where the premium is paid, and the survival benefit `+1` at time `n`; the death payments are the death benefit `+1` at each year. Valuing this one schedule at the equivalence premium gives zero. At any other premium it gives the expected present value of the policy to the insurer with its sign reversed: positive when the premium is too low (an expected loss) and negative when it is too high (an expected profit), and since the value is linear in the premium, a premium 5% off moves it by 5% of the benefit, 0.0346.

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
  const double premium = actuarial::endowment_premium(qx, i).item<double>();
  const auto net = [&](double p) {
    std::vector<double> survival(11, -p);
    survival[10] = 1.0;                      // the survival benefit at time n replaces the premium that would fall due then
    return value(actuarial::cashflow::make(pluss::make_series(survival), pluss::make_series(std::vector<double>(10, 1.0)))).item<double>();
  };
  out.writeln("at the equivalence premium", premium, " value of benefits less premiums =", net(premium));
  out.writeln("at a premium 5% too low   ", 0.95 * premium, " value =", net(0.95 * premium));
  out.writeln("at a premium 5% too high  ", 1.05 * premium, " value =", net(1.05 * premium));
}
```

Output

```text
at the equivalence premium 0.086456  value of benefits less premiums = 0.000000
at a premium 5% too low    0.082133  value = 0.034605
at a premium 5% too high   0.090778  value = -0.034605
```

## What the rate, the term and the age do to the premium

The premium rises with the age of the life, since the benefit is nearer, and falls as the interest rate rises, since the same benefit is cheaper to fund. The program prices the unit endowment premium at three rates and at three starting ages.

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
  for (const double rate : {0.02, 0.04, 0.06}) out.writeln("interest", rate, " endowment premium", actuarial::endowment_premium(qx, rate).item<double>());
  for (const double start : {50.0, 60.0, 70.0}) {
    std::vector<double> grid;
    for (int k = 0; k < 10; ++k) grid.push_back(start + k);
    const pluss::Series q = life::makeham_qx(pluss::make_series(grid), 0.0002, 0.0001, 1.08);
    out.writeln("age", start, " endowment premium", actuarial::endowment_premium(q, i).item<double>());
  }
}
```

Output

```text
interest 0.020000  endowment premium 0.095656
interest 0.040000  endowment premium 0.086456
interest 0.060000  endowment premium 0.078168
age 50.000000  endowment premium 0.083074
age 60.000000  endowment premium 0.086456
age 70.000000  endowment premium 0.093879
```

## Invalid inputs fail explicitly

The same checks apply as for the benefits: `qx` one-dimensional with values in `[0, 1]`, and a rate above −1; the premium is computed from the commutation columns, so the messages come from `commutation_table`. A payment term longer than the mortality cannot be valued.

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
  report([&] { (void)actuarial::endowment_premium(pluss::make_series({0.1, 1.5}), i); });
  report([&] { (void)actuarial::endowment_premium(qx, -1.0); });
  const actuarial::Valuation value = actuarial::operators::value({actuarial::mortality::from_qx(qx), actuarial::discount::flat(i)});
  report([&] { (void)value(actuarial::cashflow::annuity(12)); });
}
```

Output

```text
commutation_table: qx values must lie in [0, 1]
commutation_table: interest_rate must be > -1
valuation: cashflow horizon exceeds mortality horizon
```

## Where to go next

The premium of a policy fixes the value of its future liability at each duration: the difference between the benefits still to come and the premiums still to be paid is the reserve ([Reserves](reserves.md)).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="commutation_columns.html">
    <span class="label">Previous</span>
    <span class="title">Commutation columns</span>
  </a>
  <a class="next-link" href="reserves.html">
    <span class="label">Next</span>
    <span class="title">Reserves</span>
  </a>
</div>
<!-- apiref:nav:end -->
