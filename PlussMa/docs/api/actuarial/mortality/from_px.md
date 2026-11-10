<!-- generated from from_px.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::mortality::from_px`

Builds a mortality law from one-year survival probabilities.

## Signature

```cpp
actuarial::Mortality actuarial::mortality::from_px(const pluss::Series &px);
```

## What It Does

Takes {math}`p_{x+k}=1-q_{x+k}` and stores the complementary death probabilities. Use it when the table lists the probability of surviving each year, as in many published tables. The result is the same law as [`from_qx`](from_qx.md) of the complement, and `px()` returns the values given, to rounding.

## Parameters

`px`
: One-dimensional series of probabilities in {math}`[0,1]`.

## Returns

A defined `Mortality` with horizon `px.numel()`.

## Errors

Throws `qx_px: px values must lie in [0,1]` for a probability outside the unit interval.

## Practical Examples

### Example 1: survival probabilities

The same law as the `from_qx` page, entered as survival probabilities.

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
  const actuarial::Mortality m = actuarial::mortality::from_px(pluss::make_series({0.99, 0.988, 0.985}));
  out.writeln("qx       =", m.qx().items<double>());
  out.writeln("survival =", m.survival().items<double>());
}
```

Output

```text
qx       = [0.010000, 0.012000, 0.015000]
survival = [1.000000, 0.990000, 0.978120, 0.963448]
```

### Example 2: equivalence with qx

Passing `px` is the same as passing `1 - px` to `from_qx`.

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
  const actuarial::Mortality a = actuarial::mortality::from_px(pluss::make_series({0.99, 0.988, 0.985}));
  const actuarial::Mortality b = actuarial::mortality::from_qx(pluss::make_series({0.01, 0.012, 0.015}));
  out.writeln(a.survival().items<double>());
  out.writeln(b.survival().items<double>());
}
```

Output

```text
[1.0000000000, 0.9900000000, 0.9781200000, 0.9634482000]
[1.0000000000, 0.9900000000, 0.9781200000, 0.9634482000]
```

### Example 3: certain survival

A law with all `px` equal to one never loses a life.

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
  const actuarial::Mortality m = actuarial::mortality::from_px(pluss::make_series({1.0, 1.0, 1.0}));
  out.writeln("survival =", m.survival().items<double>());
}
```

Output

```text
survival = [1.000000, 1.000000, 1.000000, 1.000000]
```

### Example 4: the error

A survival probability above one or below zero is refused.

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
  report([&] { (void)actuarial::mortality::from_px(pluss::make_series({0.99, 1.2})); });
  report([&] { (void)actuarial::mortality::from_px(pluss::make_series({-0.1})); });
}
```

Output

```text
qx_px: px values must lie in [0,1]
qx_px: px values must lie in [0,1]
```

## See Also

- [`Mortality`](Mortality.md): the value returned and its accessors
- [`Basis`](../valuation/Basis.md): pairs the law with a discount

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_qx.html">
    <span class="label">Previous</span>
    <span class="title">from_qx</span>
  </a>
  <a class="next-link" href="from_force.html">
    <span class="label">Next</span>
    <span class="title">from_force</span>
  </a>
</div>
<!-- apiref:nav:end -->
