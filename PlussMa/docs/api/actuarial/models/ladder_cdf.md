<!-- generated from ladder_cdf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::ladder_cdf`

Cumulative development factors from age-to-age factors.

## Signature

```cpp
pluss::Series actuarial::models::ladder_cdf(const pluss::Series &age_to_age_factors);
```

## What It Does

Returns the factors that take claims at each age to ultimate: {math}`F_j=\prod_{k\ge j}f_k`, with 1 at the last age. The result has one more value than the input, ends in 1 and is non-increasing. Its reciprocal is the share of the ultimate reported at each age, which is what [`bf_ultimate`](bf_ultimate.md) needs as `percent_reported`. The factors must be at least 1: claims do not shrink in a cumulative triangle.

## Parameters

`age_to_age_factors`
: One-dimensional series of factors, finite and at least 1. An empty series gives the single value 1.

## Returns

A one-dimensional series of `n + 1` factors ending in 1.

## Errors

Throws `ladder_cdf: age_to_age_factors must be >= 1` (and `must be defined`, `must be 1D`, `must be finite`).

## Practical Examples

### Example 1: three factors

The pattern to ultimate.

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
  out.writeln(actuarial::models::ladder_cdf(pluss::make_series({1.8, 1.2, 1.1})).items<double>());
}
```

Output

```text
[2.376000, 1.320000, 1.100000, 1.000000]
```

### Example 2: by hand

Products from the right.

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
  out.writeln(1.8 * 1.2 * 1.1, 1.2 * 1.1, 1.1, 1.0);
}
```

Output

```text
2.376000 1.320000 1.100000 1.000000
```

### Example 3: percent reported

The reciprocals are the shares reported, as needed by the Bornhuetter-Ferguson functions.

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
  const pluss::Series cdf = actuarial::models::ladder_cdf(pluss::make_series({1.8, 1.2, 1.1}));
  out.writeln(pluss::series::full({4}, 1.0).div(cdf).items<double>());
}
```

Output

```text
[0.420875, 0.757576, 0.909091, 1.000000]
```

### Example 4: no factors

A one-age triangle has nothing to develop.

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
  out.writeln(actuarial::models::ladder_cdf(pluss::series::zeros({0})).items<double>());
}
```

Output

```text
[1.000000]
```

### Example 5: from a triangle

Estimate the factors, then the pattern.

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
  out.writeln(actuarial::models::ladder_cdf(actuarial::models::age_factors(triangle)).items<double>());
}
```

Output

```text
[2.4328, 1.3493, 1.0909, 1.0000]
```

### Example 6: the errors

A factor below 1, a matrix and a NaN.

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
  report([&] { (void)actuarial::models::ladder_cdf(pluss::make_series({1.8, 0.9})); });
  report([&] { (void)actuarial::models::ladder_cdf(pluss::make_series({1.8, 1.2}).reshape({1, 2})); });
  report([&] { (void)actuarial::models::ladder_cdf(pluss::make_series({1.8, std::nan("")})); });
}
```

Output

```text
ladder_cdf: age_to_age_factors must be >= 1
ladder_cdf: age_to_age_factors must be 1D
ladder_cdf: age_to_age_factors must be finite
```

## See Also

- [`age_factors`](age_factors.md): estimates the factors
- [`ladder_ultimate`](ladder_ultimate.md): applies the pattern

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="age_factors.html">
    <span class="label">Previous</span>
    <span class="title">age_factors</span>
  </a>
  <a class="next-link" href="ladder_ultimate.html">
    <span class="label">Next</span>
    <span class="title">ladder_ultimate</span>
  </a>
</div>
<!-- apiref:nav:end -->
