<!-- generated from curve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::discount::curve`

Explicit discount factors, one per year from time zero.

## Signature

```cpp
actuarial::Discount actuarial::discount::curve(const pluss::Series &factors);
```

## What It Does

Creates a `Discount` from the factors {math}`v_0,v_1,\dots` themselves, with position `t` holding the factor for time `t`. A curve of `n + 1` values serves horizons up to `n` years; a longer horizon is refused when the valuation asks for the factors. This is how a yield curve, a swap curve or any term structure enters a valuation: each cashflow is discounted at the factor of its own date, not at one rate.

The factors must be positive and the vector one-dimensional with at least the entry for time zero. The curve is stored as given: it is not required to start at one or to decrease. A first factor other than 1 changes only the value of a payment at time 0, and a rising segment implies a negative forward rate. Use `flat` for a constant rate.

## Parameters

`const pluss::Series &factors`
: A one-dimensional `Series` of positive factors, position `t` for time `t`.

## Returns

A defined `Discount`.

## Errors

Throws `discount::curve: factors must be 1D`, `discount::curve: factors must contain time zero` for an empty vector, and `discount::curve: factors must be > 0` for a non-positive factor.

## Practical Examples

### Example 1: a curve and its factors

Four factors cover a three-year horizon.

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
  const actuarial::Discount curve = actuarial::discount::curve(pluss::make_series({1.0, 0.97, 0.93, 0.88}));
  out.writeln(curve.factors(3).items<double>());
}
```

Output

```text
[1.000000, 0.970000, 0.930000, 0.880000]
```

### Example 2: a term structure

Spot rates of 3%, 3.5% and 4% for years 1 to 3 give factors {math}`(1+r_t)^{-t}`; the annuity is valued against them and against a flat 3.5%.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(pluss::make_series({0.010, 0.012, 0.015}));
  const std::vector<double> spot = {0.0, 0.03, 0.035, 0.04};
  std::vector<double> factors;
  for (int t = 0; t <= 3; ++t) factors.push_back(std::pow(1.0 + spot[t], -t));
  const auto value = [&](const actuarial::Discount &d) { return actuarial::operators::value({mortality, d})(actuarial::cashflow::annuity(3, 1.0, actuarial::PaymentTiming::End)).item<double>(); };
  out.writeln("factors        =", factors);
  out.writeln("on the curve   =", value(actuarial::discount::curve(pluss::make_series(factors))));
  out.writeln("on a flat 3.5% =", value(actuarial::discount::flat(0.035)));
}
```

Output

```text
factors        = [1.000000, 0.970874, 0.933511, 0.888996]
on the curve   = 2.730752
on a flat 3.5% = 2.738582
```

### Example 3: the same value as a flat rate

A curve built from the flat factors reproduces the flat valuation.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024}));
  std::vector<double> factors;
  for (int t = 0; t <= 5; ++t) factors.push_back(std::pow(1.04, -t));
  const auto value = [&](const actuarial::Discount &d) { return actuarial::operators::value({mortality, d})(actuarial::cashflow::annuity(5)).item<double>(); };
  out.writeln(value(actuarial::discount::curve(pluss::make_series(factors))), value(actuarial::discount::flat(0.04)));
}
```

Output

```text
4.520664 4.520664
```

### Example 4: what the curve is not checked for

The factors are stored as given: a first factor of 0.98 changes only the payment at time 0, whose value falls by exactly 0.02, and a curve may rise.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(pluss::make_series({0.010, 0.012, 0.015}));
  const auto value = [&](const std::vector<double> &f) { return actuarial::operators::value({mortality, actuarial::discount::curve(pluss::make_series(f))})(actuarial::cashflow::annuity(3)).item<double>(); };
  out.writeln("starting at 1.00 =", value({1.0, 0.96, 0.92, 0.88}), " starting at 0.98 =", value({0.98, 0.96, 0.92, 0.88}), " difference", value({1.0, 0.96, 0.92, 0.88}) - value({0.98, 0.96, 0.92, 0.88}));
}
```

Output

```text
starting at 1.00 = 2.850270  starting at 0.98 = 2.830270  difference 0.020000
```

### Example 5: the errors

A non-positive factor, a matrix, an empty vector and a horizon beyond the curve are refused.

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
  report([&] { (void)actuarial::discount::curve(pluss::make_series({1.0, -0.5})); });
  report([&] { (void)actuarial::discount::curve(pluss::make_series({1.0, 0.9, 0.8, 0.7}, {2, 2})); });
  report([&] { (void)actuarial::discount::curve(pluss::series::zeros({0})); });
  report([&] { (void)actuarial::discount::curve(pluss::make_series({1.0, 0.96})).factors(5); });
}
```

Output

```text
discount::curve: factors must be > 0
discount::curve: factors must be 1D
discount::curve: factors must contain time zero
Discount::factors: curve must contain at least horizon + 1 values
```

## See Also

[`flat`](flat.md) is the constant-rate form, [`Discount`](Discount.md) the type and [`Valuation`](Valuation.md) the operator that uses the factors.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flat.html">
    <span class="label">Previous</span>
    <span class="title">flat</span>
  </a>
  <a class="next-link" href="make.html">
    <span class="label">Next</span>
    <span class="title">make</span>
  </a>
</div>
<!-- apiref:nav:end -->
