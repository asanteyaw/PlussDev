<!-- generated from survival_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::survival_force`

Builds the survival curve from forces of mortality.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::survival_force(const pluss::Series &force);
```

## What It Does

Returns {math}`[1,\,e^{-\mu_0},\,e^{-(\mu_0+\mu_1)},\ldots]`: **one more value** than `force`, the leading 1 being survival at duration zero. Because forces add, this is the cumulative sum of the forces exponentiated. Use it when mortality is a hazard; for death probabilities the equivalent is the survival curve of [`Mortality::survival`](../mortality/Mortality/survival.md).

## Parameters

`force`
: Series of finite, non-negative forces.

## Returns

A series of `force.numel() + 1` probabilities, non-increasing from 1.

## Errors

Throws `survival_force: force must be >= 0` for a negative force.

## Practical Examples

### Example 1: the curve

Three years of force.

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
  out.writeln(life::survival_force(pluss::make_series({0.1, 0.2, 0.3})).items<double>());
}
```

Output

```text
[1.000000, 0.904837, 0.740818, 0.548812]
```

### Example 2: one value more than the forces

Five forces give six survival points.

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
  out.writeln(life::survival_force(life::force_qx(pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024}))).numel());
}
```

Output

```text
6
```

### Example 3: agreement with the probabilities

The same survival whether it is built from probabilities or forces.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::survival_force(life::force_qx(qx)).items<double>());
  out.writeln(actuarial::mortality::from_qx(qx).survival().items<double>());
}
```

Output

```text
[1.00000000, 0.99000000, 0.97812000, 0.96344820, 0.94514268, 0.92245926]
[1.00000000, 0.99000000, 0.97812000, 0.96344820, 0.94514268, 0.92245926]
```

### Example 4: constant force

A constant force gives geometric survival.

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
  out.writeln(life::survival_force(pluss::make_series({0.1, 0.1, 0.1, 0.1})).items<double>());
  out.writeln(std::exp(-0.1), std::exp(-0.2), std::exp(-0.3), std::exp(-0.4));
}
```

Output

```text
[1.000000, 0.904837, 0.818731, 0.740818, 0.670320]
0.904837 0.818731 0.740818 0.670320
```

### Example 5: the error

A negative force is refused.

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
  report([&] { (void)life::survival_force(pluss::make_series({0.1, -0.2})); });
}
```

Output

```text
survival_force: force must be >= 0
```

## See Also

- [`hazard_qx`](hazard_qx.md): the cumulative force from probabilities- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select_qx.html">
    <span class="label">Previous</span>
    <span class="title">select_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
