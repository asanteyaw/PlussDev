<!-- generated from MortalityModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::MortalityModel`

A calibrated parametric law waiting for ages: a callable that turns attained ages into a `Mortality`.

## Signature

```cpp
using actuarial::MortalityModel = actuarial::Operator<actuarial::Mortality(const pluss::Series &)>;
actuarial::Mortality actuarial::MortalityModel::operator()(const pluss::Series &attained_ages) const;
```

## What It Represents

An [`Operator`](../foundation/Operator.md) whose call takes a one-dimensional series of attained ages and returns the [`Mortality`](Mortality.md) of a fixed parametric law. The scalar-parameter overloads of [`gompertz`](gompertz.md) and [`makeham`](makeham.md) return one: the parameters are fixed once, and the ages vary per call. The `key` is the name of the law, `"gompertz"` or `"makeham"`. Keep the model when the same calibration is applied to several cohorts or starting ages; call the direct overloads for a single age vector.

## Methods

An operator is called like a function and has the four members of [`Operator`](../foundation/Operator.md): [`defined`](../foundation/Operator/defined.md), [`key`](../foundation/Operator/key.md), [`operator bool`](../foundation/Operator/operator_bool.md) and [`operator()`](../foundation/Operator/operator_call.md), which does the work described here.

## Practical Examples

### Example 1: one calibration, several cohorts

One set of Gompertz parameters applied to three different starting ages.

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
  const actuarial::MortalityModel model = actuarial::mortality::gompertz(0.0001, 1.08);
  for (const double start : {40.0, 60.0, 80.0}) {
    const actuarial::Mortality m = model(pluss::make_series({start, start + 1.0, start + 2.0}));
    out.writeln("from", start, " qx =", m.qx().items<double>());
  }
}
```

Output

```text
from 40.000000  qx = [0.002170, 0.002343, 0.002531]
from 60.000000  qx = [0.010075, 0.010876, 0.011741]
from 80.000000  qx = [0.046099, 0.049694, 0.053561]
```

### Example 2: the key names the law

The key of a model is the name of its law.

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
  out.writeln(actuarial::mortality::gompertz(0.0001, 1.08).key(), actuarial::mortality::makeham(0.0005, 0.0001, 1.08).key());
}
```

Output

```text
gompertz makeham
```

### Example 3: pricing with a model

Value a five-year annuity-due at three starting ages under one calibrated law.

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
  const actuarial::MortalityModel model = actuarial::mortality::makeham(0.0005, 0.0001, 1.08);
  for (const double start : {55.0, 65.0, 75.0}) {
    const actuarial::Mortality m = model(pluss::make_series({start, start + 1, start + 2, start + 3, start + 4}));
    const actuarial::Valuation value = actuarial::operators::value({m, actuarial::discount::flat(0.04)});
    out.writeln("age", start, " annuity =", value(actuarial::cashflow::annuity(5)).item<double>());
  }
}
```

Output

```text
age 55.000000  annuity = 4.559982
age 65.000000  annuity = 4.485939
age 75.000000  annuity = 4.332524
```

### Example 4: an undefined model

A default-constructed model is not defined and throws when called.

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
  out.writeln("defined:", actuarial::MortalityModel{}.defined());
  report([&] { (void)actuarial::MortalityModel{}(pluss::make_series({60.0})); });
}
```

Output

```text
defined: false
actuarial Operator is undefined
```

## Changelog

- **0.1**: initial operator-first actuarial surface.

## See Also

[`Operator`](../foundation/Operator.md) is the common callable contract, and [`gompertz`](gompertz.md) and [`makeham`](makeham.md) create models.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Mortality.html">
    <span class="label">Previous</span>
    <span class="title">Mortality</span>
  </a>
  <a class="next-link" href="from_qx.html">
    <span class="label">Next</span>
    <span class="title">from_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
