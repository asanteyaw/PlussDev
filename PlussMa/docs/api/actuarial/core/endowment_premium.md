<!-- generated from endowment_premium.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::endowment_premium`

The level annual net premium of an endowment insurance.

## Signature

```cpp
pluss::Series actuarial::endowment_premium(const pluss::Series &qx, double interest_rate);
```

## What It Does

Returns {math}`P=A_{x:\overline{n}\vert}\,/\,\ddot a_{x:\overline{n}\vert}`: the level premium, paid at the start of each year while the life survives, that makes the expected present value of premiums equal to that of the benefit of 1 (the equivalence principle). It is computed from the [`commutation_table`](commutation_table.md) as {math}`(M_0+D_n)/N_0`. It is a net premium: no expenses or profit. Use [`endowment_reserve`](endowment_reserve.md) for the reserve it builds up.

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

`interest_rate`
: The annual effective rate {math}`i`, finite and greater than −1. A zero or negative rate is valid.

## Returns

A rank-zero series: the premium per unit of benefit.

## Errors

Throws `endowment_premium: denominator must be > 0` when the annuity-due is zero (an empty horizon), and the `commutation_table: …` errors for invalid inputs.

## Practical Examples

### Example 1: a five-year endowment

The yearly premium per unit insured.

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
  out.writeln(actuarial::endowment_premium(qx, 0.04).item<double>());
}
```

Output

```text
0.18274491
```

### Example 2: the equivalence principle

The premium times the annuity-due equals the benefit's value.

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
  const double p = actuarial::endowment_premium(qx, 0.04).item<double>();
  out.writeln(p * actuarial::annuity_due(qx, 0.04).item<double>(), actuarial::endowment_insurance(qx, 0.04).item<double>());
}
```

Output

```text
0.82612831 0.82612831
```

### Example 3: the effect of the rate

A higher rate means the investment income does more of the work, so the premium falls.

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
    out.writeln("i =", i, " premium =", actuarial::endowment_premium(qx, i).item<double>());
}
```

Output

```text
i = 0.000000  premium = 0.205056
i = 0.020000  premium = 0.193531
i = 0.040000  premium = 0.182745
i = 0.080000  premium = 0.163187
```

### Example 4: no interest and no mortality

Saving 1 over three years costs one third a year.

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
  out.writeln(actuarial::endowment_premium(pluss::make_series({0.0, 0.0, 0.0}), 0.0).item<double>());
}
```

Output

```text
0.33333333
```

### Example 5: certain death in the first year

The benefit is paid at once at the end of year one, so the premium is its discount factor.

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
  out.writeln(actuarial::endowment_premium(pluss::make_series({1.0, 0.5}), 0.04).item<double>(), 1.0 / 1.04);
}
```

Output

```text
0.96153846 0.96153846
```

### Example 6: the errors

An empty horizon has no annuity to divide by; invalid inputs are refused.

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
  report([&] { (void)actuarial::endowment_premium(pluss::series::zeros({0}), 0.04); });
  report([&] { (void)actuarial::endowment_premium(pluss::make_series({0.1, 0.2}), -1.0); });
  report([&] { (void)actuarial::endowment_premium(pluss::make_series({1.5}), 0.04); });
}
```

Output

```text
endowment_premium: denominator must be > 0
commutation_table: interest_rate must be > -1
commutation_table: qx values must lie in [0, 1]
```

## See Also

- [`endowment_reserve`](endowment_reserve.md): the reserve it builds
- [`endowment_insurance`](endowment_insurance.md): the benefit value

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CommutationTable.html">
    <span class="label">Previous</span>
    <span class="title">CommutationTable</span>
  </a>
  <a class="next-link" href="endowment_reserve.html">
    <span class="label">Next</span>
    <span class="title">endowment_reserve</span>
  </a>
</div>
<!-- apiref:nav:end -->
