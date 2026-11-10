<!-- generated from make.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::cashflow::make`

Any contingent schedule from its two aligned payment vectors.

## Signature

```cpp
actuarial::Cashflow actuarial::cashflow::make(const pluss::Series &survival_payments, const pluss::Series &death_payments);
```

## What It Does

Creates a `Cashflow` from the survival payments at times `0..n` (`n + 1` values) and the death payments at the ends of years `1..n` (`n` values). It is the general form: every standard benefit is a special case, and anything else, a benefit that rises or falls, premiums as negative payments, a refund of premiums on death, is a pair of vectors. The expected present value on a basis is {math}`\sum_t s_t v_t\,{}_tp_x+\sum_k d_k v_k\,{}_{k-1}p_x q_{x+k-1}`.

The two vectors must have lengths `n + 1` and `n`, be one-dimensional and finite. Negative values are allowed. A pair of lengths 1 and 0 is a zero-year cashflow paying only at time 0.

## Parameters

`const pluss::Series &survival_payments`
: One-dimensional, finite, `n + 1` values for times `0..n`.

`const pluss::Series &death_payments`
: One-dimensional, finite, `n` values for years `1..n`.

## Returns

A defined `Cashflow` of horizon `n`.

## Errors

Throws `cashflow::make: survival_payments must have one more value than death_payments`, `cashflow::make: survival_payments must be 1D` (and the same for `death_payments`), and `cashflow::make: survival_payments must be finite` (and the same for `death_payments`).

## Practical Examples

### Example 1: the shape rule

Four survival payments and three death payments make a three-year cashflow.

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
  const actuarial::Cashflow c = actuarial::cashflow::make(pluss::make_series({0.0, 0.0, 0.0, 5.0}), pluss::make_series({1.0, 1.0, 1.0}));
  out.writeln("horizon", c.horizon(), " survival", c.survival_payments().items<double>(), " death", c.death_payments().items<double>());
}
```

Output

```text
horizon 3  survival [0.0, 0.0, 0.0, 5.0]  death [1.0, 1.0, 1.0]
```

### Example 2: an increasing term insurance

A death benefit of 1, 2, 3, 4 and 5 over five years, valued on a basis, and checked against the sum written out.

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
  const actuarial::Cashflow increasing = actuarial::cashflow::make(pluss::series::zeros({6}), pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));
  const auto p = actuarial::survival_curve(qx).items<double>();
  const auto q = qx.items<double>();
  double by_hand = 0.0;
  for (int k = 0; k < 5; ++k) by_hand += (k + 1.0) * std::pow(1.0 / 1.04, k + 1) * p[k] * q[k];
  out.writeln("value =", value(increasing).item<double>(), " by the sum =", by_hand);
}
```

Output

```text
value = 0.226524  by the sum = 0.226524
```

### Example 3: benefits less premiums

Premiums of 0.03 at times 0..4 are negative survival payments; with a death benefit of 1 the net value is from the policyholder's side, and it is negative because the premiums outweigh the expected benefit.

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
  const actuarial::Cashflow net = actuarial::cashflow::make(pluss::make_series({-0.03, -0.03, -0.03, -0.03, -0.03, 0.0}), pluss::make_series({1.0, 1.0, 1.0, 1.0, 1.0}));
  out.writeln("benefits", value(actuarial::cashflow::death(5)).item<double>(), " premiums", value(actuarial::cashflow::annuity(5, 0.03)).item<double>(), " net", value(net).item<double>());
}
```

Output

```text
benefits 0.067934  premiums 0.135620  net -0.067686
```

### Example 4: return of premium on death

A premium of 0.02 per year, returned in full if the life dies: the death payment in year `k` is the premiums paid so far.

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
  const double premium = 0.02;
  const actuarial::Cashflow refund = actuarial::cashflow::make(pluss::make_series({-premium, -premium, -premium, -premium, -premium, 0.0}), pluss::make_series({premium, 2 * premium, 3 * premium, 4 * premium, 5 * premium}));
  out.writeln("premiums less refund, per policy =", value(refund).item<double>());
}
```

Output

```text
premiums less refund, per policy = -0.085883
```

### Example 5: the errors

Mismatched lengths, a matrix, and non-finite payments are refused.

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
  report([&] { (void)actuarial::cashflow::make(pluss::make_series({1.0, 1.0}), pluss::make_series({1.0, 1.0})); });
  report([&] { (void)actuarial::cashflow::make(pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2}), pluss::make_series({1.0, 2.0})); });
  report([&] { (void)actuarial::cashflow::make(pluss::make_series({1.0, std::nan("")}), pluss::make_series({1.0})); });
}
```

Output

```text
cashflow::make: survival_payments must have one more value than death_payments
cashflow::make: survival_payments must be 1D
cashflow::make: survival_payments must be finite
```

## See Also

[`annuity`](annuity.md), [`death`](death.md), [`survival`](survival.md) and [`endowment`](endowment.md) are the standard cases; [`Cashflow`](Cashflow.md) is the type.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="curve.html">
    <span class="label">Previous</span>
    <span class="title">curve</span>
  </a>
  <a class="next-link" href="annuity.html">
    <span class="label">Next</span>
    <span class="title">annuity</span>
  </a>
</div>
<!-- apiref:nav:end -->
