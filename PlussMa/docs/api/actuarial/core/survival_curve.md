<!-- generated from survival_curve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::survival_curve`

Survival probabilities from death probabilities.

## Signature

```cpp
pluss::Series actuarial::survival_curve(const pluss::Series &qx);
```

## What It Does

Returns {math}`{}_kp_x=\prod_{j<k}(1-q_{x+j})` for `k = 0, ..., n`: `n + 1` values, the first always 1. It is the building block of every contingent value in this family: a payment due at time `k` if the life survives is worth {math}`v^k\,{}_kp_x`. It is [`Mortality::survival`](../mortality/Mortality/survival.md) as a free function; use it when the probabilities are a bare vector and no `Mortality` is needed.

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

## Returns

A series of `qx.numel() + 1` values, non-increasing from 1.

## Errors

Throws `survival_curve: qx must be defined`, `survival_curve: qx must be 1D`, `survival_curve: qx must be finite` and `survival_curve: qx values must lie in [0, 1]`.

## Practical Examples

### Example 1: the survival curve

Five years of mortality.

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
  out.writeln(actuarial::survival_curve(qx).items<double>());
}
```

Output

```text
[1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
```

### Example 2: by hand

The cumulative product of the complements, starting from 1.

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
  out.writeln(1.0, 0.99, 0.99 * 0.988, 0.99 * 0.988 * 0.985);
  out.writeln(actuarial::survival_curve(qx).slice(0, 0, 4, 1).items<double>());
}
```

Output

```text
1.000000 0.990000 0.978120 0.963448
[1.000000, 0.990000, 0.978120, 0.963448]
```

### Example 3: no years

An empty vector gives the single value 1.

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
  out.writeln(actuarial::survival_curve(pluss::series::zeros({0})).items<double>());
}
```

Output

```text
[1.000000]
```

### Example 4: certain death

Once a probability of one occurs, survival stays at zero.

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
  out.writeln(actuarial::survival_curve(pluss::make_series({0.5, 1.0, 0.2})).items<double>());
}
```

Output

```text
[1.000000, 0.500000, 0.000000, 0.000000]
```

### Example 5: the same as a Mortality

The free function and the accessor agree.

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
  out.writeln(actuarial::survival_curve(qx).items<double>());
  out.writeln(actuarial::mortality::from_qx(qx).survival().items<double>());
}
```

Output

```text
[1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
[1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
```

### Example 6: the errors

A matrix, a NaN, an out-of-range value and an undefined series.

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
  report([&] { (void)actuarial::survival_curve(pluss::make_series({0.1, 0.2}).reshape({1, 2})); });
  report([&] { (void)actuarial::survival_curve(pluss::make_series({0.1, std::nan("")})); });
  report([&] { (void)actuarial::survival_curve(pluss::make_series({0.1, 1.5})); });
  report([&] { (void)actuarial::survival_curve(pluss::series::undefined()); });
}
```

Output

```text
survival_curve: qx must be 1D
survival_curve: qx must be finite
survival_curve: qx values must lie in [0, 1]
survival_curve: qx must be defined
```

## See Also

- [`Mortality::survival`](../mortality/Mortality/survival.md): the same curve as a method
- [`lx_qx`](lx_qx.md): the curve scaled to a cohort

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="annuity_due.html">
    <span class="label">Next</span>
    <span class="title">annuity_due</span>
  </a>
</div>
<!-- apiref:nav:end -->
