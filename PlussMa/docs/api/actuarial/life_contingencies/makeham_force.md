<!-- generated from makeham_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::makeham_force`

Evaluates the Makeham force of mortality at ages.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::makeham_force(const pluss::Series &attained_ages, double a, double b, double c);
```

## What It Does

Returns {math}`\mu_x=a+b\,c^{x}`: the [Gompertz](gompertz_force.md) force plus a constant `a` for deaths that do not depend on age. With `a = 0` it is Gompertz. The result has the shape, dtype and platform of `attained_ages`. [`makeham_qx`](makeham_qx.md) gives the one-year probability.

## Parameters

`attained_ages`
: One-dimensional, finite, non-negative ages.

`a`
: Age-independent force, 0 or more.

`b`
: Force of the age-dependent part at age 0, 0 or more.

`c`
: Yearly growth factor, greater than 1.

## Returns

A series of forces, one per age.

## Errors

Throws `makeham_force: a must be >= 0` and the `gompertz_force` errors for `b`, `c` and the ages.

## Practical Examples

### Example 1: forces at four ages

Makeham forces from age 60.

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
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0});
  out.writeln(life::makeham_force(ages, 0.0005, 0.0001, 1.08).items<double>());
}
```

Output

```text
[0.0106257, 0.0114358, 0.0123106, 0.0132555]
```

### Example 2: the constant term

The difference from Gompertz is `a` at every age.

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
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0});
  const auto m = life::makeham_force(ages, 0.0005, 0.0001, 1.08).items<double>();
  const auto g = life::gompertz_force(ages, 0.0001, 1.08).items<double>();
  out.writeln(m[0] - g[0], m[1] - g[1], m[2] - g[2], m[3] - g[3]);
}
```

Output

```text
0.0005000 0.0005000 0.0005000 0.0005000
```

### Example 3: a = 0 is Gompertz

The two coincide.

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
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0});
  out.writeln(life::makeham_force(ages, 0.0, 0.0001, 1.08).items<double>());
  out.writeln(life::gompertz_force(ages, 0.0001, 1.08).items<double>());
}
```

Output

```text
[0.0101257, 0.0109358, 0.0118106, 0.0127555]
[0.0101257, 0.0109358, 0.0118106, 0.0127555]
```

### Example 4: young ages

The constant dominates at young ages, where the exponential part is tiny.

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
  out.print_defaults(PrintOptions{}.fixed(7));
  const pluss::Series ages = pluss::make_series({20.0, 40.0, 60.0, 80.0});
  out.writeln(life::makeham_force(ages, 0.0005, 0.00005, 1.09).items<double>());
}
```

Output

```text
[0.0007802, 0.0020705, 0.0093016, 0.0498276]
```

### Example 5: the errors

A negative constant and a bad growth factor.

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
  report([&] { (void)life::makeham_force(pluss::make_series({60.0}), -0.1, 0.0001, 1.08); });
  report([&] { (void)life::makeham_force(pluss::make_series({60.0}), 0.0005, 0.0001, 1.0); });
}
```

Output

```text
makeham_force: a must be >= 0
makeham_force: c must be > 1 for Gompertz-style age acceleration
```

## See Also

- [`makeham_qx`](makeham_qx.md): as a probability
- [`mortality::makeham`](../mortality/makeham.md): builds a `Mortality`- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lx_qx.html">
    <span class="label">Previous</span>
    <span class="title">lx_qx</span>
  </a>
  <a class="next-link" href="makeham_qx.html">
    <span class="label">Next</span>
    <span class="title">makeham_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
