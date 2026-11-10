<!-- generated from qx_px.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::qx_px`

Converts survival probabilities to death probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::qx_px(const pluss::Series &px);
```

## What It Does

Returns {math}`q_{x+k}=1-p_{x+k}` elementwise. It is the inverse of [`px_qx`](px_qx.md) and is what you apply to a table of survival probabilities before building a [`Mortality`](../mortality/Mortality.md) by hand; [`from_px`](../mortality/from_px.md) does both steps.

## Parameters

`px`
: Series of finite probabilities in {math}`[0,1]`.

## Returns

A series of the same length as `px`.

## Errors

Throws `qx_px: px values must lie in [0,1]` for an out-of-range value and `qx_px: px must be finite` for a NaN or infinity.

## Practical Examples

### Example 1: death probabilities

Survival probabilities in, death probabilities out.

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
  const pluss::Series px = pluss::make_series({0.990, 0.988, 0.985});
  out.writeln(life::qx_px(px).items<double>());
}
```

Output

```text
[0.010000, 0.012000, 0.015000]
```

### Example 2: the round trip

`qx_px(px_qx(qx))` is the original.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::qx_px(life::px_qx(qx)).items<double>());
}
```

Output

```text
[0.0100000000, 0.0120000000, 0.0150000000, 0.0190000000, 0.0240000000]
```

### Example 3: a table of survival rates

The death probability implied by a five-year table, and the chance of dying in the first three years.

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
  const pluss::Series px = pluss::make_series({0.990, 0.988, 0.985, 0.981, 0.976});
  const auto q = life::qx_px(px).items<double>();
  out.writeln("qx =", q);
  out.writeln("probability of death within 3 years =", 1.0 - px.items<double>()[0] * px.items<double>()[1] * px.items<double>()[2]);
}
```

Output

```text
qx = [0.010000, 0.012000, 0.015000, 0.019000, 0.024000]
probability of death within 3 years = 0.036552
```

### Example 4: the errors

A survival probability above one and a NaN are refused.

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
  report([&] { (void)life::qx_px(pluss::make_series({0.99, 1.1})); });
  report([&] { (void)life::qx_px(pluss::make_series({0.99, std::nan("")})); });
}
```

Output

```text
qx_px: px values must lie in [0,1]
qx_px: px must be finite
```

## See Also

- [`px_qx`](px_qx.md): the inverse
- [`from_px`](../mortality/from_px.md): builds a `Mortality`- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="qx_lx.html">
    <span class="label">Previous</span>
    <span class="title">qx_lx</span>
  </a>
  <a class="next-link" href="select_qx.html">
    <span class="label">Next</span>
    <span class="title">select_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
