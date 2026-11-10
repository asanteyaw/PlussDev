<!-- generated from bf_reserve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::models::bf_reserve`

Bornhuetter-Ferguson reserve.

## Signature

```cpp
pluss::Series actuarial::models::bf_reserve(const pluss::Series &expected_ultimate_losses, const pluss::Series &percent_reported);
```

## What It Does

Returns {math}`\text{expected ultimate}\times(1-\text{percent reported})`: the part of the a priori ultimate that has not yet been reported, which is the reserve. It does not depend on the reported amounts at all, so it is the same for any diagonal. Add the reported amounts to get [`bf_ultimate`](bf_ultimate.md).

## Parameters

`expected_ultimate_losses`
: A priori ultimate of each origin, finite and 0 or more.

`percent_reported`
: Fraction expected to be reported, the same length, between 0 and 1.

## Returns

A series of reserves, one per origin.

## Errors

Throws `bf_reserve: expected_ultimate_losses and percent_reported must have same length`, `bf_reserve: percent_reported must be <= 1` and the `must be defined/finite/>= 0` errors.

## Practical Examples

### Example 1: four origins

The unreported share of each a priori ultimate.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln(actuarial::models::bf_reserve(expected, pct).items<double>());
}
```

Output

```text
[10.00, 28.00, 75.00, 165.00]
```

### Example 2: independent of the diagonal

The reserve does not use the reported amounts; only the ultimate does.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln("reserve:", actuarial::models::bf_reserve(expected, pct).items<double>()[3]);
  out.writeln("ultimate at reported 130:", actuarial::models::bf_ultimate(reported, expected, pct).items<double>()[3]);
  out.writeln("ultimate at reported 180:", actuarial::models::bf_ultimate(pluss::make_series({240.0, 250.0, 215.0, 180.0}), expected, pct).items<double>()[3]);
}
```

Output

```text
reserve: 165.00
ultimate at reported 130: 295.00
ultimate at reported 180: 345.00
```

### Example 3: the same as the operator

On a triangle, the operator gives the same reserves.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  const actuarial::Triangle t = actuarial::triangle::cumulative(triangle);
  out.writeln(actuarial::operators::bornhuetter_ferguson(expected, pct)(t).reserve.items<double>());
  out.writeln(actuarial::models::bf_reserve(expected, pct).items<double>());
}
```

Output

```text
[10.00, 28.00, 75.00, 165.00]
[10.00, 28.00, 75.00, 165.00]
```

### Example 4: the total

The total reserve of the book.

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
  const pluss::Series reported = pluss::make_series({240.0, 250.0, 215.0, 130.0});
  const pluss::Series expected = pluss::make_series({250.0, 280.0, 300.0, 330.0});
  const pluss::Series pct = pluss::make_series({0.96, 0.90, 0.75, 0.50});
  out.writeln(actuarial::models::bf_reserve(expected, pct).sum().item<double>());
}
```

Output

```text
278.00
```

### Example 5: the errors

A mismatched length and a percentage above 1.

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
  report([&] { (void)actuarial::models::bf_reserve(pluss::make_series({1.0, 2.0}), pluss::make_series({0.5})); });
  report([&] { (void)actuarial::models::bf_reserve(pluss::make_series({1.0}), pluss::make_series({1.5})); });
  report([&] { (void)actuarial::models::bf_reserve(pluss::make_series({-1.0}), pluss::make_series({0.5})); });
}
```

Output

```text
bf_reserve: expected_ultimate_losses and percent_reported must have same length
bf_reserve: percent_reported must be <= 1
bf_reserve: expected_ultimate_losses must be >= 0
```

## See Also

- [`bf_ultimate`](bf_ultimate.md): reported plus reserve

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bf_ultimate.html">
    <span class="label">Previous</span>
    <span class="title">bf_ultimate</span>
  </a>
  <a class="next-link" href="age_factors.html">
    <span class="label">Next</span>
    <span class="title">age_factors</span>
  </a>
</div>
<!-- apiref:nav:end -->
