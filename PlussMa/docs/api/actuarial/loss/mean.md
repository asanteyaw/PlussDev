<!-- generated from mean.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::operators::mean`

Creates the empirical-mean measure.

## Signature

```cpp
actuarial::LossMeasure actuarial::operators::mean();
```

## What It Does

Returns the operator that reduces a one-dimensional sample of amounts to its arithmetic mean, as a rank-zero series: read it with `item<double>()`. Apply it to losses for the average claim, or to the output of a [`LossTransform`](LossTransform.md) for the **expected payment** of a contract on the sample, which is the pure premium per claim. An empty sample gives zero rather than an error, so a portfolio with no claims in a cell stays defined. The reduction stays on the input's platform.

## Parameters

`losses` (of the returned operator)
: One-dimensional, finite, non-negative, defined series of loss amounts.

## Returns

A `LossMeasure` with key `"mean"`.

## Errors

Calling the returned operator throws `mean: losses must be defined`, `mean: losses must be 1D`, `mean: losses must be finite` or `mean: losses must be >= 0`: like the transforms it takes amounts, not signed values.

## Practical Examples

### Example 1: the average loss

Six losses.

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
  out.writeln(actuarial::operators::mean()(x).item<double>());
}
```

Output

```text
1071.666667
```

### Example 2: the expected payment of a contract

Composing a transform and the mean gives the pure premium per claim.

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
  out.writeln("ground up        =", mean(x).item<double>());
  out.writeln("deductible 100   =", mean(actuarial::operators::deductible(100.0)(x)).item<double>());
  out.writeln("limit 1000       =", mean(actuarial::operators::limit(1000.0)(x)).item<double>());
  out.writeln("layer 1000 xs 500 =", mean(actuarial::operators::layer(500.0, 1000.0)(x)).item<double>());
}
```

Output

```text
ground up        = 1071.666667
deductible 100   = 975.000000
limit 1000       = 538.333333
layer 1000 xs 500 = 316.666667
```

### Example 3: an empty sample

The mean is zero, as a rank-zero series.

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
  const pluss::Series none = pluss::series::zeros({0});
  const pluss::Series m = actuarial::operators::mean()(none);
  out.writeln("rank", m.dim(), " value", m.item<double>());
}
```

Output

```text
rank 0  value 0.000000
```

### Example 4: the definition

The sum divided by the count.

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
  out.writeln(actuarial::operators::mean()(x).item<double>(), x.sum().item<double>() / 6.0);
}
```

Output

```text
1071.666667 1071.666667
```

### Example 5: the errors

A matrix, a non-finite value and an undefined sample.

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
  report([&] { (void)actuarial::operators::mean()(pluss::make_series({1.0, 2.0, 3.0, 4.0}).reshape({2, 2})); });
  report([&] { (void)actuarial::operators::mean()(pluss::make_series({1.0, std::nan("")})); });
  report([&] { (void)actuarial::operators::mean()(pluss::series::undefined()); });
}
```

Output

```text
mean: losses must be 1D
mean: losses must be finite
mean: losses must be defined
```

## See Also

- [`LossMeasure`](LossMeasure.md): the type
- [`deductible`](deductible.md), [`limit`](limit.md), [`layer`](layer.md): the contracts to average

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="layer.html">
    <span class="label">Previous</span>
    <span class="title">layer</span>
  </a>
</div>
<!-- apiref:nav:end -->
