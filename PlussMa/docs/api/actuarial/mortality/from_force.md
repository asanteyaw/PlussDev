<!-- generated from from_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::mortality::from_force`

Builds a mortality law from forces of mortality.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::from_force(const pluss::Series &force);
```

## What It Does

Takes the force {math}`\mu_{x+k}\ge0`, the constant instantaneous death rate over year `k + 1`, and stores {math}`q_{x+k}=1-e^{-\mu_{x+k}}`. Use it when mortality comes from a hazard model, whose forces add over time: the survival to the end is {math}`\exp(-\sum\mu)`. The inverse is [`Mortality::force`](Mortality/force.md).

## Parameters

`force`
: One-dimensional series of non-negative forces.

## Returns

A defined `Mortality` with horizon `force.numel()`.

## Errors

Throws `qx_force: force must be >= 0` for a negative force.

## Practical Examples

### Example 1: forces to probabilities

A force of 0.02 is a death probability of 0.0198.

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
  const actuarial::Mortality m = actuarial::mortality::from_force(pluss::make_series({0.01, 0.02, 0.03}));
  out.writeln("qx            =", m.qx().items<double>());
  out.writeln("1 - exp(-0.02) =", 1.0 - std::exp(-0.02));
}
```

Output

```text
qx            = [0.00995017, 0.01980133, 0.02955447]
1 - exp(-0.02) = 0.01980133
```

### Example 2: forces add

Survival to the end is the exponential of minus the total force.

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
  const actuarial::Mortality m = actuarial::mortality::from_force(pluss::make_series({0.01, 0.02, 0.03}));
  out.writeln("survival to 3   =", m.survival().items<double>().back());
  out.writeln("exp(-0.06)      =", std::exp(-0.06));
}
```

Output

```text
survival to 3   = 0.94176453
exp(-0.06)      = 0.94176453
```

### Example 3: the round trip

`Mortality::force` returns the forces that were passed in.

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
  const pluss::Series mu = pluss::make_series({0.01, 0.02, 0.03});
  out.writeln(actuarial::mortality::from_force(mu).force().items<double>());
}
```

Output

```text
[0.01000000, 0.02000000, 0.03000000]
```

### Example 4: zero force

No hazard means no deaths.

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
  out.writeln(actuarial::mortality::from_force(pluss::make_series({0.0, 0.0})).survival().items<double>());
}
```

Output

```text
[1.000000, 1.000000, 1.000000]
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
  report([&] { (void)actuarial::mortality::from_force(pluss::make_series({0.01, -0.02})); });
}
```

Output

```text
qx_force: force must be >= 0
```

## See Also

- [`Mortality`](Mortality.md): the value returned and its accessors
- [`Basis`](../valuation/Basis.md): pairs the law with a discount

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_px.html">
    <span class="label">Previous</span>
    <span class="title">from_px</span>
  </a>
  <a class="next-link" href="from_lx.html">
    <span class="label">Next</span>
    <span class="title">from_lx</span>
  </a>
</div>
<!-- apiref:nav:end -->
