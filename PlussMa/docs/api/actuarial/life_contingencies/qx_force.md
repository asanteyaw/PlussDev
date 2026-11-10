<!-- generated from qx_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::qx_force`

Converts forces of mortality to death probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::qx_force(const pluss::Series &force);
```

## What It Does

Returns {math}`q=1-e^{-\mu}` elementwise. Any non-negative force gives a probability in {math}`[0,1)`: the larger the force, the closer to one but never reaching it. It is the inverse of [`force_qx`](force_qx.md); [`from_force`](../mortality/from_force.md) wraps it into a law.

## Parameters

`force`
: Series of finite, non-negative forces.

## Returns

A series of probabilities of the same length as `force`.

## Errors

Throws `qx_force: force must be >= 0` for a negative force and `qx_force: force must be finite` for NaN or infinity.

## Practical Examples

### Example 1: probabilities

Three forces and the probabilities they produce.

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
  out.writeln(life::qx_force(pluss::make_series({0.01, 0.02, 0.03})).items<double>());
}
```

Output

```text
[0.009950, 0.019801, 0.029554]
```

### Example 2: large forces approach one

A force of 5 leaves almost no survivors but never reaches certainty.

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
  out.writeln(life::qx_force(pluss::make_series({0.0, 1.0, 5.0, 20.0})).items<double>());
}
```

Output

```text
[0.0000000000, 0.6321205588, 0.9932620530, 0.9999999979]
```

### Example 3: the round trip

`force_qx(qx_force(mu))` is the original.

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
  out.writeln(life::force_qx(life::qx_force(pluss::make_series({0.01, 0.5, 2.0}))).items<double>());
}
```

Output

```text
[0.0100000000, 0.5000000000, 2.0000000000]
```

### Example 4: the errors

A negative force and a NaN are refused.

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
  report([&] { (void)life::qx_force(pluss::make_series({0.01, -0.5})); });
  report([&] { (void)life::qx_force(pluss::make_series({0.01, std::nan("")})); });
}
```

Output

```text
qx_force: force must be >= 0
qx_force: force must be finite
```

## See Also

- [`force_qx`](force_qx.md): the inverse- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="px_qx.html">
    <span class="label">Previous</span>
    <span class="title">px_qx</span>
  </a>
  <a class="next-link" href="qx_lx.html">
    <span class="label">Next</span>
    <span class="title">qx_lx</span>
  </a>
</div>
<!-- apiref:nav:end -->
