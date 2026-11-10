<!-- generated from factors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 80b24b64c2b4 -->

# `actuarial::Discount::factors`

[Back to `actuarial::Discount`](../Discount.md)

The discount factors for times 0 to a horizon.

## Signature

```cpp
pluss::Series actuarial::Discount::factors(std::int64_t horizon, pluss::Platform platform = pluss::pCPU) const;
```

## What It Does

Returns the factors {math}`v_0,v_1,\dots,v_h` for a horizon of `h` years: `h + 1` values, the first always the factor for time zero. A flat rate generates them as {math}`(1+i)^{-t}`, so a rate of zero gives all ones and a negative rate gives factors above one (an accumulation). A curve returns its first `h + 1` stored factors unchanged and refuses a horizon longer than it covers.

`operators::value` calls this itself with the horizon of the cashflow. Call it directly to inspect the factors a valuation will use, or to build a cashflow's present value by hand.

## Parameters

`std::int64_t horizon`
: The number of years; must be at least 0. A horizon of 0 returns the single factor 1.

`pluss::Platform platform`
: Where the result lives; the default is the CPU.

## Returns

A one-dimensional `Series` of `horizon + 1` factors.

## Errors

Throws `Discount is undefined` for a default-constructed discount, `Discount::factors: horizon must be >= 0` for a negative horizon, and `Discount::factors: curve must contain at least horizon + 1 values` when the curve is too short.

## Practical Examples

### Example 1: a flat rate

Five years at 4%: the factors fall geometrically.

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
  out.writeln(actuarial::discount::flat(0.04).factors(5).items<double>());
}
```

Output

```text
[1.000000, 0.961538, 0.924556, 0.888996, 0.854804, 0.821927]
```

### Example 2: zero and negative rates

A rate of zero gives no discounting and a negative rate gives factors above one.

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
  out.writeln("rate 0.0   =", actuarial::discount::flat(0.0).factors(3).items<double>());
  out.writeln("rate -0.5  =", actuarial::discount::flat(-0.5).factors(2).items<double>());
}
```

Output

```text
rate 0.0   = [1.000000, 1.000000, 1.000000, 1.000000]
rate -0.5  = [1.000000, 2.000000, 4.000000]
```

### Example 3: a curve returns its own factors

The curve is read as stored, and a shorter horizon takes a prefix.

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
  const actuarial::Discount curve = actuarial::discount::curve(pluss::make_series({1.0, 0.97, 0.93, 0.88}));
  out.writeln("horizon 3 =", curve.factors(3).items<double>());
  out.writeln("horizon 2 =", curve.factors(2).items<double>());
  out.writeln("horizon 0 =", curve.factors(0).items<double>());
}
```

Output

```text
horizon 3 = [1.000000, 0.970000, 0.930000, 0.880000]
horizon 2 = [1.000000, 0.970000, 0.930000]
horizon 0 = [1.000000]
```

### Example 4: present value by hand

The present value of 100 paid at the end of years 1 to 3 is the sum of 100 times the factors.

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
  const pluss::Series factors = actuarial::discount::flat(0.04).factors(3);
  const auto f = factors.items<double>();
  out.writeln("PV =", 100.0 * (f[1] + f[2] + f[3]));
}
```

Output

```text
PV = 277.509103
```

### Example 5: the errors

A negative horizon, a horizon beyond the curve and an undefined discount are refused.

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
  report([&] { (void)actuarial::discount::flat(0.04).factors(-1); });
  report([&] { (void)actuarial::discount::curve(pluss::make_series({1.0, 0.96})).factors(5); });
  report([&] { (void)actuarial::Discount{}.factors(3); });
}
```

Output

```text
Discount::factors: horizon must be >= 0
Discount::factors: curve must contain at least horizon + 1 values
Discount is undefined
```

## See Also

- [`Discount`](../Discount.md): the type
- [`discount::flat`](../flat.md), [`discount::curve`](../curve.md): the factories
- [`Valuation`](../Valuation.md): uses the factors

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
</div>
<!-- apiref:nav:end -->
