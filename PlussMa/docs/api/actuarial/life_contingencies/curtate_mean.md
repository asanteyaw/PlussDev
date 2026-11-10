<!-- generated from curtate_mean.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::curtate_mean`

Expected complete years lived within the horizon.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::curtate_mean(const pluss::Series &qx);
```

## What It Does

Returns {math}`e=\sum_{k=1}^{n}{}_kp_x`: the expected number of complete years lived, **limited to the horizon** the vector describes. It is a temporary (truncated) expectation: lives that survive the horizon count only the `n` years observed, so the value is below the whole-life figure and approaches it as the horizon grows. The result is a rank-zero series, read with `item<double>()`. [`curtate_curve`](curtate_curve.md) gives the same quantity at every duration.

## Parameters

`qx`
: One-dimensional series of probabilities in {math}`[0,1]`.

## Returns

A rank-zero series.

## Errors

Throws `survival_curve: qx must be 1D`, `survival_curve: qx must be finite` and `survival_curve: qx values must lie in [0, 1]`.

## Practical Examples

### Example 1: a short horizon

Five years of mortality: slightly under five years are lived.

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
  out.writeln(life::curtate_mean(qx).item<double>());
}
```

Output

```text
4.799170
```

### Example 2: the sum of survival probabilities

The definition: the survival curve summed from one year on.

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
  const auto s = actuarial::mortality::from_qx(qx).survival().items<double>();
  out.writeln(s[1] + s[2] + s[3] + s[4] + s[5], life::curtate_mean(qx).item<double>());
}
```

Output

```text
4.799170 4.799170
```

### Example 3: longer horizons

Extending the horizon with the Makeham law raises the expectation by less than a year for each added year, since fewer lives remain.

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
  for (const int n : {5, 10, 20, 40}) {
    std::vector<double> ages;
    for (int a = 60; a < 60 + n; ++a) ages.push_back(a);
    out.writeln("horizon", n, " =", life::curtate_mean(life::makeham_qx(pluss::make_series(ages), 0.0002, 0.0001, 1.08)).item<double>());
  }
}
```

Output

```text
horizon 5  = 4.831595
horizon 10  = 9.309160
horizon 20  = 16.720156
horizon 40  = 22.981389
```

### Example 4: no mortality

Without deaths every year is lived.

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
  out.writeln(life::curtate_mean(pluss::make_series({0.0, 0.0, 0.0})).item<double>());
}
```

Output

```text
3.000000
```

### Example 5: the errors

An invalid probability.

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
  report([&] { (void)life::curtate_mean(pluss::make_series({0.1, 2.0})); });
}
```

Output

```text
survival_curve: qx values must lie in [0, 1]
```

## See Also

- [`curtate_curve`](curtate_curve.md): at every duration- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="curtate_curve.html">
    <span class="label">Previous</span>
    <span class="title">curtate_curve</span>
  </a>
  <a class="next-link" href="dx_lx.html">
    <span class="label">Next</span>
    <span class="title">dx_lx</span>
  </a>
</div>
<!-- apiref:nav:end -->
