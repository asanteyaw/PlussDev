<!-- generated from age_factors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::age_factors`

Volume-weighted age-to-age factors of a cumulative triangle.

## Signature

```cpp
pluss::Series actuarial::models::age_factors(const pluss::Series &cumulative_triangle);
```

## What It Does

Returns the factors that take cumulative claims from each development age to the next: {math}`f_j=\sum_iC_{i,j+1}/\sum_iC_{i,j}` over the origins observed at both ages, one value fewer than there are ages. The triangle is a matrix of origins by ages with zeros in the unobserved lower-right corner. It is the `age_to_age` of [`operators::chain_ladder`](../reserving/chain_ladder.md) on a triangle built from the matrix; use [`ladder_cdf`](ladder_cdf.md) to turn the factors into the pattern to ultimate.

## Parameters

`cumulative_triangle`
: Two-dimensional cumulative claims, origins by ages, zero where unobserved; validated as in [`triangle::cumulative`](../reserving/cumulative.md).

## Returns

A one-dimensional series of `ages − 1` factors.

## Errors

Throws the `triangle::cumulative: …` errors for an invalid matrix, and `chain_ladder: at least two development periods are required` for a single column.

## Practical Examples

### Example 1: a four-year triangle

The three factors.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  out.writeln(actuarial::models::age_factors(triangle).items<double>());
}
```

Output

```text
[1.803030, 1.236842, 1.090909]
```

### Example 2: by hand

The first factor is the second column over the first, for the origins observed at both.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  out.writeln((180.0 + 200.0 + 215.0) / (100.0 + 110.0 + 120.0), actuarial::models::age_factors(triangle).items<double>()[0]);
}
```

Output

```text
1.80303030 1.80303030
```

### Example 3: the pattern to ultimate

Feed the factors to ladder_cdf.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  out.writeln(actuarial::models::ladder_cdf(actuarial::models::age_factors(triangle)).items<double>());
}
```

Output

```text
[2.432797, 1.349282, 1.090909, 1.000000]
```

### Example 4: the same as the operator

On a triangle value, chain_ladder reports the same factors.

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
  const pluss::Series triangle = pluss::make_series({100.0, 180.0, 220.0, 240.0,
                                                     110.0, 200.0, 250.0, 0.0,
                                                     120.0, 215.0, 0.0, 0.0,
                                                     130.0, 0.0, 0.0, 0.0}, {4, 4});
  out.writeln(actuarial::operators::chain_ladder()(actuarial::triangle::cumulative(triangle)).age_to_age.items<double>());
  out.writeln(actuarial::models::age_factors(triangle).items<double>());
}
```

Output

```text
[1.803030, 1.236842, 1.090909]
[1.803030, 1.236842, 1.090909]
```

### Example 5: the errors

A matrix that decreases, a vector and a single column.

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
  report([&] { (void)actuarial::models::age_factors(pluss::make_series({100.0, 90.0, 0.0, 0.0}, {2, 2})); });
  report([&] { (void)actuarial::models::age_factors(pluss::make_series({1.0, 2.0})); });
  report([&] { (void)actuarial::models::age_factors(pluss::make_series({1.0, 2.0, 3.0}, {3, 1})); });
}
```

Output

```text
triangle::cumulative: observed cumulative values must be non-decreasing
triangle::cumulative: values must be 2D
chain_ladder: at least two development periods are required
```

## See Also

- [`ladder_cdf`](ladder_cdf.md): the pattern to ultimate
- [`operators::chain_ladder`](../reserving/chain_ladder.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bf_reserve.html">
    <span class="label">Previous</span>
    <span class="title">bf_reserve</span>
  </a>
  <a class="next-link" href="ladder_cdf.html">
    <span class="label">Next</span>
    <span class="title">ladder_cdf</span>
  </a>
</div>
<!-- apiref:nav:end -->
