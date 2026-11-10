<!-- generated from PaymentTiming.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::PaymentTiming`

When the payments of an annuity fall in each year: at the start or at the end.

## Signature

```cpp
actuarial::PaymentTiming actuarial::PaymentTiming::Start;
actuarial::PaymentTiming actuarial::PaymentTiming::End;
actuarial::Cashflow actuarial::cashflow::annuity(std::int64_t years, double amount = 1.0, actuarial::PaymentTiming timing = actuarial::PaymentTiming::Start);
```

## Fields

`PaymentTiming` is the `timing` argument of [`cashflow::annuity`](annuity.md). The two conventions differ by one payment at each end and, since a payment at an earlier time is worth more and more likely to be made, the annuity-due is always worth the more.

### `Start`

Payments at times `0, 1, ..., years−1`: an **annuity-due**, the usual form for premiums and for a pension paid in advance. This is the default. The program prints the schedule of a three-year annuity-due.

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
  const actuarial::Cashflow c = actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::Start);
  out.writeln("survival payments at times 0..3 =", c.survival_payments().items<double>());
}
```

Output

```text
survival payments at times 0..3 = [1.0, 1.0, 1.0, 0.0]
```

### `End`

Payments at times `1, 2, ..., years`: an **annuity-immediate**, the usual form for a loan repayment or a pension paid in arrear. The program prints the schedule.

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
  const actuarial::Cashflow c = actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::End);
  out.writeln("survival payments at times 0..3 =", c.survival_payments().items<double>());
}
```

Output

```text
survival payments at times 0..3 = [0.0, 1.0, 1.0, 1.0]
```

## Both together

The two timings valued on one basis. The due annuity exceeds the immediate annuity by {math}`1-v^n\,{}_np_x`.

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
  const double due = value(actuarial::cashflow::annuity(5, 1.0, actuarial::PaymentTiming::Start)).item<double>();
  const double immediate = value(actuarial::cashflow::annuity(5, 1.0, actuarial::PaymentTiming::End)).item<double>();
  out.writeln("Start =", due, " End =", immediate, " difference =", due - immediate);
}
```

Output

```text
Start = 4.520664  End = 4.278858  difference = 0.241806
```

## The default

Omitting the argument means `Start`.

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
  out.writeln("default equals Start:", actuarial::cashflow::annuity(3).survival_payments().items<double>() == actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::Start).survival_payments().items<double>());
  out.writeln("underlying values: Start =", static_cast<int>(actuarial::PaymentTiming::Start), " End =", static_cast<int>(actuarial::PaymentTiming::End));
}
```

Output

```text
default equals Start: true
underlying values: Start = 0  End = 1
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`cashflow::annuity`](annuity.md) takes the timing, and [`Cashflow`](Cashflow.md) holds the resulting schedule.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Cashflow.html">
    <span class="label">Previous</span>
    <span class="title">Cashflow</span>
  </a>
  <a class="next-link" href="Valuation.html">
    <span class="label">Next</span>
    <span class="title">Valuation</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
