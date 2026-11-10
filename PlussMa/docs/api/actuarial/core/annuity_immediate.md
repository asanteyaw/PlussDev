<!-- generated from annuity_immediate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::annuity_immediate`

Expected present value of a temporary annuity-immediate of 1 a year.

## Signature

```cpp
pluss::Series actuarial::annuity_immediate(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns {math}`a_{x:\overline{n}\vert}=\sum_{k=1}^{n}v^k\,{}_kp_x`: a payment of 1 at the end of each year, made only if the life is then alive, for at most `n` years. No payment is certain. It is [`cashflow::annuity`](../valuation/annuity.md) with payments at the end. The two annuities differ by the first payment and the last survival: {math}`\ddot a=1+a-{}_nE_x`, where {math}`{}_nE_x` is the [`pure_endowment`](pure_endowment.md).

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

`interest_rate`
: The annual effective rate {math}`i`, finite and greater than −1. A zero or negative rate is valid.

## Returns

A rank-zero series: the expected present value of a benefit of 1 per unit, read with `item<double>()`.

## Errors

Throws `survival_curve: qx must be defined`, `survival_curve: qx must be 1D`, `survival_curve: qx must be finite` and `survival_curve: qx values must lie in [0, 1]`, and `discount::flat: rate must be finite` and `discount::flat: rate must be > -1`.

## Practical Examples

### Example 1: five years at 4%

Five payments at the end of the years, each weighted by survival.

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
  out.writeln(actuarial::annuity_immediate(qx, 0.04).item<double>());
}
```

Output

```text
4.278858
```

### Example 2: by hand

The sum of discounted survival probabilities from year 1.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const double v = 1.0 / 1.04;
  const auto s = actuarial::survival_curve(qx).items<double>();
  double total = 0.0;
  for (int k = 1; k <= 5; ++k) total += std::pow(v, k) * s[k];
  out.writeln(total, actuarial::annuity_immediate(qx, 0.04).item<double>());
}
```

Output

```text
4.27885814 4.27885814
```

### Example 3: against the annuity-due

The identity {math}`\ddot a = 1 + a - {}_nE_x` holds.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(actuarial::annuity_due(qx, 0.04).item<double>(), 1.0 + actuarial::annuity_immediate(qx, 0.04).item<double>() - actuarial::pure_endowment(qx, 0.04).item<double>());
}
```

Output

```text
4.52066387 4.52066387
```

### Example 4: no interest

At a zero rate the value is the expected number of payments received.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(actuarial::annuity_immediate(qx, 0.0).item<double>(), actuarial::survival_curve(qx).slice(0, 1, 6, 1).sum().item<double>());
}
```

Output

```text
4.79917014 4.79917014
```

### Example 5: the effect of the interest rate

A higher rate lowers the value of money paid in the future.

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
  for (const double i : {0.0, 0.02, 0.04, 0.08})
    out.writeln("i =", i, " value =", actuarial::annuity_immediate(qx, i).item<double>());
}
```

Output

```text
i = 0.000000  value = 4.799170
i = 0.020000  value = 4.527271
i = 0.040000  value = 4.278858
i = 0.080000  value = 3.842582
```

### Example 6: the effect of the horizon

Each added year of mortality extends the benefit.

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
  const pluss::Series full = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024, 0.030, 0.037});
  for (const int n : {2, 4, 7})
    out.writeln("n =", n, " value =", actuarial::annuity_immediate(full.slice(0, 0, n, 1), 0.04).item<double>());
}
```

Output

```text
n = 2  value = 1.856250
n = 4  value = 3.520664
n = 7  value = 5.640825
```

### Example 7: the errors

Invalid probabilities and interest rates are refused.

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
  report([&] { (void)actuarial::annuity_immediate(pluss::make_series({0.1, 1.5}), 0.04); });
  report([&] { (void)actuarial::annuity_immediate(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::annuity_immediate(pluss::make_series({0.1, 0.2}), std::nan("")); });
  report([&] { (void)actuarial::annuity_immediate(pluss::make_series({0.1, 0.2}).reshape({1, 2}), 0.04); });
}
```

Output

```text
survival_curve: qx values must lie in [0, 1]
discount::flat: rate must be > -1
discount::flat: rate must be finite
survival_curve: qx must be 1D
```

## See Also

- [`annuity_due`](annuity_due.md): payments at the start of the year
- [`Valuation`](../valuation/Valuation.md): the general operator behind these values
- [`Mortality`](../mortality/Mortality.md): the law as a value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="annuity_due.html">
    <span class="label">Previous</span>
    <span class="title">annuity_due</span>
  </a>
  <a class="next-link" href="term_insurance.html">
    <span class="label">Next</span>
    <span class="title">term_insurance</span>
  </a>
</div>
<!-- apiref:nav:end -->
