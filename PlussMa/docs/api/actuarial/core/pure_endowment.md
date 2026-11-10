<!-- generated from pure_endowment.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::pure_endowment`

Expected present value of a pure endowment of 1.

## Signature

```cpp
pluss::Series actuarial::pure_endowment(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns {math}`{}_nE_x=v^n\,{}_np_x`: a benefit of 1 paid at the end of the term only if the life survives it, and nothing otherwise. It is [`cashflow::survival`](../valuation/survival.md) on a flat rate. It is the discount factor to the end of the horizon times the probability of getting there, which is why it falls with both the rate and the mortality.

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

The value today of 1 payable in five years if alive.

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
  out.writeln(actuarial::pure_endowment(qx, 0.04).item<double>());
}
```

Output

```text
0.758194
```

### Example 2: by hand

The discount factor times the survival probability.

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
  out.writeln(std::pow(1.0 / 1.04, 5.0) * actuarial::survival_curve(qx).items<double>().back(), actuarial::pure_endowment(qx, 0.04).item<double>());
}
```

Output

```text
0.75819427 0.75819427
```

### Example 3: no interest

At a zero rate the value is the survival probability.

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
  out.writeln(actuarial::pure_endowment(qx, 0.0).item<double>(), actuarial::survival_curve(qx).items<double>().back());
}
```

Output

```text
0.92245926 0.92245926
```

### Example 4: no mortality

Without deaths it is the plain discount factor.

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
  out.writeln(actuarial::pure_endowment(pluss::make_series({0.0, 0.0, 0.0}), 0.05).item<double>(), std::pow(1.05, -3.0));
}
```

Output

```text
0.86383760 0.86383760
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
    out.writeln("i =", i, " value =", actuarial::pure_endowment(qx, i).item<double>());
}
```

Output

```text
i = 0.000000  value = 0.922459
i = 0.020000  value = 0.835500
i = 0.040000  value = 0.758194
i = 0.080000  value = 0.627810
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
    out.writeln("n =", n, " value =", actuarial::pure_endowment(full.slice(0, 0, n, 1), 0.04).item<double>());
}
```

Output

```text
n = 2  value = 0.904327
n = 4  value = 0.807912
n = 7  value = 0.654805
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
  report([&] { (void)actuarial::pure_endowment(pluss::make_series({0.1, 1.5}), 0.04); });
  report([&] { (void)actuarial::pure_endowment(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::pure_endowment(pluss::make_series({0.1, 0.2}), std::nan("")); });
  report([&] { (void)actuarial::pure_endowment(pluss::make_series({0.1, 0.2}).reshape({1, 2}), 0.04); });
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

- [`term_insurance`](term_insurance.md): the death benefit
- [`endowment_insurance`](endowment_insurance.md): both
- [`Valuation`](../valuation/Valuation.md): the general operator behind these values
- [`Mortality`](../mortality/Mortality.md): the law as a value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="term_insurance.html">
    <span class="label">Previous</span>
    <span class="title">term_insurance</span>
  </a>
  <a class="next-link" href="endowment_insurance.html">
    <span class="label">Next</span>
    <span class="title">endowment_insurance</span>
  </a>
</div>
<!-- apiref:nav:end -->
