<!-- generated from layer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::layer`

Creates an excess-of-loss layer transform.

## Signature

```cpp
actuarial::LossTransform actuarial::operators::layer(double attachment, double limit, double share = 1.0);
```

## What It Does

Returns the operator {math}`X\mapsto\alpha\,\min\big((X-a)_+,\;l\big)`: the part of each loss above the **attachment** `a`, capped at the **limit** `l`, times the **share** `α`. It is the payment of an excess-of-loss cover, and the building block of reinsurance programmes: stacking layers that meet end to end splits every loss exactly. With `a = 0` it is a [`limit`](limit.md) of `l`; with an unbounded limit it would be a [`deductible`](deductible.md). Attachment, exhaustion and participation are applied in one vectorized pass.

## Parameters

`attachment`
: Where the layer starts, finite and 0 or more.

`limit`
: The width of the layer, finite and 0 or more.

`share`
: The participation, between 0 and 1. Default 1.

`losses` (of the returned operator)
: One-dimensional, finite, non-negative, defined series of loss amounts.

## Returns

A `LossTransform` with key `"layer"`.

## Errors

Throws `operators::layer: attachment must be >= 0`, `operators::layer: limit must be >= 0`, `operators::layer: share must be >= 0` and `operators::layer: share must be <= 1` when the operator is built. Calling the returned operator throws `<name>: losses must be defined`, `<name>: losses must be 1D`, `<name>: losses must be finite` or `<name>: losses must be >= 0`, with the operator's own name.

## Practical Examples

### Example 1: a layer of 500 xs 100

The layer pays nothing below 100, the excess above it, and at most 500.

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
  out.writeln("layer =", actuarial::operators::layer(100.0, 500.0)(x).items<double>());
}
```

Output

```text
loss  = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
layer = [0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
```

### Example 2: a half share

The insurer takes half of the layer.

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
  out.writeln(actuarial::operators::layer(100.0, 500.0, 0.5)(x).items<double>());
}
```

Output

```text
[0.000000, 25.000000, 100.000000, 250.000000, 250.000000, 250.000000]
```

### Example 3: the difference of two limits

A layer is the difference between a limit at its top and a limit at its bottom.

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
  out.writeln(actuarial::operators::layer(100.0, 500.0)(x).items<double>());
  out.writeln(actuarial::operators::limit(600.0)(x).sub(actuarial::operators::limit(100.0)(x)).items<double>());
}
```

Output

```text
[0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
[0.000000, 50.000000, 200.000000, 500.000000, 500.000000, 500.000000]
```

### Example 4: layers that tile the losses

Retained below 500, a layer of 1,500 above it, and the rest above 2,000: the three pieces sum to the loss.

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
  const pluss::Series bottom = actuarial::operators::limit(500.0)(x);
  const pluss::Series middle = actuarial::operators::layer(500.0, 1500.0)(x);
  const pluss::Series top = actuarial::operators::deductible(2000.0)(x);
  out.writeln("bottom =", bottom.items<double>());
  out.writeln("middle =", middle.items<double>());
  out.writeln("top    =", top.items<double>());
  out.writeln("sum    =", bottom.add(middle).add(top).items<double>());
}
```

Output

```text
bottom = [80.000000, 150.000000, 300.000000, 500.000000, 500.000000, 500.000000]
middle = [0.000000, 0.000000, 0.000000, 200.000000, 700.000000, 1500.000000]
top    = [0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 2000.000000]
sum    = [80.000000, 150.000000, 300.000000, 700.000000, 1200.000000, 4000.000000]
```

### Example 5: the layer premium

The mean payment of the layer is the expected loss to the reinsurer; the share scales it.

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
  const actuarial::LossMeasure mean = actuarial::operators::mean();
  out.writeln("mean layer loss     =", mean(actuarial::operators::layer(100.0, 500.0)(x)).item<double>());
  out.writeln("with a 30% share    =", mean(actuarial::operators::layer(100.0, 500.0, 0.3)(x)).item<double>());
}
```

Output

```text
mean layer loss     = 291.666667
with a 30% share    = 87.500000
```

### Example 6: the errors

Negative parameters and a share above one.

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
  report([&] { (void)actuarial::operators::layer(-1.0, 100.0); });
  report([&] { (void)actuarial::operators::layer(100.0, -1.0); });
  report([&] { (void)actuarial::operators::layer(100.0, 100.0, -0.5); });
  report([&] { (void)actuarial::operators::layer(100.0, 100.0, 1.5); });
}
```

Output

```text
operators::layer: attachment must be >= 0
operators::layer: limit must be >= 0
operators::layer: share must be >= 0
operators::layer: share must be <= 1
```

## See Also

- [`LossTransform`](LossTransform.md): the type
- [`layer`](layer.md): deductible and limit in one step

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="coinsurance.html">
    <span class="label">Previous</span>
    <span class="title">coinsurance</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
<!-- apiref:nav:end -->
