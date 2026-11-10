<!-- generated from excess_loss.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::excess_loss`

The part of each loss above an attachment.

## Signature

```cpp
actuarial::losses::pluss::Series excess_loss(const pluss::Series &losses, double attachment);
```

## What It Does

Returns {math}`(X-a)_+=\max(X-a,0)` for each loss: what an excess-of-loss cover with attachment `a` and no limit pays. It is the [`operators::deductible`](../loss/deductible.md) transform applied once, without building the operator. Use it to turn a sample of ground-up losses into the losses to a cover; [`loss_layer`](loss_layer.md) adds a limit and [`stoploss`](stoploss.md) takes the mean.

## Parameters

`losses`
: One-dimensional, defined, finite series of loss amounts, 0 or more.

`attachment`
: The amount where the cover starts, finite and 0 or more.

## Returns

A series of the shape of `losses`: zero where a loss is at or below the attachment.

## Errors

Throws `operators::deductible: amount must be >= 0` and `operators::deductible: amount must be finite` for the attachment, and `deductible: losses must be defined`, `must be 1D`, `must be finite` or `must be >= 0` for the losses.

## Practical Examples

### Example 1: the excess over 100

Small losses cost the cover nothing.

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
  out.writeln("loss   =", x.items<double>());
  out.writeln("excess =", actuarial::losses::excess_loss(x, 100.0).items<double>());
}
```

Output

```text
loss   = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
excess = [0.000000, 50.000000, 200.000000, 600.000000, 1100.000000, 3900.000000]
```

### Example 2: the same as the deductible operator

Identical output.

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
  out.writeln(actuarial::losses::excess_loss(x, 100.0).items<double>());
  out.writeln(actuarial::operators::deductible(100.0)(x).items<double>());
}
```

Output

```text
[0.000000, 50.000000, 200.000000, 600.000000, 1100.000000, 3900.000000]
[0.000000, 50.000000, 200.000000, 600.000000, 1100.000000, 3900.000000]
```

### Example 3: a higher attachment

Fewer losses reach the cover and each costs less.

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
  out.print_defaults(PrintOptions{}.fixed(1));
  const pluss::Series x = pluss::make_series({80.0, 150.0, 300.0, 700.0, 1200.0, 4000.0});
  for (const double a : {0.0, 100.0, 500.0, 2000.0}) {
    const pluss::Series e = actuarial::losses::excess_loss(x, a);
    out.writeln("attachment", a, " losses reaching it:", e.gt(0.0).sum().item<double>(), " total:", e.sum().item<double>());
  }
}
```

Output

```text
attachment 0.0  losses reaching it: 6.0  total: 6430.0
attachment 100.0  losses reaching it: 5.0  total: 5850.0
attachment 500.0  losses reaching it: 3.0  total: 4400.0
attachment 2000.0  losses reaching it: 1.0  total: 2000.0
```

### Example 4: zero attachment

The cover pays everything.

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
  out.writeln(actuarial::losses::excess_loss(x, 0.0).items<double>());
}
```

Output

```text
[80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

### Example 5: the errors

A negative attachment and a negative loss are refused.

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
  report([&] { (void)actuarial::losses::excess_loss(pluss::make_series({100.0, 200.0}), -1.0); });
  report([&] { (void)actuarial::losses::excess_loss(pluss::make_series({-1.0, 200.0}), 100.0); });
  report([&] { (void)actuarial::losses::excess_loss(pluss::make_series({100.0, 200.0}), std::nan("")); });
}
```

Output

```text
operators::deductible: amount must be >= 0
deductible: losses must be >= 0
operators::deductible: amount must be finite
```

## See Also

- [`loss_layer`](loss_layer.md): with a limit
- [`stoploss`](stoploss.md): its mean
- [`operators::deductible`](../loss/deductible.md): the operator form

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="negbin_pmf.html">
    <span class="label">Previous</span>
    <span class="title">negbin_pmf</span>
  </a>
  <a class="next-link" href="loss_layer.html">
    <span class="label">Next</span>
    <span class="title">loss_layer</span>
  </a>
</div>
<!-- apiref:nav:end -->
