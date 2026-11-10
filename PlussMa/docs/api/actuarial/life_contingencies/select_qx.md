<!-- generated from select_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::select_qx`

Joins select and ultimate death probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::select_qx(const pluss::Series &select_rates, const pluss::Series &ultimate_qx, std::int64_t projection_years);
```

## What It Does

A select table holds mortality for the first years after underwriting, an ultimate table the mortality afterwards. The result has `projection_years` values: the first `min(len(select_rates), projection_years)` come from `select_rates`, and the rest from `ultimate_qx` **in sequence from its first value**. A single ultimate probability is repeated for every remaining year; a longer ultimate vector must cover them all. The `ultimate_qx` argument is not read when the select rates already fill the horizon. It is the free-function form of [`mortality::select_ultimate`](../mortality/select_ultimate.md).

## Parameters

`select_rates`
: Probabilities for the select period.

`ultimate_qx`
: Probabilities that follow; one value is broadcast.

`projection_years`
: Length of the result, 0 or more.

## Returns

A series of `projection_years` probabilities.

## Errors

Throws `select_qx: projection_years must be >= 0`, `select_qx: ultimate_qx length must be >= required tail horizon or equal to 1` and `select_qx: ultimate_qx must be non-empty when projection exceeds select horizon`; invalid probabilities throw the `survival_curve: …` errors.

## Practical Examples

### Example 1: select then ultimate

Two select years, then the ultimate table.

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
  out.writeln(life::select_qx(pluss::make_series({0.005, 0.008}), pluss::make_series({0.012, 0.015, 0.02}), 5).items<double>());
}
```

Output

```text
[0.005000, 0.008000, 0.012000, 0.015000, 0.020000]
```

### Example 2: a single ultimate rate

One value is repeated.

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
  out.writeln(life::select_qx(pluss::make_series({0.005}), pluss::make_series({0.01}), 4).items<double>());
}
```

Output

```text
[0.005000, 0.010000, 0.010000, 0.010000]
```

### Example 3: a short horizon

Inside the select period only select rates are used.

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
  out.writeln(life::select_qx(sel, ult, 1).items<double>());
  out.writeln(life::select_qx(sel, ult, 2).items<double>());
  out.writeln(life::select_qx(sel, ult, 0).numel());
}
```

Output

```text
[0.005000]
[0.005000, 0.008000]
0
```

### Example 4: the ultimate table is not read when not needed

The horizon sits inside the select period, so an empty ultimate vector is fine.

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
  out.writeln(life::select_qx(pluss::make_series({0.005, 0.008}), pluss::series::zeros({0}), 2).items<double>());
}
```

Output

```text
[0.005000, 0.008000]
```

### Example 5: the errors

A negative horizon, a short ultimate vector and an empty one.

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
  report([&] { (void)life::select_qx(pluss::make_series({0.005}), pluss::make_series({0.01}), -1); });
  report([&] { (void)life::select_qx(pluss::make_series({0.005}), pluss::make_series({0.01, 0.02}), 4); });
  report([&] { (void)life::select_qx(pluss::make_series({0.005}), pluss::series::zeros({0}), 3); });
}
```

Output

```text
select_qx: projection_years must be >= 0
select_qx: ultimate_qx length must be >= required tail horizon or equal to 1
select_qx: ultimate_qx must be non-empty when projection exceeds select horizon
```

## See Also

- [`mortality::select_ultimate`](../mortality/select_ultimate.md): builds a `Mortality`- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qx_px.html">
    <span class="label">Previous</span>
    <span class="title">qx_px</span>
  </a>
  <a class="next-link" href="survival_force.html">
    <span class="label">Next</span>
    <span class="title">survival_force</span>
  </a>
</div>
<!-- apiref:nav:end -->
