<!-- generated from survival.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ea1601f36d43 -->

# `actuarial::Mortality::survival`

[Back to `actuarial::Mortality`](../Mortality.md)

The probabilities of surviving 0 to the horizon years.

## Signature

```cpp
pluss::Series actuarial::Mortality::survival() const;
```

## What It Does

Returns {math}`{}_kp_x=\prod_{j<k}(1-q_{x+j})` for `k = 0, ..., n`: `n + 1` values, the first always 1. It is the series every valuation weights survival payments with, and its difference is the lifetime distribution.

## Returns

A one-dimensional `Series` of `horizon + 1` probabilities, non-increasing from 1.

## Errors

Throws `Mortality is undefined` for a default-constructed law.

## Practical Examples

### Example 1: the survival curve

Five years of mortality: survival falls from 1.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  out.writeln(mortality.survival().items<double>());
}
```

Output

```text
[1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
```

### Example 2: a parametric law

Survival of a Gompertz law over ten years from age 60.

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
  std::vector<double> ages;
  for (int a = 60; a < 70; ++a) ages.push_back(a);
  out.writeln(actuarial::mortality::gompertz(pluss::make_series(ages), 0.0001, 1.08).survival().items<double>());
}
```

Output

```text
[1.000000, 0.989925, 0.979159, 0.967662, 0.955398, 0.942326, 0.928410, 0.913612, 0.897894, 0.881222, 0.863565]
```

### Example 3: survival and the lifetime distribution

The probability of dying in year `k` is the drop in survival.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const auto s = mortality.survival().items<double>();
  out.writeln("drops in survival =", s[0] - s[1], s[1] - s[2], s[2] - s[3]);
  out.writeln("lifetime pmf      =", mortality.lifetime_pmf().items<double>()[0], mortality.lifetime_pmf().items<double>()[1], mortality.lifetime_pmf().items<double>()[2]);
}
```

Output

```text
drops in survival = 0.010000 0.011880 0.014672
lifetime pmf      = 0.010000 0.011880 0.014672
```

### Example 4: the error

An undefined law is refused.

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
  report([&] { (void)actuarial::Mortality{}.survival(); });
}
```

Output

```text
Mortality is undefined
```

## See Also

- [`Mortality`](../Mortality.md): the type

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qx.html">
    <span class="label">Previous</span>
    <span class="title">qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
