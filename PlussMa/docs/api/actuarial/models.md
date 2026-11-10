<!-- generated from models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Rating and reserving formulas

The model functions are the individual steps of a rate indication and of a reserve estimate, each a free function on plain `Series`: measure the pure premium and loss ratio, blend them with a manual rate by credibility, load them for expenses and profit, compare with the current premium; and on the reserving side estimate the development factors, form the pattern to ultimate and apply it, or add the unreported share of an a priori ultimate. The operators in [Pricing](pricing.md) and [Reserving](reserving.md) run these steps together on typed inputs; use the functions when you want one step, or want to substitute your own for another.

## What This Family Owns

| You want | Use |
|---|---|
| The expected loss from exposure, frequency and severity | [`claim_cost`](models/claim_cost.md) |
| The pure premium or loss ratio of the whole book | [`pure_premium`](models/pure_premium.md), [`loss_ratio`](models/loss_ratio.md) |
| The same for each cell | [`premium_curve`](models/premium_curve.md), [`ratio_curve`](models/ratio_curve.md) |
| The weight a cell's own experience deserves | [`credibility_weight`](models/credibility_weight.md) |
| The experience blended with a manual rate | [`credibility_blend`](models/credibility_blend.md) |
| The premium that covers expenses and profit | [`indicated_premium`](models/indicated_premium.md) |
| How far the premium must move | [`rate_change`](models/rate_change.md), or the whole chain [`experience_change`](models/experience_change.md) |
| A reserve from an a priori ultimate | [`bf_reserve`](models/bf_reserve.md), [`bf_ultimate`](models/bf_ultimate.md) |
| Development factors and the pattern to ultimate | [`age_factors`](models/age_factors.md), [`ladder_cdf`](models/ladder_cdf.md) |
| Chain-ladder ultimates and reserves from a pattern | [`ladder_ultimate`](models/ladder_ultimate.md), [`ladder_reserve`](models/ladder_reserve.md) |

The aggregate functions ([`pure_premium`](models/pure_premium.md), [`loss_ratio`](models/loss_ratio.md)) return one number for a book; the curve functions return one number per cell and give zero for an empty cell. Series arguments are one-dimensional and of the same length unless a function says otherwise.

## Practical Examples

### Example 1: a rate indication, step by step

Measure, load and compare, then check that the single-call form gives the same answer.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series losses = pluss::make_series({520000.0, 690000.0, 810000.0, 1100000.0});
  const pluss::Series exposure = pluss::make_series({1000.0, 1200.0, 1500.0, 1800.0});
  const pluss::Series premium = pluss::make_series({700.0, 720.0, 740.0, 760.0});
  const pluss::Series pure = actuarial::models::premium_curve(losses, exposure);
  const pluss::Series indicated = actuarial::models::indicated_premium(pure, 50.0, 0.15, 0.05);
  out.writeln("pure      =", pure.items<double>());
  out.writeln("indicated =", indicated.items<double>());
  out.writeln("change    =", actuarial::models::rate_change(premium, indicated).items<double>());
  out.writeln("one call  =", actuarial::models::experience_change(losses, exposure, premium, 50.0, 0.15, 0.05).items<double>());
}
```

Output

```text
pure      = [520.0000, 575.0000, 540.0000, 611.1111]
indicated = [712.5000, 781.2500, 737.5000, 826.3889]
change    = [0.0179, 0.0851, -0.0034, 0.0874]
one call  = [0.0179, 0.0851, -0.0034, 0.0874]
```

### Example 2: a reserve from the development pattern

Estimate the factors, form the pattern and apply it to the latest diagonal.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  const pluss::Series factors = actuarial::models::age_factors(triangle);
  const pluss::Series cdf = actuarial::models::ladder_cdf(factors);
  out.writeln("factors  =", factors.items<double>());
  out.writeln("cdf      =", cdf.items<double>());
  out.writeln("ultimate =", actuarial::models::ladder_ultimate(triangle, cdf).items<double>());
  out.writeln("reserve  =", actuarial::models::ladder_reserve(triangle, cdf).items<double>());
}
```

Output

```text
factors  = [1.8030, 1.2368, 1.0909]
cdf      = [2.4328, 1.3493, 1.0909, 1.0000]
ultimate = [240.0000, 272.7273, 290.0957, 316.2636]
reserve  = [0.0000, 22.7273, 75.0957, 186.2636]
```

### Example 3: the Bornhuetter-Ferguson reserve using the same pattern

The share reported at each age is the reciprocal of the cumulative factor.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  const pluss::Series cdf = actuarial::models::ladder_cdf(actuarial::models::age_factors(triangle));
  const pluss::Series pct = pluss::series::full({4}, 1.0).div(cdf);
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  out.writeln("percent reported =", pct.items<double>());
  out.writeln("BF reserve       =", actuarial::models::bf_reserve(expected, pct).items<double>());
}
```

Output

```text
percent reported = [0.4110, 0.7411, 0.9167, 1.0000]
BF reserve       = [147.2376, 72.4823, 25.0000, 0.0000]
```

## Related Pages

Use the persistent sidebar to open each function. The {guide}`actuarial guide <actuarial>` develops rate indications and reserving with longer workflows.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

models/claim_cost
models/pure_premium
models/loss_ratio
models/premium_curve
models/ratio_curve
models/credibility_weight
models/credibility_blend
models/indicated_premium
models/rate_change
models/experience_change
models/bf_ultimate
models/bf_reserve
models/age_factors
models/ladder_cdf
models/ladder_ultimate
models/ladder_reserve
```
<!-- apiref:toctree:end -->
