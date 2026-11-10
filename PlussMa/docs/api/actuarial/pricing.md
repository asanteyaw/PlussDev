<!-- generated from pricing.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Pricing

The pricing family turns loss experience into a rate. [`operators::price`](pricing/price.md) takes expense and profit [`Loads`](pricing/Loads.md) and returns a [`Pricing`](pricing/Pricing.md) that maps an [`Experience`](pricing/Experience.md) to a [`PricingResult`](pricing/PricingResult.md): the pure premium, the indicated premium and the rate change, linked so the three cannot drift apart. [`operators::credibility`](pricing/credibility_operator.md) blends a cell's own rate with a manual rate by how much exposure stands behind it.

## What This Family Owns

| You want | Use |
|---|---|
| The observed losses, exposure and premium | [`Experience`](pricing/Experience.md) |
| Expense and profit assumptions | [`Loads`](pricing/Loads.md) |
| The indicated rate and rate change | [`operators::price`](pricing/price.md), [`Pricing`](pricing/Pricing.md), [`PricingResult`](pricing/PricingResult.md) |
| To stabilise the rates of thin cells | [`operators::credibility`](pricing/credibility_operator.md), [`Credibility`](pricing/Credibility.md), [`CredibilityInput`](pricing/CredibilityInput.md) |

Both paths take `Series` of the same length (or the same shape, for a grid of cells), keep the platform of the data, and reject assumptions that would make the result undefined.

## Practical Examples

### Example 1: an indication

Pure premium, loaded premium and rate change for four cells.

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
  const auto r = actuarial::operators::price(actuarial::Loads{50.0, 0.15, 0.05})(experience);
  out.writeln("pure      =", r.pure_premium.items<double>());
  out.writeln("indicated =", r.indicated_premium.items<double>());
  out.writeln("change    =", r.rate_change.items<double>());
}
```

Output

```text
pure      = [520.000000, 575.000000, 540.000000, 611.111111]
indicated = [712.500000, 781.250000, 737.500000, 826.388889]
change    = [0.017857, 0.085069, -0.003378, 0.087354]
```

### Example 2: credibility

Blend the experience of three cells with a manual rate; more exposure means more weight on the cell.

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
  const actuarial::CredibilityInput input{pluss::make_series({0.80, 0.80, 0.80}), pluss::make_series({0.65, 0.65, 0.65}), pluss::make_series({0.0, 100.0, 10000.0})};
  out.writeln(actuarial::operators::credibility(100.0)(input).items<double>());
}
```

Output

```text
[0.650000, 0.725000, 0.798515]
```

### Example 3: credibility before pricing

Smooth the cells' pure premiums toward the book average, then compare with the raw ones.

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
  const auto raw = actuarial::operators::price()(experience).pure_premium;
  const pluss::Series manual = pluss::series::full({4}, raw.mean().item<double>());
  out.writeln("raw     =", raw.items<double>());
  out.writeln("blended =", actuarial::operators::credibility(1500.0)(actuarial::CredibilityInput{raw, manual, exposure}).items<double>());
}
```

Output

```text
raw     = [520.000000, 575.000000, 540.000000, 611.111111]
blended = [544.916663, 567.515428, 550.763885, 588.573229]
```

## Related Pages

Use the persistent sidebar to open each type or factory. The {guide}`actuarial guide <actuarial>` develops longer workflows and mathematical background.

```{toctree}
:maxdepth: 1
:hidden:

pricing/Experience
pricing/Loads
pricing/PricingResult
pricing/CredibilityInput
pricing/Pricing
pricing/Credibility
pricing/price
pricing/credibility_operator
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

pricing/Experience
pricing/Loads
pricing/PricingResult
pricing/CredibilityInput
pricing/Pricing
pricing/Credibility
pricing/price
pricing/credibility_operator
```
<!-- apiref:toctree:end -->
