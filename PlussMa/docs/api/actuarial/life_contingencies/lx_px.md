<!-- generated from lx_px.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::lx_px`

Builds a life table from survival probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::lx_px(const pluss::Series &px, double radix = 100000.0);
```

## What It Does

Returns {math}`l_0=\text{radix}` and {math}`l_{k+1}=l_k\,p_k`: the same table as [`lx_qx`](lx_qx.md) when the input is `1 − q`. Use it when the source table lists survival probabilities. The result has one more value than `px`.

## Parameters

`px`
: One-dimensional series of probabilities in {math}`[0,1]`.

`radix`
: Starting number of lives, greater than 0. Default 100000.

## Returns

A series of `px.numel() + 1` lives, non-increasing from `radix`.

## Errors

Throws `lx_px: radix must be > 0` and `lx_px: px values must lie in [0,1]`.

## Practical Examples

### Example 1: a table

Five years of survival probabilities.

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
  out.print_defaults(PrintOptions{}.fixed(2));
  out.writeln(life::lx_px(pluss::make_series({0.990, 0.988, 0.985, 0.981, 0.976})).items<double>());
}
```

Output

```text
[100000.00, 99000.00, 97812.00, 96344.82, 94514.27, 92245.93]
```

### Example 2: the same as lx_qx

Built from the complements, the tables agree.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::lx_px(life::px_qx(qx), 1000.0).items<double>());
  out.writeln(life::lx_qx(qx, 1000.0).items<double>());
}
```

Output

```text
[1000.0000, 990.0000, 978.1200, 963.4482, 945.1427, 922.4593]
[1000.0000, 990.0000, 978.1200, 963.4482, 945.1427, 922.4593]
```

### Example 3: a small cohort

A cohort of 10 lives shows the fractional expected survivors.

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
  out.writeln(life::lx_px(pluss::make_series({0.9, 0.8, 0.7}), 10.0).items<double>());
}
```

Output

```text
[10.000000, 9.000000, 7.200000, 5.040000]
```

### Example 4: the errors

A bad radix and an out-of-range probability.

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
  report([&] { (void)life::lx_px(pluss::make_series({0.9}), -1.0); });
  report([&] { (void)life::lx_px(pluss::make_series({1.5}), 10.0); });
}
```

Output

```text
lx_px: radix must be > 0
lx_px: px values must lie in [0,1]
```

## See Also

- [`lx_qx`](lx_qx.md): from death probabilities
- [`px_lx`](px_lx.md): the inverse- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lifetime_pmf.html">
    <span class="label">Previous</span>
    <span class="title">lifetime_pmf</span>
  </a>
  <a class="next-link" href="lx_qx.html">
    <span class="label">Next</span>
    <span class="title">lx_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
