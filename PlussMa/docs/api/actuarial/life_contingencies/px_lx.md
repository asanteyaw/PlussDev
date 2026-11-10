<!-- generated from px_lx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::px_lx`

Recovers survival probabilities from the lives of a table.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::px_lx(const pluss::Series &lx);
```

## What It Does

Returns {math}`p_k=l_{k+1}/l_k`: one fewer value than `lx`. Only the ratios matter, so a table of any size works. The first value must be positive, later values non-negative and the table non-increasing; a table that rises is refused rather than turned into a probability above one. A table may end at zero, but if it reaches zero earlier the later ratios are undefined and the call is refused.

## Parameters

`lx`
: One-dimensional series of lives, non-increasing, with a positive first value.

## Returns

A series of `lx.numel() − 1` probabilities.

## Errors

Throws `px_lx: lx[0] must be > 0`, `px_lx: lx must be non-increasing`, `px_lx: lx must be >= 0` and `px_lx: lx[:-1] must be > 0 to derive deterministic ratios`.

## Practical Examples

### Example 1: survival from a table

Lives at five ages.

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
  out.writeln(life::px_lx(pluss::make_series({100000.0, 99000.0, 97812.0, 96347.0, 94516.0})).items<double>());
}
```

Output

```text
[0.990000, 0.988000, 0.985022, 0.980996]
```

### Example 2: the scale does not matter

Dividing the table by 100 gives the same ratios.

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
  out.writeln(life::px_lx(pluss::make_series({1000.0, 990.0, 978.12})).items<double>());
  out.writeln(life::px_lx(pluss::make_series({100000.0, 99000.0, 97812.0})).items<double>());
}
```

Output

```text
[0.990000, 0.988000]
[0.990000, 0.988000]
```

### Example 3: a table that ends at zero

The last year has survival probability zero.

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
  out.writeln(life::px_lx(pluss::make_series({100.0, 50.0, 0.0})).items<double>());
}
```

Output

```text
[0.500000, 0.000000]
```

### Example 4: the round trip

`lx_px` rebuilds the table from the ratios.

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
  const pluss::Series lx = pluss::make_series({1000.0, 990.0, 978.0});
  out.writeln(life::lx_px(life::px_lx(lx), 1000.0).items<double>());
}
```

Output

```text
[1000.000000, 990.000000, 978.000000]
```

### Example 5: the errors

A rising table, a zero start and a table that dies out early.

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
  report([&] { (void)life::px_lx(pluss::make_series({100.0, 120.0})); });
  report([&] { (void)life::px_lx(pluss::make_series({0.0, 0.0})); });
  report([&] { (void)life::px_lx(pluss::make_series({100.0, 0.0, 0.0})); });
}
```

Output

```text
px_lx: lx must be non-increasing
px_lx: lx[0] must be > 0
px_lx: lx[:-1] must be > 0 to derive deterministic ratios
```

## See Also

- [`qx_lx`](qx_lx.md): the death probabilities
- [`lx_px`](lx_px.md): the inverse- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="makeham_qx.html">
    <span class="label">Previous</span>
    <span class="title">makeham_qx</span>
  </a>
  <a class="next-link" href="px_qx.html">
    <span class="label">Next</span>
    <span class="title">px_qx</span>
  </a>
</div>
<!-- apiref:nav:end -->
