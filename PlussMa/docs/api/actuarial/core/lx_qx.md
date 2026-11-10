<!-- generated from lx_qx.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::lx_qx`

Expected lives of a cohort from death probabilities.

## Signature

```cpp
pluss::Series actuarial::lx_qx(const pluss::Series &qx, double radix = 100000.0);
```

## What It Does

Returns {math}`l_0=\text{radix}` and {math}`l_{k+1}=l_k(1-q_{x+k})`: the survival curve scaled to a starting cohort, `n + 1` values. It is the same function as [`life_contingencies::lx_qx`](../life_contingencies/lx_qx.md), reachable without the namespace, and [`Mortality::life_table`](../mortality/Mortality/life_table.md) as a free function. The radix only scales the table.

## Parameters

`qx`
: One-dimensional, finite series of one-year death probabilities in {math}`[0,1]`; the horizon is its length.

`radix`
: Starting number of lives, finite and greater than 0. Default 100000.

## Returns

A series of `qx.numel() + 1` lives, non-increasing from `radix`.

## Errors

Throws `lx_qx: radix must be finite`, `lx_qx: radix must be > 0` and `lx_qx: qx …` errors for invalid probabilities.

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
  out.writeln(actuarial::lx_qx(qx).items<double>());
}
```

Output

```text
[100000.00, 99000.00, 97812.00, 96344.82, 94514.27, 92245.93]
```

### Example 2: a cohort of 1,000

The table scales with the radix.

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
  out.writeln(actuarial::lx_qx(qx, 1000.0).items<double>());
}
```

Output

```text
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
```

### Example 3: the same as the life-table function

The two namespaces give identical tables.

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
  out.writeln(actuarial::lx_qx(qx, 1000.0).items<double>());
  out.writeln(actuarial::life_contingencies::lx_qx(qx, 1000.0).items<double>());
}
```

Output

```text
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
```

### Example 4: against the survival curve

The table is the curve times the radix.

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
  out.writeln(actuarial::survival_curve(qx).mul(1000.0).items<double>());
}
```

Output

```text
[1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
```

### Example 5: the errors

A radix that is not positive or finite, and an invalid probability.

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
  report([&] { (void)actuarial::lx_qx(pluss::make_series({0.1}), 0.0); });
  report([&] { (void)actuarial::lx_qx(pluss::make_series({0.1}), std::nan("")); });
  report([&] { (void)actuarial::lx_qx(pluss::make_series({1.5})); });
}
```

Output

```text
lx_qx: radix must be > 0
lx_qx: radix must be finite
lx_qx: qx values must lie in [0, 1]
```

## See Also

- [`life_contingencies::lx_qx`](../life_contingencies/lx_qx.md): the same function
- [`commutation_table`](commutation_table.md): the table with its discounted columns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="endowment_insurance.html">
    <span class="label">Previous</span>
    <span class="title">endowment_insurance</span>
  </a>
  <a class="next-link" href="commutation_table.html">
    <span class="label">Next</span>
    <span class="title">commutation_table</span>
  </a>
</div>
<!-- apiref:nav:end -->
