<!-- generated from annuity.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::cashflow::annuity`

Level payments while the life survives, in advance or in arrear.

## Signature

```cpp
actuarial::Cashflow actuarial::cashflow::annuity(std::int64_t years, double amount = 1.0, actuarial::PaymentTiming timing = actuarial::PaymentTiming::Start);
```

## What It Does

Creates `amount` paid each year while the life is alive: at times `0..years−1` for `PaymentTiming::Start`, an annuity-due, and at times `1..years` for `End`, an annuity-immediate. The expected present value of a unit annuity-due over `n` years is {math}`\ddot a_{x:\overline n\vert}=\sum_{k=0}^{n-1}v^k\,{}_kp_x`. It is a convenience constructor: a valuation treats it like any other cashflow.

A zero-year annuity is valid and empty. The amount may be negative, which is how a stream of premiums is written. The cashflow has no death payments.

## Parameters

`std::int64_t years`
: The number of payments; at least 0.

`double amount`
: The payment each year, finite; default 1.

`actuarial::PaymentTiming timing`
: `Start` (default) or `End`.

## Returns

A defined `Cashflow` with `years` years and no death payments.

## Errors

Throws `cashflow::annuity: years must be >= 0` for a negative number of years and `cashflow::annuity: amount must be finite` for a NaN or infinite amount.

## Practical Examples

### Example 1: the two timings

Three payments of 1: at times 0, 1, 2 (due) or at times 1, 2, 3 (immediate).

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
  out.writeln("due       ", actuarial::cashflow::annuity(3).survival_payments().items<double>());
  out.writeln("immediate ", actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::End).survival_payments().items<double>());
}
```

Output

```text
due        [1.0, 1.0, 1.0, 0.0]
immediate  [0.0, 1.0, 1.0, 1.0]
```

### Example 2: valuing the two timings

The due annuity is worth more, by the payment at time 0 less the missing last payment: {math}`\ddot a - a = 1 - v^n\,{}_np_x`.

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
  const double due = value(actuarial::cashflow::annuity(5)).item<double>();
  const double immediate = value(actuarial::cashflow::annuity(5, 1.0, actuarial::PaymentTiming::End)).item<double>();
  const double survive = actuarial::survival_curve(qx).items<double>().back();
  out.writeln("due", due, " immediate", immediate, " difference", due - immediate, " 1 - v^n nPx", 1.0 - std::pow(1.0 / 1.04, 5) * survive);
}
```

Output

```text
due 4.520664  immediate 4.278858  difference 0.241806  1 - v^n nPx 0.241806
```

### Example 3: the amount

The value is proportional to the payment: an amount of 1,200 is 1,200 times the unit annuity.

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
  out.writeln("unit =", value(actuarial::cashflow::annuity(5)).item<double>(), " 1200 =", value(actuarial::cashflow::annuity(5, 1200.0)).item<double>());
}
```

Output

```text
unit = 4.520664  1200 = 5424.796643
```

### Example 4: a shorter term than the mortality

An annuity of three years on a basis of five is valued over its own three years.

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
  out.writeln("3 years =", value(actuarial::cashflow::annuity(3)).item<double>(), " 5 years =", value(actuarial::cashflow::annuity(5)).item<double>(), " 0 years =", value(actuarial::cashflow::annuity(0)).item<double>());
}
```

Output

```text
3 years = 2.856250  5 years = 4.520664  0 years = 0.000000
```

### Example 5: the errors

A negative number of years and a non-finite amount are refused; zero years and a negative amount are accepted.

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
  report([&] { (void)actuarial::cashflow::annuity(-1); });
  report([&] { (void)actuarial::cashflow::annuity(3, std::nan("")); });
  out.writeln("annuity(0) horizon", actuarial::cashflow::annuity(0).horizon(), " amount -1 survival payments", actuarial::cashflow::annuity(3, -1.0).survival_payments().items<double>());
}
```

Output

```text
cashflow::annuity: years must be >= 0
cashflow::annuity: amount must be finite
annuity(0) horizon 0  amount -1 survival payments [-1.000000, -1.000000, -1.000000, 0.000000]
```

## See Also

[`PaymentTiming`](PaymentTiming.md) chooses the timing, [`make`](make.md) is the general form and [`Valuation`](Valuation.md) values the result.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="make.html">
    <span class="label">Previous</span>
    <span class="title">make</span>
  </a>
  <a class="next-link" href="death.html">
    <span class="label">Next</span>
    <span class="title">death</span>
  </a>
</div>
<!-- apiref:nav:end -->
