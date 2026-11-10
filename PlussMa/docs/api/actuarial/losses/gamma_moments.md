<!-- generated from gamma_moments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::gamma_moments`

Mean and variance of a gamma claim severity.

## Signature

```cpp
actuarial::losses::AggregateMoments gamma_moments(double shape, double scale, double location = 0.0, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

A gamma severity with shape {math}`k` and scale {math}`\theta` has mean {math}`k\theta` and variance {math}`k\theta^2`; the optional **location** shifts the whole distribution to the right, adding to the mean and leaving the variance alone, which models a minimum claim size (a threshold below which nothing is reported). The gamma is lighter-tailed than the lognormal; shape 1 is the exponential. Use the pair as the severity input of [`aggregate_moments`](aggregate_moments.md).

## Parameters

`shape`
: The shape {math}`k`, finite and greater than 0.

`scale`
: The scale {math}`\theta`, finite and greater than 0.

`location`
: The shift, finite and 0 or more. Default 0.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

An [`AggregateMoments`](AggregateMoments.md) of two rank-zero series.

## Errors

Throws `gamma_moments: shape must be > 0`, `gamma_moments: scale must be > 0`, `gamma_moments: location must be >= 0` and the corresponding `must be finite` errors.

## Practical Examples

### Example 1: shape 2, scale 3

Mean 6 and variance 18.

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
  const auto m = actuarial::losses::gamma_moments(2.0, 3.0);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 6.000000  variance = 18.000000
```

### Example 2: with a location

A shift of 5 adds 5 to the mean and leaves the variance.

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
  const auto m = actuarial::losses::gamma_moments(2.0, 3.0, 5.0);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 11.000000  variance = 18.000000
```

### Example 3: the exponential

Shape 1 gives mean equal to the scale and variance its square.

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
  const auto m = actuarial::losses::gamma_moments(1.0, 4.0);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 4.000000  variance = 16.000000
```

### Example 4: matching a mean and variance

Solve shape = mean^2 / variance and scale = variance / mean for a mean of 500 and a standard deviation of 400.

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
  const double mean = 500.0, variance = 400.0 * 400.0;
  const double shape = mean * mean / variance, scale = variance / mean;
  const auto m = actuarial::losses::gamma_moments(shape, scale);
  out.writeln("shape", shape, " scale", scale, " mean", m.mean.item<double>(), " variance", m.variance.item<double>());
}
```

Output

```text
shape 1.562500  scale 320.000000  mean 500.000000  variance 160000.000000
```

### Example 5: the errors

Non-positive shape or scale and a negative location are refused.

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
  report([&] { (void)actuarial::losses::gamma_moments(0.0, 3.0); });
  report([&] { (void)actuarial::losses::gamma_moments(2.0, 0.0); });
  report([&] { (void)actuarial::losses::gamma_moments(2.0, 3.0, -1.0); });
}
```

Output

```text
gamma_moments: shape must be > 0
gamma_moments: scale must be > 0
gamma_moments: location must be >= 0
```

## See Also

- [`AggregateMoments`](AggregateMoments.md): the result type
- [`aggregate_moments`](aggregate_moments.md): combine a frequency and a severity
- [`gamma_lev`](gamma_lev.md): limited expected value of the same law

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lognormal_moments.html">
    <span class="label">Previous</span>
    <span class="title">lognormal_moments</span>
  </a>
  <a class="next-link" href="aggregate_moments.html">
    <span class="label">Next</span>
    <span class="title">aggregate_moments</span>
  </a>
</div>
<!-- apiref:nav:end -->
