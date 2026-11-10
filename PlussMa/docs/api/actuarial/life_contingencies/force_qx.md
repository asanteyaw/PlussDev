<!-- generated from force_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::force_qx`

Converts death probabilities to forces of mortality.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::force_qx(const pluss::Series &qx);
```

## What It Does

Returns {math}`\mu_{x+k}=-\ln(1-q_{x+k})`: the constant instantaneous death rate over a year that produces the one-year probability `q`. A force is a rate, not a probability, so it can exceed one, and it is always at least `q`. Forces add over time, which makes them the convenient scale for combining hazards; [`qx_force`](qx_force.md) is the inverse. A death probability of exactly one has no finite force and is refused.

## Parameters

`qx`
: One-dimensional series of probabilities in {math}`[0,1)`.

## Returns

A series of the same length as `qx`.

## Errors

Throws `force_qx: qx must lie in [0,1)` when a value is one or more, `survival_curve: qx values must lie in [0, 1]` for a negative value and `survival_curve: qx must be finite` for NaN.

## Practical Examples

### Example 1: forces

Death probabilities and the forces that produce them.

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
  out.writeln("qx    =", qx.items<double>());
  out.writeln("force =", life::force_qx(qx).items<double>());
}
```

Output

```text
qx    = [0.010000, 0.012000, 0.015000, 0.019000, 0.024000]
force = [0.010050, 0.012073, 0.015114, 0.019183, 0.024293]
```

### Example 2: force is at least q

For small probabilities they are close; for large ones the force is much larger.

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
  out.writeln(life::force_qx(pluss::make_series({0.001, 0.1, 0.5, 0.9, 0.99})).items<double>());
}
```

Output

```text
[0.001001, 0.105361, 0.693147, 2.302585, 4.605170]
```

### Example 3: forces add

The survival over three years is the exponential of minus the sum of three forces.

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
  const pluss::Series mu = life::force_qx(qx);
  out.writeln("exp(-sum of first 3 forces) =", std::exp(-mu.slice(0, 0, 3, 1).sum().item<double>()));
  out.writeln("product of first 3 px       =", life::px_qx(qx).slice(0, 0, 3, 1).prod().item<double>());
}
```

Output

```text
exp(-sum of first 3 forces) = 0.96344820
product of first 3 px       = 0.96344820
```

### Example 4: the round trip

`qx_force(force_qx(qx))` is the original.

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
  out.print_defaults(PrintOptions{}.fixed(10));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::qx_force(life::force_qx(qx)).items<double>());
}
```

Output

```text
[0.0100000000, 0.0120000000, 0.0150000000, 0.0190000000, 0.0240000000]
```

### Example 5: the errors

A certain death has no finite force.

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
  report([&] { (void)life::force_qx(pluss::make_series({0.1, 1.0})); });
  report([&] { (void)life::force_qx(pluss::make_series({0.1, -0.1})); });
}
```

Output

```text
force_qx: qx must lie in [0,1)
survival_curve: qx values must lie in [0, 1]
```

## See Also

- [`qx_force`](qx_force.md): the inverse
- [`hazard_qx`](hazard_qx.md): the running sum of the forces- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dx_lx.html">
    <span class="label">Previous</span>
    <span class="title">dx_lx</span>
  </a>
  <a class="next-link" href="gompertz_force.html">
    <span class="label">Next</span>
    <span class="title">gompertz_force</span>
  </a>
</div>
<!-- apiref:nav:end -->
