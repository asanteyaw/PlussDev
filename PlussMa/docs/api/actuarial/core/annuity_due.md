<!-- generated from annuity_due.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::annuity_due`

Expected present value of a temporary annuity-due of 1 a year.

## Signature

```cpp
pluss::Series actuarial::annuity_due(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns {math}`\ddot a_{x:\overline{n}\vert}=\sum_{k=0}^{n-1}v^k\,{}_kp_x` with {math}`v=1/(1+i)`: a payment of 1 at the start of each year, made only if the life is then alive, for at most `n` years (the length of `qx`). The first payment is certain. It is [`cashflow::annuity`](../valuation/annuity.md) with payments at the start, valued on a flat rate; use it for the common case of a level rate and a probability vector, and the [`Valuation`](../valuation/Valuation.md) operator for a curve of rates or any other benefit. A horizon of zero gives zero.

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

The first payment is certain; each later one is discounted and weighted by the probability of being alive to receive it.

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
  out.writeln(actuarial::annuity_due(qx, 0.04).item<double>());
}
```

Output

```text
4.520664
```

### Example 2: by hand

The sum of discounted survival probabilities.

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
  const double by_hand = 1.0 + v * 0.99 + v * v * 0.99 * 0.988 + v * v * v * 0.99 * 0.988 * 0.985 + v * v * v * v * 0.99 * 0.988 * 0.985 * 0.981;
  out.writeln(by_hand, actuarial::annuity_due(qx, 0.04).item<double>());
}
```

Output

```text
4.52066387 4.52066387
```

### Example 3: no interest

At a zero rate the annuity is the expected number of payments.

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
  out.writeln(actuarial::annuity_due(qx, 0.0).item<double>(), actuarial::survival_curve(qx).slice(0, 0, 5, 1).sum().item<double>());
}
```

Output

```text
4.87671088 4.87671088
```

### Example 4: a negative rate

Money grows with time, so the annuity is worth more than the undiscounted count.

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
  out.writeln(actuarial::annuity_due(qx, -0.02).item<double>());
}
```

Output

```text
5.076993
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
    out.writeln("i =", i, " value =", actuarial::annuity_due(qx, i).item<double>());
}
```

Output

```text
i = 0.000000  value = 4.876711
i = 0.020000  value = 4.691771
i = 0.040000  value = 4.520664
i = 0.080000  value = 4.214771
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
    out.writeln("n =", n, " value =", actuarial::annuity_due(full.slice(0, 0, n, 1), 0.04).item<double>());
}
```

Output

```text
n = 2  value = 1.951923
n = 4  value = 3.712752
n = 7  value = 5.986020
```

### Example 7: no years

An empty horizon has no payments.

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
  out.writeln(actuarial::annuity_due(pluss::series::zeros({0}), 0.04).item<double>());
}
```

Output

```text
0.000000
```

### Example 8: the errors

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
  report([&] { (void)actuarial::annuity_due(pluss::make_series({0.1, 1.5}), 0.04); });
  report([&] { (void)actuarial::annuity_due(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::annuity_due(pluss::make_series({0.1, 0.2}), std::nan("")); });
  report([&] { (void)actuarial::annuity_due(pluss::make_series({0.1, 0.2}).reshape({1, 2}), 0.04); });
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

- [`annuity_immediate`](annuity_immediate.md): payments at the end of the year
- [`Valuation`](../valuation/Valuation.md): the general operator behind these values
- [`Mortality`](../mortality/Mortality.md): the law as a value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="survival_curve.html">
    <span class="label">Previous</span>
    <span class="title">survival_curve</span>
  </a>
  <a class="next-link" href="annuity_immediate.html">
    <span class="label">Next</span>
    <span class="title">annuity_immediate</span>
  </a>
</div>
<!-- apiref:nav:end -->
