<!-- generated from gompertz_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::gompertz_qx`

One-year death probabilities from the Gompertz law.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::gompertz_qx(const pluss::Series &attained_ages, double b, double c);
```

## What It Does

Returns {math}`q_x=1-\exp(-\mu_x)` with {math}`\mu_x=b\,c^{x}` the [`gompertz_force`](gompertz_force.md): the probability of dying within a year at each attained age. It equals `qx_force(gompertz_force(...))`. [`mortality::gompertz`](../mortality/gompertz.md) wraps the same numbers into a `Mortality`.

## Parameters

`attained_ages`
: One-dimensional, finite, non-negative ages.

`b`
: Force at age 0, 0 or more.

`c`
: Yearly growth factor, greater than 1.

## Returns

A series of probabilities, one per age.

## Errors

Throws the same errors as [`gompertz_force`](gompertz_force.md).

## Practical Examples

### Example 1: probabilities at four ages

Probabilities rise with age.

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
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0});
  out.writeln(life::gompertz_qx(ages, 0.0001, 1.08).items<double>());
}
```

Output

```text
[0.010075, 0.010876, 0.011741, 0.012674]
```

### Example 2: through the force

The same as converting the force.

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
  const pluss::Series ages = pluss::make_series({60.0, 61.0, 62.0, 63.0});
  out.writeln(life::qx_force(life::gompertz_force(ages, 0.0001, 1.08)).items<double>());
  out.writeln(life::gompertz_qx(ages, 0.0001, 1.08).items<double>());
}
```

Output

```text
[0.01007461, 0.01087618, 0.01174115, 0.01267447]
[0.01007461, 0.01087618, 0.01174115, 0.01267447]
```

### Example 3: the curve over a lifetime

Probabilities from age 40 to 100.

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
  const pluss::Series ages = pluss::make_series({40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0});
  out.writeln(life::gompertz_qx(ages, 0.0001, 1.08).items<double>());
}
```

Output

```text
[0.002170, 0.004679, 0.010075, 0.021623, 0.046099, 0.096872, 0.197462]
```

### Example 4: a life table from the law

Feed the probabilities to `lx_qx`.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  std::vector<double> a;
  for (int x = 60; x < 65; ++x) a.push_back(x);
  out.writeln(life::lx_qx(life::gompertz_qx(pluss::make_series(a), 0.0001, 1.08), 1000.0).items<double>());
}
```

Output

```text
[1000.000, 989.925, 979.159, 967.662, 955.398, 942.326]
```

### Example 5: the errors

Parameters out of range.

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
  report([&] { (void)life::gompertz_qx(pluss::make_series({60.0}), -1.0, 1.08); });
  report([&] { (void)life::gompertz_qx(pluss::make_series({60.0}), 0.0001, 0.9); });
}
```

Output

```text
gompertz_force: b must be >= 0
gompertz_force: c must be > 1 for Gompertz-style age acceleration
```

## See Also

- [`gompertz_force`](gompertz_force.md): the force- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gompertz_force.html">
    <span class="label">Previous</span>
    <span class="title">gompertz_force</span>
  </a>
  <a class="next-link" href="hazard_qx.html">
    <span class="label">Next</span>
    <span class="title">hazard_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
