<!-- generated from life_table.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3a35a0f526e4 -->

# `actuarial::Mortality::life_table`

[Back to `actuarial::Mortality`](../Mortality.md)

The expected lives of a cohort at each age.

## Signature

```cpp
pluss::Series actuarial::Mortality::life_table(double radix = 100000.0) const;
```

## What It Does

Returns {math}`l_k=l_0\,{}_kp_x` for `k = 0, ..., n`: the number of lives expected alive at each age from a starting cohort of `radix` lives. The radix only scales the table; its default of 100,000 is the conventional size. The table is the survival curve times the radix, and [`from_lx`](../from_lx.md) turns it back into a law.

## Returns

A one-dimensional `Series` of `horizon + 1` values, non-increasing from `radix`.

## Errors

Throws `Mortality is undefined` for a default-constructed law, and `lx_qx: radix must be > 0` for a radix that is not positive.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  out.writeln(mortality.life_table().items<double>());
}
```

Output

```text
[100000.00, 99000.00, 97812.00, 96344.82, 94514.27, 92245.93]
```

### Example 2: a different radix

The table scales with the radix; the deaths are `lx` differences.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  const auto lx = mortality.life_table(1000.0).items<double>();
  out.writeln("lives of 1000 =", lx);
  out.writeln("deaths        =", lx[0] - lx[1], lx[1] - lx[2], lx[2] - lx[3], lx[3] - lx[4], lx[4] - lx[5]);
}
```

Output

```text
lives of 1000 = [1000.000, 990.000, 978.120, 963.448, 945.143, 922.459]
deaths        = 10.000 11.880 14.672 18.306 22.683
```

### Example 3: the round trip

`from_lx` recovers the probabilities from the table.

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
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  out.writeln("qx       =", mortality.qx().items<double>());
  out.writeln("from lx  =", actuarial::mortality::from_lx(mortality.life_table()).qx().items<double>());
}
```

Output

```text
qx       = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
from lx  = [0.01000000, 0.01200000, 0.01500000, 0.01900000, 0.02400000]
```

### Example 4: the errors

A radix that is not positive is refused, as is an undefined law.

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
  const pluss::Series qx = pluss::make_series({0.010, 0.012, 0.015, 0.019, 0.024});
  const actuarial::Mortality mortality = actuarial::mortality::from_qx(qx);
  report([&] { (void)mortality.life_table(0.0); });
  report([&] { (void)mortality.life_table(-5.0); });
  report([&] { (void)actuarial::Mortality{}.life_table(); });
}
```

Output

```text
lx_qx: radix must be > 0
lx_qx: radix must be > 0
Mortality is undefined
```

## See Also

- [`Mortality`](../Mortality.md): the type

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="horizon.html">
    <span class="label">Previous</span>
    <span class="title">horizon</span>
  </a>
  <a class="next-link" href="lifetime_pmf.html">
    <span class="label">Next</span>
    <span class="title">lifetime_pmf</span>
  </a>
</div>
<!-- apiref:nav:end -->
