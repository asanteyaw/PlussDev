<!-- generated from force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: affc59f9eaa9 -->

# `actuarial::Mortality::force`

[Back to `actuarial::Mortality`](../Mortality.md)

The forces of mortality of each year.

## Signature

```cpp
pluss::Series actuarial::Mortality::force() const;
```

## What It Does

Returns {math}`\mu_{x+k}=-\ln(1-q_{x+k})`: the constant instantaneous rate of death over year `k + 1` that gives the same one-year probability. It is the transformation of `qx` that adds: the force of two years is the sum of the forces, so the cumulative sums of `force` give the cumulative hazard. It requires every `qx` below one, since a certain death has an infinite force.

## Returns

A one-dimensional `Series` of `horizon` forces.

## Errors

Throws `Mortality is undefined` for a default-constructed law, and `force_qx: qx must lie in [0,1)` when a death probability is one.

## Practical Examples

### Example 1: forces and probabilities

For small probabilities the force is close to `qx`; it is always larger.

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
  out.writeln("qx    =", mortality.qx().items<double>());
  out.writeln("force =", mortality.force().items<double>());
}
```

Output

```text
qx    = [0.010000, 0.012000, 0.015000, 0.019000, 0.024000]
force = [0.010050, 0.012073, 0.015114, 0.019183, 0.024293]
```

### Example 2: the force adds

The survival to the end is the exponential of minus the sum of the forces.

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
  out.writeln("exp(-sum of forces) =", std::exp(-mortality.force().sum().item<double>()), " survival to 5 =", mortality.survival().items<double>().back());
}
```

Output

```text
exp(-sum of forces) = 0.922459  survival to 5 = 0.922459
```

### Example 3: a force that grows with age

The Gompertz law has an exponentially increasing force.

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
  const pluss::Series ages = pluss::make_series({60.0, 70.0, 80.0, 90.0});
  out.writeln("force of the law =", actuarial::mortality::gompertz(ages, 0.0001, 1.08).force().items<double>());
  out.writeln("b * c^age        =", 0.0001 * std::pow(1.08, 60.0), 0.0001 * std::pow(1.08, 70.0), 0.0001 * std::pow(1.08, 80.0), 0.0001 * std::pow(1.08, 90.0));
}
```

Output

```text
force of the law = [0.010126, 0.021861, 0.047195, 0.101892]
b * c^age        = 0.010126 0.021861 0.047195 0.101892
```

### Example 4: the errors

A certain death in some year has no finite force, and an undefined law is refused.

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
  const actuarial::Mortality certain = actuarial::mortality::from_qx(pluss::make_series({0.5, 1.0}));
  out.writeln("survival is fine:", certain.survival().items<double>());
  report([&] { (void)certain.force(); });
  report([&] { (void)actuarial::Mortality{}.force(); });
}
```

Output

```text
survival is fine: [1.000000, 0.500000, 0.000000]
force_qx: qx must lie in [0,1)
Mortality is undefined
```

## See Also

- [`Mortality`](../Mortality.md): the type

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="horizon.html">
    <span class="label">Next</span>
    <span class="title">horizon</span>
  </a>
</div>
<!-- apiref:nav:end -->
