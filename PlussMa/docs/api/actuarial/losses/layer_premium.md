<!-- generated from layer_premium.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::layer_premium`

The expected loss to a layer, estimated from a sample.

## Signature

```cpp
actuarial::losses::pluss::Series layer_premium(const pluss::Series &losses, double attachment, double limit);
```

## What It Does

Returns the mean of [`loss_layer`](loss_layer.md): the average amount the layer pays per loss in the sample, which is its **burning-cost** pure premium per claim. Multiply by the expected number of claims for the pure premium of the cover. An empty sample gives zero. When the severity is parametric use [`lognormal_layer`](lognormal_layer.md) or [`gamma_layer`](gamma_layer.md), which integrate the distribution instead of averaging a sample.

## Parameters

`losses`
: One-dimensional, finite series of loss amounts, 0 or more.

`attachment`
: Where the layer starts, 0 or more.

`limit`
: The width of the layer, 0 or more.

## Returns

A rank-zero series.

## Errors

Throws the same errors as [`loss_layer`](loss_layer.md).

## Practical Examples

### Example 1: a layer premium

500 excess of 100 on six losses.

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
  out.writeln(actuarial::losses::layer_premium(x, 100.0, 500.0).item<double>());
}
```

Output

```text
291.666667
```

### Example 2: the mean of the layer

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
  out.writeln(actuarial::losses::layer_premium(x, 100.0, 500.0).item<double>(), actuarial::losses::loss_layer(x, 100.0, 500.0).mean().item<double>());
}
```

Output

```text
291.666667 291.666667
```

### Example 3: a ladder of layers

The premiums of adjacent layers add up to the premium of the whole.

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
  const double a = actuarial::losses::layer_premium(x, 0.0, 500.0).item<double>();
  const double b = actuarial::losses::layer_premium(x, 500.0, 1500.0).item<double>();
  const double c = actuarial::losses::layer_premium(x, 2000.0, 1.0e9).item<double>();
  out.writeln(a, b, c, "sum", a + b + c, " mean loss", x.mean().item<double>());
}
```

Output

```text
338.333333 400.000000 333.333333 sum 1071.666667  mean loss 1071.666667
```

### Example 4: a difference of stop-loss premiums

The layer is the stop-loss at the attachment less the stop-loss at the exhaustion point.

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
  out.writeln(actuarial::losses::layer_premium(x, 100.0, 500.0).item<double>(), actuarial::losses::stoploss(x, 100.0).item<double>() - actuarial::losses::stoploss(x, 600.0).item<double>());
}
```

Output

```text
291.666667 291.666667
```

### Example 5: an empty sample

No losses, no premium.

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
  out.writeln(actuarial::losses::layer_premium(pluss::series::zeros({0}), 100.0, 500.0).item<double>());
}
```

Output

```text
0.000000
```

### Example 6: the errors

A negative parameter or loss.

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
  report([&] { (void)actuarial::losses::layer_premium(pluss::make_series({100.0}), -1.0, 500.0); });
  report([&] { (void)actuarial::losses::layer_premium(pluss::make_series({-100.0}), 100.0, 500.0); });
}
```

Output

```text
operators::layer: attachment must be >= 0
layer: losses must be >= 0
```

## See Also

- [`loss_layer`](loss_layer.md): the layer losses
- [`lognormal_layer`](lognormal_layer.md), [`gamma_layer`](gamma_layer.md): from a distribution

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="loss_layer.html">
    <span class="label">Previous</span>
    <span class="title">loss_layer</span>
  </a>
  <a class="next-link" href="stoploss.html">
    <span class="label">Next</span>
    <span class="title">stoploss</span>
  </a>
</div>
<!-- apiref:nav:end -->
