<!-- generated from mortality.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mortality

The mortality family turns the common representations of a mortality law into one validated [`Mortality`](mortality/Mortality.md). Once the law is stored as annual death probabilities, the same object supplies survival, the lifetime distribution, forces and life-table counts, and a valuation does not care how the law was entered.

## What This Family Owns

| You have | Use |
|---|---|
| Death probabilities | [`from_qx`](mortality/from_qx.md) |
| Survival probabilities | [`from_px`](mortality/from_px.md) |
| Forces of mortality | [`from_force`](mortality/from_force.md) |
| The lives of a life table | [`from_lx`](mortality/from_lx.md) |
| A select table and an ultimate table | [`select_ultimate`](mortality/select_ultimate.md) |
| A Gompertz or Makeham law | [`gompertz`](mortality/gompertz.md), [`makeham`](mortality/makeham.md) |
| A calibrated law to apply to many ages | [`MortalityModel`](mortality/MortalityModel.md) |
| The views of a law | [`Mortality`](mortality/Mortality.md) and its methods |

The direct factories take a completed vector and return a `Mortality`. The parametric overloads with only parameters return a reusable `MortalityModel`, applied to any attained-age `Series`.

## Practical Examples

### Example 1: one law, four views

Death probabilities in; survival, force and the life table out.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024}));
  out.writeln("survival =", mortality.survival().items<double>());
  out.writeln("force    =", mortality.force().items<double>());
  out.writeln("lx       =", mortality.life_table(1000.0).items<double>());
}
```

Output

```text
survival = [1.0000, 0.9900, 0.9781, 0.9634, 0.9451, 0.9225]
force    = [0.0101, 0.0121, 0.0151, 0.0192, 0.0243]
lx       = [1000.0000, 990.0000, 978.1200, 963.4482, 945.1427, 922.4593]
```

### Example 2: a parametric law

Gompertz mortality over four ages, directly and through a reusable model.

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
  out.writeln(actuarial::mortality::gompertz(ages, 0.0001, 1.08).qx().items<double>());
  const actuarial::MortalityModel model = actuarial::mortality::gompertz(0.0001, 1.08);
  out.writeln(model(ages).qx().items<double>());
}
```

Output

```text
[0.010075, 0.010876, 0.011741, 0.012674]
[0.010075, 0.010876, 0.011741, 0.012674]
```

### Example 3: a select period

Two years of lighter select mortality, then the ultimate table.

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
  const actuarial::Mortality m = actuarial::mortality::select_ultimate(pluss::make_series({0.005, 0.008}), pluss::make_series({0.012, 0.015, 0.02}), 5);
  out.writeln(m.qx().items<double>());
  out.writeln(m.survival().items<double>().back());
}
```

Output

```text
[0.005000, 0.008000, 0.012000, 0.015000, 0.020000]
0.941356
```

## Related Pages

Use the persistent sidebar to open each type or factory. The {guide}`actuarial guide <actuarial>` develops longer workflows and mathematical background.

```{toctree}
:maxdepth: 1
:hidden:

mortality/Mortality
mortality/MortalityModel
mortality/from_qx
mortality/from_px
mortality/from_force
mortality/from_lx
mortality/select_ultimate
mortality/gompertz
mortality/makeham
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

mortality/Mortality
mortality/MortalityModel
mortality/from_qx
mortality/from_px
mortality/from_force
mortality/from_lx
mortality/select_ultimate
mortality/gompertz
mortality/makeham
```
<!-- apiref:toctree:end -->
