<!-- generated from from_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::mortality::from_qx`

Builds a mortality law from one-year death probabilities.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::from_qx(const pluss::Series &qx);
```

## What It Does

Takes {math}`q_{x+k}`, the probability of dying in year `k + 1` given alive at its start, for `k = 0, ..., n−1`, and stores them as they are: this is the native form of a [`Mortality`](Mortality.md), so no conversion error enters. Call it when you hold a table of `qx`; use [`from_px`](from_px.md), [`from_force`](from_force.md) or [`from_lx`](from_lx.md) when the table is in another form. The horizon is the length of the vector, and an empty vector is a valid law with horizon 0. A probability of exactly one is accepted (everyone dies in that year); only its `force` is undefined.

## Parameters

`qx`
: One-dimensional, finite series of probabilities in {math}`[0,1]`.

## Returns

A defined `Mortality` with horizon `qx.numel()`.

## Errors

Throws `survival_curve: qx must be 1D` for a matrix, `survival_curve: qx must be finite` for a NaN or infinity, and `survival_curve: qx values must lie in [0, 1]` for a probability outside the unit interval.

## Practical Examples

### Example 1: survival from probabilities

Five years of mortality and the probability of surviving 0 to 5 years.

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
  out.writeln("horizon  =", mortality.horizon());
  out.writeln("survival =", mortality.survival().items<double>());
}
```

Output

```text
horizon  = 5
survival = [1.000000, 0.990000, 0.978120, 0.963448, 0.945143, 0.922459]
```

### Example 2: the probabilities are stored unchanged

`qx()` returns exactly what was passed in.

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
  out.writeln(actuarial::mortality::from_qx(qx).qx().items<double>());
}
```

Output

```text
[0.010000, 0.012000, 0.015000, 0.019000, 0.024000]
```

### Example 3: an empty law

No years is a defined law with a survival curve of the single value 1.

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
  const actuarial::Mortality empty = actuarial::mortality::from_qx(pluss::series::zeros({0}));
  out.writeln("horizon:", empty.horizon(), " survival:", empty.survival().items<double>());
}
```

Output

```text
horizon: 0  survival: [1.000000]
```

### Example 4: certain death

A probability of one is a valid law: no one survives that year, though the force is not finite.

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
  const actuarial::Mortality m = actuarial::mortality::from_qx(pluss::make_series({0.5, 1.0, 0.0}));
  out.writeln("survival =", m.survival().items<double>());
  out.writeln("pmf      =", m.lifetime_pmf().items<double>());
}
```

Output

```text
survival = [1.000000, 0.500000, 0.000000, 0.000000]
pmf      = [0.500000, 0.500000, 0.000000, 0.000000]
```

### Example 5: the errors

Values outside [0,1], non-finite values and matrices are refused.

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
  report([&] { (void)actuarial::mortality::from_qx(pluss::make_series({0.01, -0.02})); });
  report([&] { (void)actuarial::mortality::from_qx(pluss::make_series({0.01, 1.5})); });
  report([&] { (void)actuarial::mortality::from_qx(pluss::make_series({0.01, std::nan("")})); });
  report([&] { (void)actuarial::mortality::from_qx(pluss::make_series({0.1, 0.2, 0.3, 0.4}).reshape({2, 2})); });
}
```

Output

```text
survival_curve: qx values must lie in [0, 1]
survival_curve: qx values must lie in [0, 1]
survival_curve: qx must be finite
survival_curve: qx must be 1D
```

## See Also

- [`Mortality`](Mortality.md): the value returned and its accessors
- [`Basis`](../valuation/Basis.md): pairs the law with a discount

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="MortalityModel.html">
    <span class="label">Previous</span>
    <span class="title">MortalityModel</span>
  </a>
  <a class="next-link" href="from_px.html">
    <span class="label">Next</span>
    <span class="title">from_px</span>
  </a>
</div>
<!-- apiref:nav:end -->
