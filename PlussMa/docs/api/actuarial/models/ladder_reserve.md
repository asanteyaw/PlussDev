<!-- generated from ladder_reserve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::ladder_reserve`

Chain-ladder reserve for a triangle and a pattern.

## Signature

```cpp
pluss::Series actuarial::models::ladder_reserve(const pluss::Series &cumulative_triangle, const pluss::Series &cdf_to_ultimate);
```

## What It Does

Returns the estimated unpaid claims of each origin: {math}`C_i(F_j-1)`, never negative. The pattern is imposed, not estimated: select the factors yourself (from [`age_factors`](age_factors.md) and [`ladder_cdf`](ladder_cdf.md), a smoothed set, or a benchmark) and apply them to the latest diagonal of the triangle. It is [`operators::chain_ladder`](../reserving/chain_ladder.md) with a supplied pattern, on a plain matrix.

## Parameters

`cumulative_triangle`
: Two-dimensional cumulative claims, origins by ages, zero where unobserved.

`cdf_to_ultimate`
: One cumulative factor per development age, each at least 1, non-increasing and ending in 1.

## Returns

A one-dimensional series of `origins` values: the reserve of each origin.

## Errors

Throws `chain_ladder: cdf length must match development count`, `chain_ladder: final cdf value must equal 1`, `chain_ladder: cdf must be non-increasing`, `chain_ladder: cdf values must be >= 1` and the `triangle::cumulative: …` errors for an invalid matrix.

## Practical Examples

### Example 1: with the estimated pattern

Estimate the pattern from the triangle and apply it.

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
  out.writeln(actuarial::models::ladder_reserve(triangle, cdf).items<double>());
}
```

Output

```text
[0.0000, 22.7273, 75.0957, 186.2636]
```

### Example 2: with a selected pattern

Impose factors of your own.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  out.writeln(actuarial::models::ladder_reserve(triangle, pluss::make_series({1.8, 1.2, 1.1, 1.0})).items<double>());
}
```

Output

```text
[0.00, 25.00, 43.00, 104.00]
```

### Example 3: the same as the operator

On a triangle value the chain-ladder operator with the same pattern.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  const pluss::Series cdf = pluss::make_series({1.8, 1.2, 1.1, 1.0});
  out.writeln(actuarial::operators::chain_ladder(cdf)(actuarial::triangle::cumulative(triangle)).reserve.items<double>());
  out.writeln(actuarial::models::ladder_reserve(triangle, cdf).items<double>());
}
```

Output

```text
[0.00, 25.00, 43.00, 104.00]
[0.00, 25.00, 43.00, 104.00]
```

### Example 4: the relation between ultimate and reserve

Ultimate = reported + reserve, with the reported amounts the latest diagonal.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  const pluss::Series cdf = pluss::make_series({1.8, 1.2, 1.1, 1.0});
  out.writeln(actuarial::models::ladder_ultimate(triangle, cdf).items<double>());
  out.writeln(actuarial::models::ladder_reserve(triangle, cdf).add(pluss::make_series({240.0, 250.0, 215.0, 130.0})).items<double>());
}
```

Output

```text
[240.00, 275.00, 258.00, 234.00]
[240.00, 275.00, 258.00, 234.00]
```

### Example 5: the errors

A pattern of the wrong length, one that does not end in 1, and one that rises.

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
  report([&] { (void)actuarial::models::ladder_reserve(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}), pluss::make_series({1.8, 1.2, 1.0})); });
  report([&] { (void)actuarial::models::ladder_reserve(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}), pluss::make_series({1.8, 1.5, 1.2, 1.1})); });
  report([&] { (void)actuarial::models::ladder_reserve(pluss::make_series({100.0, 180.0, 220.0, 240.0, 110.0, 200.0, 250.0, 0.0, 120.0, 215.0, 0.0, 0.0, 130.0, 0.0, 0.0, 0.0}, {4, 4}), pluss::make_series({1.0, 1.2, 1.5, 1.8})); });
}
```

Output

```text
chain_ladder: cdf length must match development count
chain_ladder: final cdf value must equal 1
chain_ladder: cdf must be non-increasing
```

## See Also

- [`ladder_cdf`](ladder_cdf.md): the pattern
- [`ladder_ultimate`](ladder_ultimate.md): the companion
- [`operators::chain_ladder`](../reserving/chain_ladder.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ladder_ultimate.html">
    <span class="label">Previous</span>
    <span class="title">ladder_ultimate</span>
  </a>
</div>
<!-- apiref:nav:end -->
