<!-- generated from makeham.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::mortality::makeham`

Builds a mortality law from a Makeham force.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::makeham(const pluss::Series &attained_ages, double a, double b, double c);
actuarial::MortalityModel actuarial::mortality::makeham(double a, double b, double c);
```

## What It Does

The Makeham force is {math}`\mu_x=a+b\,c^{x}`: the [Gompertz](gompertz.md) force plus a constant `a` for deaths that do not depend on age, such as accidents. Everything else is the same: {math}`q_x=1-\exp(-\mu_x)`, and the second overload returns a [`MortalityModel`](MortalityModel.md) bound to the parameters. With `a = 0` the law is Gompertz. Use it when the young ages matter and a pure exponential understates their mortality.

## Parameters

`attained_ages`
: One-dimensional, finite, non-negative ages.

`a`
: The age-independent force, 0 or more.

`b`
: Level of the age-dependent force at age 0, 0 or more.

`c`
: Yearly growth factor, greater than 1.

## Returns

A `Mortality` of one probability per age, or a `MortalityModel` that produces one when called with ages.

## Errors

Throws `makeham_force: a must be >= 0` and the same range errors for `b`, `c` and the ages as `gompertz`; an undefined `MortalityModel` throws `actuarial Operator is undefined`.

## Practical Examples

### Example 1: a law at four ages

Makeham mortality from age 60.

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
  const actuarial::Mortality m = actuarial::mortality::makeham(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0005, 0.0001, 1.08);
  out.writeln("qx    =", m.qx().items<double>());
  out.writeln("force =", m.force().items<double>());
}
```

Output

```text
qx    = [0.010569, 0.011371, 0.012235, 0.013168]
force = [0.010626, 0.011436, 0.012311, 0.013255]
```

### Example 2: with a = 0 it is Gompertz

The two laws coincide when the constant term is zero.

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
  out.writeln(actuarial::mortality::makeham(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0, 0.0001, 1.08).qx().items<double>());
  out.writeln(actuarial::mortality::gompertz(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0001, 1.08).qx().items<double>());
}
```

Output

```text
[0.010075, 0.010876, 0.011741, 0.012674]
[0.010075, 0.010876, 0.011741, 0.012674]
```

### Example 3: the constant term

The difference in force from Gompertz is the constant `a` at every age.

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
  const auto mk = actuarial::mortality::makeham(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0005, 0.0001, 1.08).force().items<double>();
  const auto gz = actuarial::mortality::gompertz(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0001, 1.08).force().items<double>();
  out.writeln("makeham - gompertz =", mk[0] - gz[0], mk[1] - gz[1], mk[2] - gz[2], mk[3] - gz[3]);
}
```

Output

```text
makeham - gompertz = 0.000500 0.000500 0.000500 0.000500
```

### Example 4: a reusable model

Bind three parameters and apply the model to any ages.

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
  out.writeln("key:", model.key());
  out.writeln(model(pluss::make_series({60.0, 61.0, 62.0, 63.0})).qx().items<double>());
}
```

Output

```text
key: makeham
[0.010569, 0.011371, 0.012235, 0.013168]
```

### Example 5: the errors

A negative constant and an undefined model are refused.

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
  report([&] { (void)actuarial::mortality::makeham(pluss::make_series({60.0, 61.0, 62.0, 63.0}), -0.0005, 0.0001, 1.08); });
  report([&] { (void)actuarial::mortality::makeham(pluss::make_series({60.0, 61.0, 62.0, 63.0}), 0.0005, 0.0001, 1.0); });
  report([&] { (void)actuarial::MortalityModel{}(pluss::make_series({60.0, 61.0, 62.0, 63.0})); });
}
```

Output

```text
makeham_force: a must be >= 0
makeham_force: c must be > 1 for Gompertz-style age acceleration
actuarial Operator is undefined
```

## See Also

- [`Mortality`](Mortality.md): the value returned and its accessors
- [`Basis`](../valuation/Basis.md): pairs the law with a discount
- [`gompertz`](gompertz.md): the age-only special case

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gompertz.html">
    <span class="label">Previous</span>
    <span class="title">gompertz</span>
  </a>
</div>
<!-- apiref:nav:end -->
