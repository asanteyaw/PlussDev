<!-- generated from stoploss.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::stoploss`

The expected excess over a deductible, estimated from a sample.

## Signature

```cpp
actuarial::losses::pluss::Series stoploss(const pluss::Series &losses, double deductible);
```

## What It Does

Returns the mean of {math}`(S-d)_+`: the **stop-loss premium** at deductible `d`, the expected amount by which the sample values exceed `d`. Applied to a sample of aggregate (annual) losses it prices a stop-loss reinsurance cover that pays everything above `d`; applied to claim amounts it is the expected excess per claim. It is [`excess_loss`](excess_loss.md) followed by the mean, and it falls as the deductible rises. An empty sample gives zero.

## Parameters

`losses`
: One-dimensional, finite series of amounts, 0 or more.

`deductible`
: The retention, finite and 0 or more.

## Returns

A rank-zero series.

## Errors

Throws `operators::deductible: amount must be >= 0` and `operators::deductible: amount must be finite`, and the `deductible: losses must be …` errors.

## Practical Examples

### Example 1: a stop-loss premium

Retention of 100 on six values.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  out.writeln(actuarial::losses::stoploss(x, 100.0).item<double>());
}
```

Output

```text
975.000000
```

### Example 2: falling with the deductible

Raising the retention lowers the premium, and a retention of zero is the plain mean.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  for (const double d : {0.0, 100.0, 500.0, 2000.0, 5000.0})
    out.writeln("d =", d, " stop-loss =", actuarial::losses::stoploss(x, d).item<double>());
  out.writeln("mean =", x.mean().item<double>());
}
```

Output

```text
d = 0.000000  stop-loss = 1071.666667
d = 100.000000  stop-loss = 975.000000
d = 500.000000  stop-loss = 733.333333
d = 2000.000000  stop-loss = 333.333333
d = 5000.000000  stop-loss = 0.000000
mean = 1071.666667
```

### Example 3: the mean of the excess

The definition.

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
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  out.writeln(actuarial::losses::stoploss(x, 100.0).item<double>(), actuarial::losses::excess_loss(x, 100.0).mean().item<double>());
}
```

Output

```text
975.000000 975.000000
```

### Example 4: annual losses

Stop-loss premium on simulated-style annual totals of a portfolio.

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
  const pluss::Series annual = pluss::make_series({820.0, 1400.0, 960.0, 2300.0, 1100.0, 1750.0, 3100.0, 1250.0});
  out.writeln("expected annual loss =", annual.mean().item<double>());
  out.writeln("stop-loss at 2000    =", actuarial::losses::stoploss(annual, 2000.0).item<double>());
}
```

Output

```text
expected annual loss = 1585.000000
stop-loss at 2000    = 175.000000
```

### Example 5: the errors

A negative deductible and a negative amount.

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
  report([&] { (void)actuarial::losses::stoploss(pluss::make_series({100.0}), -1.0); });
  report([&] { (void)actuarial::losses::stoploss(pluss::make_series({-100.0}), 10.0); });
}
```

Output

```text
operators::deductible: amount must be >= 0
deductible: losses must be >= 0
```

## See Also

- [`excess_loss`](excess_loss.md): the per-loss excess
- [`layer_premium`](layer_premium.md): with a limit

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="layer_premium.html">
    <span class="label">Previous</span>
    <span class="title">layer_premium</span>
  </a>
  <a class="next-link" href="lognormal_lev.html">
    <span class="label">Next</span>
    <span class="title">lognormal_lev</span>
  </a>
</div>
<!-- apiref:nav:end -->
