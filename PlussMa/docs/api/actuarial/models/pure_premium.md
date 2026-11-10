<!-- generated from pure_premium.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::pure_premium`

Aggregate pure premium of a book.

## Signature

```cpp
pluss::Series actuarial::models::pure_premium(const pluss::Series &incurred_losses, const pluss::Series &exposure);
```

## What It Does

Returns the **pure premium** of the whole book: total losses over total exposure, {math}`\sum L_i/\sum E_i`. It weights each cell by its exposure, so a large cell counts more than a small one, which is what the overall rate needs. For the rate of each cell separately use [`premium_curve`](premium_curve.md). The result is a single number, so cells with zero exposure or premium simply contribute their losses to the numerator without changing the denominator, and the call fails only when the whole denominator is zero.

## Parameters

`incurred_losses`
: One-dimensional, finite series of losses, 0 or more.

`exposure`
: One-dimensional, finite series of the same length, 0 or more.

## Returns

A rank-zero series: the aggregate pure premium.

## Errors

Throws `pure_premium: numerator must be defined`, `must be 1D`, `must be finite` and `must be >= 0`, the same for `exposure`, `pure_premium: numerator and exposure must have same length` and `pure_premium: aggregate exposure must be > 0` when the total is zero.

## Practical Examples

### Example 1: a four-cell book

Total losses over total exposure.

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
  const pluss::Series premium = pluss::make_series({700000.0, 820000.0, 1000000.0, 1230000.0});
  out.writeln(actuarial::models::pure_premium(losses, exposure).item<double>());
}
```

Output

```text
567.272727
```

### Example 2: by hand

The ratio of the two sums.

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
  const pluss::Series premium = pluss::make_series({700000.0, 820000.0, 1000000.0, 1230000.0});
  out.writeln(losses.sum().item<double>() / exposure.sum().item<double>(), actuarial::models::pure_premium(losses, exposure).item<double>());
}
```

Output

```text
567.272727 567.272727
```

### Example 3: weighted average of the cell ratios

The aggregate is the exposure-weighted average of the cell figures from [`premium_curve`](premium_curve.md).

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
  const pluss::Series premium = pluss::make_series({700000.0, 820000.0, 1000000.0, 1230000.0});
  const pluss::Series cells = actuarial::models::premium_curve(losses, exposure);
  out.writeln(cells.mul(exposure).sum().item<double>() / exposure.sum().item<double>(), actuarial::models::pure_premium(losses, exposure).item<double>());
  out.writeln("cells:", cells.items<double>());
}
```

Output

```text
567.272727 567.272727
cells: [520.000000, 575.000000, 540.000000, 611.111111]
```

### Example 4: an empty cell

A cell with zero denominator keeps its losses in the numerator.

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
  out.writeln(actuarial::models::pure_premium(pluss::make_series({100.0, 50.0}), pluss::make_series({10.0, 0.0})).item<double>());
}
```

Output

```text
15.000000
```

### Example 5: the errors

A length mismatch, an all-zero denominator and invalid values are refused.

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
  report([&] { (void)actuarial::models::pure_premium(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 2.0, 3.0})); });
  report([&] { (void)actuarial::models::pure_premium(pluss::make_series({1.0, 2.0}), pluss::make_series({0.0, 0.0})); });
  report([&] { (void)actuarial::models::pure_premium(pluss::make_series({-1.0, 2.0}), pluss::make_series({1.0, 2.0})); });
  report([&] { (void)actuarial::models::pure_premium(pluss::make_series({1.0, 2.0}).reshape({1, 2}), pluss::make_series({1.0, 2.0}).reshape({1, 2})); });
}
```

Output

```text
pure_premium: numerator and exposure must have same length
pure_premium: aggregate exposure must be > 0
pure_premium: numerator must be >= 0
pure_premium: numerator must be 1D
```

## See Also

- [`premium_curve`](premium_curve.md): the figure for each cell
- [`operators::price`](../pricing/price.md): the operator that runs these steps together

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="claim_cost.html">
    <span class="label">Previous</span>
    <span class="title">claim_cost</span>
  </a>
  <a class="next-link" href="loss_ratio.html">
    <span class="label">Next</span>
    <span class="title">loss_ratio</span>
  </a>
</div>
<!-- apiref:nav:end -->
