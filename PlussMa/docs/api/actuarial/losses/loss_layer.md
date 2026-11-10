<!-- generated from loss_layer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::loss_layer`

The part of each loss falling in an excess-of-loss layer.

## Signature

```cpp
actuarial::losses::pluss::Series loss_layer(const pluss::Series &losses, double attachment, double limit);
```

## What It Does

Returns {math}`\min\big((X-a)_+,\,l\big)` for each loss: the part above the attachment `a`, capped at the layer limit `l`. It is [`operators::layer`](../loss/layer.md) with a full share, applied once. Use it to split a sample of losses between a primary cover and the layers above it; layers that meet end to end add back to the loss. [`layer_premium`](layer_premium.md) takes the mean of the result.

## Parameters

`losses`
: One-dimensional, defined, finite series of loss amounts, 0 or more.

`attachment`
: Where the layer starts, finite and 0 or more.

`limit`
: The width of the layer, finite and 0 or more.

## Returns

A series of the shape of `losses`, between 0 and `limit`.

## Errors

Throws `operators::layer: attachment must be >= 0` and `operators::layer: limit must be >= 0` (and the `must be finite` forms), and the `layer: losses must be …` errors.

## Practical Examples

### Example 1: 500 excess of 100

Zero below the attachment, the excess above it, and at most 500.

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
  out.writeln("loss  =", x.items<double>());
  out.writeln("layer =", actuarial::losses::loss_layer(x, 100.0, 500.0).items<double>());
}
```

Output

```text
loss  = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
layer = [0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
```

### Example 2: layers that tile

Three layers that meet end to end give back the loss.

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
  const pluss::Series total = actuarial::losses::loss_layer(x, 0.0, 200.0).add(actuarial::losses::loss_layer(x, 200.0, 800.0)).add(actuarial::losses::excess_loss(x, 1000.0));
  out.writeln(total.items<double>());
  out.writeln(x.items<double>());
}
```

Output

```text
[80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
[80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

### Example 3: the same as the layer operator

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
  out.writeln(actuarial::losses::loss_layer(x, 100.0, 500.0).items<double>());
  out.writeln(actuarial::operators::layer(100.0, 500.0)(x).items<double>());
}
```

Output

```text
[0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
[0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
```

### Example 4: a layer wider than the losses

A very wide layer is the excess.

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
  out.writeln(actuarial::losses::loss_layer(x, 100.0, 1.0e9).items<double>());
  out.writeln(actuarial::losses::excess_loss(x, 100.0).items<double>());
}
```

Output

```text
[0.000000, 50.000000, 200.000000, 600.000000, 1100.000000, 3900.000000]
[0.000000, 50.000000, 200.000000, 600.000000, 1100.000000, 3900.000000]
```

### Example 5: a zero-width layer

Nothing falls in it.

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
  out.writeln(actuarial::losses::loss_layer(x, 100.0, 0.0).items<double>());
}
```

Output

```text
[0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000]
```

### Example 6: the errors

Negative parameters are refused.

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
  report([&] { (void)actuarial::losses::loss_layer(pluss::make_series({100.0}), -1.0, 500.0); });
  report([&] { (void)actuarial::losses::loss_layer(pluss::make_series({100.0}), 100.0, -5.0); });
  report([&] { (void)actuarial::losses::loss_layer(pluss::make_series({-100.0}), 100.0, 500.0); });
}
```

Output

```text
operators::layer: attachment must be >= 0
operators::layer: limit must be >= 0
layer: losses must be >= 0
```

## See Also

- [`layer_premium`](layer_premium.md): its mean
- [`excess_loss`](excess_loss.md): without a limit
- [`operators::layer`](../loss/layer.md): the operator form, with a share

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="excess_loss.html">
    <span class="label">Previous</span>
    <span class="title">excess_loss</span>
  </a>
  <a class="next-link" href="layer_premium.html">
    <span class="label">Next</span>
    <span class="title">layer_premium</span>
  </a>
</div>
<!-- apiref:nav:end -->
