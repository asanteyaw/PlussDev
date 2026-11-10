<!-- generated from term_insurance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::term_insurance`

Expected present value of a term insurance of 1.

## Signature

```cpp
pluss::Series actuarial::term_insurance(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns {math}`A^1_{x:\overline{n}\vert}=\sum_{k=0}^{n-1}v^{k+1}\,{}_kp_x\,q_{x+k}`: a benefit of 1 paid at the end of the year of death if death occurs within `n` years, and nothing otherwise. It is [`cashflow::death`](../valuation/death.md) on a flat rate. Add the [`pure_endowment`](pure_endowment.md) to cover survival as well and the result is the [`endowment_insurance`](endowment_insurance.md).

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

The cost of insuring a unit against death within five years.

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
  out.writeln(actuarial::term_insurance(qx, 0.04).item<double>());
}
```

Output

```text
0.067934
```

### Example 2: by hand

Each year's death probability, discounted to the end of that year.

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
  const auto pmf = actuarial::mortality::from_qx(qx).lifetime_pmf().items<double>();
  double total = 0.0;
  for (int k = 0; k < 5; ++k) total += std::pow(v, k + 1) * pmf[k];
  out.writeln(total, actuarial::term_insurance(qx, 0.04).item<double>());
}
```

Output

```text
0.06793404 0.06793404
```

### Example 3: no interest

At a zero rate the value is the probability of dying within the term.

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
  out.writeln(actuarial::term_insurance(qx, 0.0).item<double>(), 1.0 - actuarial::survival_curve(qx).items<double>().back());
}
```

Output

```text
0.07754074 0.07754074
```

### Example 4: term plus endowment

The sum with the pure endowment is the endowment insurance.

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
  out.writeln(actuarial::term_insurance(qx, 0.04).item<double>() + actuarial::pure_endowment(qx, 0.04).item<double>(), actuarial::endowment_insurance(qx, 0.04).item<double>());
}
```

Output

```text
0.82612831 0.82612831
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
    out.writeln("i =", i, " value =", actuarial::term_insurance(qx, i).item<double>());
}
```

Output

```text
i = 0.000000  value = 0.077541
i = 0.020000  value = 0.072505
i = 0.040000  value = 0.067934
i = 0.080000  value = 0.059984
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
    out.writeln("n =", n, " value =", actuarial::term_insurance(full.slice(0, 0, n, 1), 0.04).item<double>());
}
```

Output

```text
n = 2  value = 0.020599
n = 4  value = 0.049290
n = 7  value = 0.114964
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
  report([&] { (void)actuarial::term_insurance(pluss::make_series({0.1, 1.5}), 0.04); });
  report([&] { (void)actuarial::term_insurance(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::term_insurance(pluss::make_series({0.1, 0.2}), std::nan("")); });
  report([&] { (void)actuarial::term_insurance(pluss::make_series({0.1, 0.2}).reshape({1, 2}), 0.04); });
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

- [`pure_endowment`](pure_endowment.md): the survival benefit
- [`endowment_insurance`](endowment_insurance.md): both
- [`Valuation`](../valuation/Valuation.md): the general operator behind these values
- [`Mortality`](../mortality/Mortality.md): the law as a value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="annuity_immediate.html">
    <span class="label">Previous</span>
    <span class="title">annuity_immediate</span>
  </a>
  <a class="next-link" href="pure_endowment.html">
    <span class="label">Next</span>
    <span class="title">pure_endowment</span>
  </a>
</div>
<!-- apiref:nav:end -->
