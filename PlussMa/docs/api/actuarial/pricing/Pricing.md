<!-- generated from Pricing.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::Pricing`

A pricing calculation with its loads already bound.

## Signature

```cpp
using actuarial::Pricing = actuarial::Operator<actuarial::PricingResult(const actuarial::Experience &)>;
```

## What It Represents

An [`Operator`](../foundation/Operator.md) from an [`Experience`](Experience.md) to a [`PricingResult`](PricingResult.md). [`price`](price.md) creates it with the expense and profit assumptions fixed; the experience varies per call. Bind the loads once and apply the operator to every portfolio, year or scenario, so the same assumptions are guaranteed to be behind every figure. Its key is `"price"`.

## Methods

An operator is called like a function and has the four members of [`Operator`](../foundation/Operator.md): [`defined`](../foundation/Operator/defined.md), [`key`](../foundation/Operator/key.md), [`operator bool`](../foundation/Operator/operator_bool.md) and [`operator()`](../foundation/Operator/operator_call.md), which does the work described here.

## Practical Examples

### Example 1: one operator, several portfolios

The same loads applied to two books.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const actuarial::Experience experience{losses, exposure, premium};
  const actuarial::Loads loads{50.0, 0.15, 0.05};
  const actuarial::Pricing price = actuarial::operators::price(loads);
  const actuarial::Experience worse{losses.mul(1.2), exposure, premium};
  out.writeln("book A =", price(experience).indicated_premium.items<double>());
  out.writeln("book B =", price(worse).indicated_premium.items<double>());
}
```

Output

```text
book A = [712.500000, 781.250000, 737.500000, 826.388889]
book B = [842.500000, 925.000000, 872.500000, 979.166667]
```

### Example 2: a scenario on the loads

Rebuild the operator to test another profit target.

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
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const actuarial::Experience experience{losses, exposure, premium};
  for (const double profit : {0.0, 0.05, 0.10})
    out.writeln("profit", profit, " cell 0 indicated", actuarial::operators::price(actuarial::Loads{50.0, 0.15, profit})(experience).indicated_premium.items<double>()[0]);
}
```

Output

```text
profit 0.000000  cell 0 indicated 670.588235
profit 0.050000  cell 0 indicated 712.500000
profit 0.100000  cell 0 indicated 760.000000
```

### Example 3: the key

The key of the operator.

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
  out.writeln(actuarial::operators::price().key(), actuarial::operators::price().defined());
}
```

Output

```text
price true
```

### Example 4: an empty operator

A default-constructed operator is not defined and throws when called.

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
  out.writeln("defined:", actuarial::Pricing{}.defined());
  report([&] { (void)actuarial::Pricing{}(actuarial::Experience{}); });
}
```

Output

```text
defined: false
actuarial Operator is undefined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`price`](price.md) creates it, [`Experience`](Experience.md) is its input and [`PricingResult`](PricingResult.md) its output.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="CredibilityInput.html">
    <span class="label">Previous</span>
    <span class="title">CredibilityInput</span>
  </a>
  <a class="next-link" href="Credibility.html">
    <span class="label">Next</span>
    <span class="title">Credibility</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
