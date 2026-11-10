<!-- generated from flat.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::discount::flat`

A level annual effective interest rate.

## Signature

```cpp
actuarial::Discount actuarial::discount::flat(double annual_effective_rate);
```

## What It Does

Creates a `Discount` whose factor for time `t` is {math}`(1+i)^{-t}`, with `i` the annual effective rate. The factors are generated only when a horizon is known, by [`Discount::factors`](Discount/factors.md) or by a valuation, so one flat discount serves any horizon.

Use it for a single constant rate. When the rate varies with maturity use [`discount::curve`](curve.md). A rate of zero is allowed and gives no discounting; a negative rate is allowed and accumulates money; a rate of −1 or below has no discount factor and is refused.

## Parameters

`double annual_effective_rate`
: The rate `i`, finite and above −1.

## Returns

A defined `Discount`.

## Errors

Throws `discount::flat: rate must be finite` for a NaN or infinite rate and `discount::flat: rate must be > -1` for a rate of −1 or below.

## Practical Examples

### Example 1: the factors of a rate

4% for three years.

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
  out.writeln(actuarial::discount::flat(0.04).factors(3).items<double>());
}
```

Output

```text
[1.000000, 0.961538, 0.924556, 0.888996]
```

### Example 2: a basis and a value

A flat discount and a mortality law make a basis that values an annuity.

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
  const actuarial::Valuation value = actuarial::operators::value({mortality, actuarial::discount::flat(0.04)});
  out.writeln("annuity-due of 5 years at 4% =", value(actuarial::cashflow::annuity(5)).item<double>());
}
```

Output

```text
annuity-due of 5 years at 4% = 4.520664
```

### Example 3: the effect of the rate

A higher rate lowers the value of the same annuity.

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
  for (const double rate : {0.0, 0.02, 0.04, 0.08}) {
    const actuarial::Valuation value = actuarial::operators::value({mortality, actuarial::discount::flat(rate)});
    out.writeln("rate", rate, " annuity-due", value(actuarial::cashflow::annuity(5)).item<double>());
  }
}
```

Output

```text
rate 0.000000  annuity-due 4.876711
rate 0.020000  annuity-due 4.691771
rate 0.040000  annuity-due 4.520664
rate 0.080000  annuity-due 4.214771
```

### Example 4: the errors

A rate of −1 or below, and a non-finite rate, are refused.

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
  report([&] { (void)actuarial::discount::flat(-1.0); });
  report([&] { (void)actuarial::discount::flat(-2.0); });
  report([&] { (void)actuarial::discount::flat(std::nan("")); });
  out.writeln("a rate of -0.5 is accepted: factors", actuarial::discount::flat(-0.5).factors(2).items<double>());
}
```

Output

```text
discount::flat: rate must be > -1
discount::flat: rate must be > -1
discount::flat: rate must be finite
a rate of -0.5 is accepted: factors [1.000000, 2.000000, 4.000000]
```

## See Also

[`curve`](curve.md) is the explicit form, [`Discount`](Discount.md) the type and [`Basis`](Basis.md) pairs it with mortality.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Valuation.html">
    <span class="label">Previous</span>
    <span class="title">Valuation</span>
  </a>
  <a class="next-link" href="curve.html">
    <span class="label">Next</span>
    <span class="title">curve</span>
  </a>
</div>
<!-- apiref:nav:end -->
