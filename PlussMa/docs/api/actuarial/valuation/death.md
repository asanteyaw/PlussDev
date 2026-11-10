<!-- generated from death.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::cashflow::death`

A level benefit paid at the end of the year of death within a term.

## Signature

```cpp
actuarial::Cashflow actuarial::cashflow::death(std::int64_t years, double benefit = 1.0);
```

## What It Does

Creates a term insurance: `benefit` is paid at the end of the year in which the life dies, if that is within `years`; nothing is paid on survival. Its expected present value is {math}`A^1_{x:\overline n\vert}=\sum_{k=0}^{n-1}v^{k+1}\,{}_kp_x\,q_{x+k}`. The cashflow has `years` death payments of `benefit` and zero survival payments.

A zero-year term is valid and empty. A negative benefit is accepted.

## Parameters

`std::int64_t years`
: The length of the term in years; at least 0.

`double benefit`
: The death benefit, finite; default 1.

## Returns

A defined `Cashflow` with `years` death payments.

## Errors

Throws `cashflow::death: years must be >= 0` for a negative term and a message naming `benefit` for a non-finite benefit.

## Practical Examples

### Example 1: the schedules

A benefit of 10 for three years: death payments and no survival payments.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  const actuarial::Cashflow c = actuarial::cashflow::death(3, 10.0);
  out.writeln("survival", c.survival_payments().items<double>(), " death", c.death_payments().items<double>());
}
```

Output

```text
survival [0.0, 0.0, 0.0, 0.0]  death [10.0, 10.0, 10.0]
```

### Example 2: the value

The expected present value of a unit benefit over five years, and the same by the sum.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  const auto p = actuarial::survival_curve(qx).items<double>();
  const auto q = qx.items<double>();
  double by_hand = 0.0;
  for (int k = 0; k < 5; ++k) by_hand += std::pow(1.0 / 1.04, k + 1) * p[k] * q[k];
  out.writeln("value =", value(actuarial::cashflow::death(5)).item<double>(), " by the sum =", by_hand);
}
```

Output

```text
value = 0.067934  by the sum = 0.067934
```

### Example 3: the benefit scales the value

A benefit of 250,000 is worth 250,000 times the unit.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  out.writeln("unit =", value(actuarial::cashflow::death(5)).item<double>(), " 250000 =", value(actuarial::cashflow::death(5, 250000.0)).item<double>());
}
```

Output

```text
unit = 0.067934  250000 = 16983.510558
```

### Example 4: the length of the term

A longer term costs more, because more deaths are covered.

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
  const actuarial::Basis basis{mortality, actuarial::discount::flat(0.04)};
  const actuarial::Valuation value = actuarial::operators::value(basis);
  for (const int years : {1, 3, 5}) out.writeln("term", years, " value", value(actuarial::cashflow::death(years)).item<double>());
}
```

Output

```text
term 1  value 0.009615
term 3  value 0.033642
term 5  value 0.067934
```

### Example 5: the errors

A negative term and a non-finite benefit are refused; a zero term is accepted.

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
  report([&] { (void)actuarial::cashflow::death(-1); });
  report([&] { (void)actuarial::cashflow::death(3, std::nan("")); });
  out.writeln("death(0) horizon", actuarial::cashflow::death(0).horizon());
}
```

Output

```text
cashflow::death: years must be >= 0
cashflow::death: benefit must be finite
death(0) horizon 0
```

## See Also

[`survival`](survival.md) pays on survival, [`endowment`](endowment.md) pays on both, and [`make`](make.md) builds any benefit.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="annuity.html">
    <span class="label">Previous</span>
    <span class="title">annuity</span>
  </a>
  <a class="next-link" href="survival.html">
    <span class="label">Next</span>
    <span class="title">survival</span>
  </a>
</div>
<!-- apiref:nav:end -->
