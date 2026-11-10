<!-- generated from lx_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::life_contingencies::lx_qx`

Builds a life table from death probabilities.

## Signature

```cpp
pluss::Series actuarial::life_contingencies::lx_qx(const pluss::Series &qx, double radix = 100000.0);
```

## What It Does

Returns {math}`l_0=\text{radix}` and {math}`l_{k+1}=l_k(1-q_k)`: the expected number of lives alive at each anniversary from a starting cohort of `radix`. The result has one more value than `qx`. The radix only scales the table, and 100,000 is conventional. This is [`Mortality::life_table`](../mortality/Mortality/life_table.md) as a free function; [`qx_lx`](qx_lx.md) is the inverse and [`dx_lx`](dx_lx.md) gives the deaths.

## Parameters

`qx`
: One-dimensional series of probabilities in {math}`[0,1]`.

`radix`
: Starting number of lives, greater than 0. Default 100000.

## Returns

A series of `qx.numel() + 1` lives, non-increasing from `radix`.

## Errors

Throws `lx_qx: radix must be > 0` and the `survival_curve: …` errors of the probabilities.

## Practical Examples

### Example 1: the default cohort

A cohort of 100,000 lives.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::lx_qx(qx).items<double>());
}
```

Output

```text
[100000.00, 99000.00, 97812.00, 96344.82, 94514.27, 92245.93]
```

### Example 2: a different radix

A cohort of 1,000 gives the proportional table.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::lx_qx(qx, 1000.0).items<double>());
}
```

Output

```text
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
```

### Example 3: deaths

The differences of `lx` are the deaths; `dx_lx` computes them.

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
  out.print_defaults(PrintOptions{}.fixed(3));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::dx_lx(life::lx_qx(qx, 1000.0)).items<double>());
}
```

Output

```text
[10.000, 11.880, 14.672, 18.306, 22.683]
```

### Example 4: the round trip

`qx_lx` recovers the probabilities.

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  out.writeln(life::qx_lx(life::lx_qx(qx)).items<double>());
}
```

Output

```text
[0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
```

### Example 5: the errors

A radix that is not positive and an invalid probability.

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
  report([&] { (void)life::lx_qx(pluss::make_series({0.1}), 0.0); });
  report([&] { (void)life::lx_qx(pluss::make_series({0.1}), -5.0); });
  report([&] { (void)life::lx_qx(pluss::make_series({0.1, 1.5})); });
}
```

Output

```text
lx_qx: radix must be > 0
lx_qx: radix must be > 0
survival_curve: qx values must lie in [0, 1]
```

## See Also

- [`qx_lx`](qx_lx.md): the inverse
- [`lx_px`](lx_px.md): from survival probabilities- [`Mortality`](../mortality/Mortality.md): the same conversions as accessors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lx_px.html">
    <span class="label">Previous</span>
    <span class="title">lx_px</span>
  </a>
  <a class="next-link" href="makeham_force.html">
    <span class="label">Next</span>
    <span class="title">makeham_force</span>
  </a>
</div>
<!-- apiref:nav:end -->
