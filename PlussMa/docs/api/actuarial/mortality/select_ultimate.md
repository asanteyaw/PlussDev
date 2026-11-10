<!-- generated from select_ultimate.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::mortality::select_ultimate`

Joins a select period of mortality to an ultimate table.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::select_ultimate(const pluss::Series &select_qx, const pluss::Series &ultimate_qx, std::int64_t projection_years);
```

## What It Does

Builds a law of `projection_years` probabilities: the `select_qx` first, then `ultimate_qx` from its first value. It models a population whose first years after selection (underwriting) are lighter than the table that follows. If `projection_years` is shorter than the select vector the law is truncated; an ultimate vector of a single value is repeated for every remaining year, and a longer one must cover them all. Use it to splice a select table and an ultimate table without writing the loop.

## Parameters

`select_qx`
: Probabilities for the select period.

`ultimate_qx`
: Probabilities that follow the select period.

`projection_years`
: Horizon of the result, 0 or more.

## Returns

A defined `Mortality` with horizon `projection_years`.

## Errors

Throws `select_qx: projection_years must be >= 0` for a negative horizon, `select_qx: ultimate_qx length must be >= required tail horizon or equal to 1` when a multi-value ultimate vector is too short, and `survival_curve: qx values must lie in [0, 1]` for probabilities outside the unit interval.

## Practical Examples

### Example 1: a select period of two years

Two select probabilities, then the ultimate table, over five years.

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
}
```

Output

```text
[0.005000, 0.008000, 0.012000, 0.015000, 0.020000]
```

### Example 2: a short horizon truncates

A horizon inside the select period uses only select rates.

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
  const pluss::Series sel = pluss::make_series({0.005, 0.008});
  const pluss::Series ult = pluss::make_series({0.012, 0.015, 0.02});
  out.writeln("2 years:", actuarial::mortality::select_ultimate(sel, ult, 2).qx().items<double>());
  out.writeln("1 year: ", actuarial::mortality::select_ultimate(sel, ult, 1).qx().items<double>());
  out.writeln("0 years:", actuarial::mortality::select_ultimate(sel, ult, 0).horizon());
}
```

Output

```text
2 years: [0.005000, 0.008000]
1 year:  [0.005000]
0 years: 0
```

### Example 3: a single ultimate rate repeats

One ultimate probability is used for every year after the select period.

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
  out.writeln(actuarial::mortality::select_ultimate(pluss::make_series({0.005}), pluss::make_series({0.010}), 4).qx().items<double>());
}
```

Output

```text
[0.005000, 0.010000, 0.010000, 0.010000]
```

### Example 4: effect on survival

A lighter select period raises survival against the ultimate table alone.

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
  const pluss::Series ult = pluss::make_series({0.012, 0.015, 0.02, 0.025, 0.03});
  const actuarial::Mortality ultimate = actuarial::mortality::from_qx(ult);
  const actuarial::Mortality spliced = actuarial::mortality::select_ultimate(pluss::make_series({0.005, 0.008}), ult, 5);
  out.writeln("ultimate only:", ultimate.survival().items<double>().back());
  out.writeln("with select:  ", spliced.survival().items<double>().back());
}
```

Output

```text
ultimate only: 0.901977
with select:   0.941356
```

### Example 5: the errors

A negative horizon, a too-short ultimate vector and an invalid probability are refused.

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
  report([&] { (void)actuarial::mortality::select_ultimate(pluss::make_series({0.005}), pluss::make_series({0.01}), -1); });
  report([&] { (void)actuarial::mortality::select_ultimate(pluss::make_series({0.005}), pluss::make_series({0.01, 0.02}), 4); });
  report([&] { (void)actuarial::mortality::select_ultimate(pluss::make_series({1.5}), pluss::make_series({0.01}), 3); });
}
```

Output

```text
select_qx: projection_years must be >= 0
select_qx: ultimate_qx length must be >= required tail horizon or equal to 1
survival_curve: qx values must lie in [0, 1]
```

## See Also

- [`Mortality`](Mortality.md): the value returned and its accessors
- [`Basis`](../valuation/Basis.md): pairs the law with a discount

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_lx.html">
    <span class="label">Previous</span>
    <span class="title">from_lx</span>
  </a>
  <a class="next-link" href="gompertz.html">
    <span class="label">Next</span>
    <span class="title">gompertz</span>
  </a>
</div>
<!-- apiref:nav:end -->
