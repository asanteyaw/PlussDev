<!-- generated from loss_ratio.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::loss_ratio`

Aggregate loss ratio of a book.

## Signature

```cpp
pluss::Series actuarial::models::loss_ratio(const pluss::Series &incurred_losses, const pluss::Series &earned_premium);
```

## What It Does

Returns the **loss ratio** of the whole book: total losses over total earned premium, {math}`\sum L_i/\sum P_i`, weighting each cell by its premium. For the ratio of each cell separately use [`ratio_curve`](ratio_curve.md). The result is a single number, so cells with zero exposure or premium simply contribute their losses to the numerator without changing the denominator, and the call fails only when the whole denominator is zero.

## Parameters

`incurred_losses`
: One-dimensional, finite series of losses, 0 or more.

`earned_premium`
: One-dimensional, finite series of the same length, 0 or more.

## Returns

A rank-zero series: the aggregate loss ratio.

## Errors

Throws `loss_ratio: numerator must be defined`, `must be 1D`, `must be finite` and `must be >= 0`, the same for `earned_premium`, `loss_ratio: numerator and earned_premium must have same length` and `loss_ratio: aggregate earned_premium must be > 0` when the total is zero.

## Practical Examples

### Example 1: a four-cell book

Total losses over total earned premium.

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
  out.writeln(actuarial::models::loss_ratio(losses, premium).item<double>());
}
```

Output

```text
0.832000
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
  out.writeln(losses.sum().item<double>() / premium.sum().item<double>(), actuarial::models::loss_ratio(losses, premium).item<double>());
}
```

Output

```text
0.832000 0.832000
```

### Example 3: weighted average of the cell ratios

The aggregate is the earned premium-weighted average of the cell figures from [`ratio_curve`](ratio_curve.md).

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
  const pluss::Series cells = actuarial::models::ratio_curve(losses, premium);
  out.writeln(cells.mul(premium).sum().item<double>() / premium.sum().item<double>(), actuarial::models::loss_ratio(losses, premium).item<double>());
  out.writeln("cells:", cells.items<double>());
}
```

Output

```text
0.832000 0.832000
cells: [0.742857, 0.841463, 0.810000, 0.894309]
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
  out.writeln(actuarial::models::loss_ratio(pluss::make_series({100.0, 50.0}), pluss::make_series({10.0, 0.0})).item<double>());
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
  report([&] { (void)actuarial::models::loss_ratio(pluss::make_series({1.0, 2.0}), pluss::make_series({1.0, 2.0, 3.0})); });
  report([&] { (void)actuarial::models::loss_ratio(pluss::make_series({1.0, 2.0}), pluss::make_series({0.0, 0.0})); });
  report([&] { (void)actuarial::models::loss_ratio(pluss::make_series({-1.0, 2.0}), pluss::make_series({1.0, 2.0})); });
  report([&] { (void)actuarial::models::loss_ratio(pluss::make_series({1.0, 2.0}).reshape({1, 2}), pluss::make_series({1.0, 2.0}).reshape({1, 2})); });
}
```

Output

```text
loss_ratio: numerator and earned_premium must have same length
loss_ratio: aggregate earned_premium must be > 0
loss_ratio: numerator must be >= 0
loss_ratio: numerator must be 1D
```

## See Also

- [`ratio_curve`](ratio_curve.md): the figure for each cell
- [`operators::price`](../pricing/price.md): the operator that runs these steps together

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pure_premium.html">
    <span class="label">Previous</span>
    <span class="title">pure_premium</span>
  </a>
  <a class="next-link" href="premium_curve.html">
    <span class="label">Next</span>
    <span class="title">premium_curve</span>
  </a>
</div>
<!-- apiref:nav:end -->
