<!-- generated from lognormal_moments.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::lognormal_moments`

Mean and variance of a lognormal claim severity.

## Signature

```cpp
actuarial::losses::AggregateMoments lognormal_moments(double mean_log, double stddev_log, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

If {math}`\ln X` is normal with mean {math}`\mu` and standard deviation {math}`\sigma`, then {math}`X` has mean {math}`e^{\mu+\sigma^2/2}` and variance {math}`(e^{\sigma^2}-1)\,e^{2\mu+\sigma^2}`. The lognormal is the usual heavy-tailed severity for liability and property claims: positive, skewed, with a mean far above the median {math}`e^{\mu}`. The parameters are those of the **logarithm**, not of the claim. Use the pair as the severity input of [`aggregate_moments`](aggregate_moments.md).

## Parameters

`mean_log`
: The mean {math}`\mu` of the log-claim, finite (negative values are valid).

`stddev_log`
: The standard deviation {math}`\sigma` of the log-claim, finite and 0 or more.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

An [`AggregateMoments`](AggregateMoments.md) of two rank-zero series.

## Errors

Throws `lognormal_moments: mean_log must be finite`, `lognormal_moments: stddev_log must be finite` and `lognormal_moments: stddev_log must be >= 0`.

## Practical Examples

### Example 1: mu = 0, sigma = 1

The standard lognormal has mean e^0.5.

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
  const auto m = actuarial::losses::lognormal_moments(0.0, 1.0);
  out.writeln("mean =", m.mean.item<double>(), " variance =", m.variance.item<double>());
}
```

Output

```text
mean = 1.648721  variance = 4.670774
```

### Example 2: by hand

The closed forms.

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
  const double mu = 7.0, sigma = 1.2;
  const auto m = actuarial::losses::lognormal_moments(mu, sigma);
  out.writeln(m.mean.item<double>(), std::exp(mu + sigma * sigma / 2.0));
  out.writeln(m.variance.item<double>(), (std::exp(sigma * sigma) - 1.0) * std::exp(2.0 * mu + sigma * sigma));
}
```

Output

```text
2252.959581 2252.959581
16347694.373407 16347694.373407
```

### Example 3: the mean exceeds the median

The skew puts the mean above e^mu, more so as sigma grows.

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
  for (const double sigma : {0.25, 0.75, 1.5})
    out.writeln("sigma", sigma, " mean / median =", actuarial::losses::lognormal_moments(0.0, sigma).mean.item<double>() / std::exp(0.0));
}
```

Output

```text
sigma 0.250000  mean / median = 1.031743
sigma 0.750000  mean / median = 1.324785
sigma 1.500000  mean / median = 3.080217
```

### Example 4: the coefficient of variation

It depends on sigma alone.

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
  for (const double mu : {0.0, 5.0}) {
    const auto m = actuarial::losses::lognormal_moments(mu, 1.0);
    out.writeln("mu", mu, " cv =", std::sqrt(m.variance.item<double>()) / m.mean.item<double>());
  }
}
```

Output

```text
mu 0.000000  cv = 1.310832
mu 5.000000  cv = 1.310832
```

### Example 5: the errors

A non-finite mean or a negative spread is refused.

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
  report([&] { (void)actuarial::losses::lognormal_moments(std::nan(""), 1.0); });
  report([&] { (void)actuarial::losses::lognormal_moments(0.0, -1.0); });
}
```

Output

```text
lognormal_moments: mean_log must be finite
lognormal_moments: stddev_log must be >= 0
```

## See Also

- [`AggregateMoments`](AggregateMoments.md): the result type
- [`aggregate_moments`](aggregate_moments.md): combine a frequency and a severity
- [`lognormal_lev`](lognormal_lev.md): limited expected value of the same law

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="negbin_moments.html">
    <span class="label">Previous</span>
    <span class="title">negbin_moments</span>
  </a>
  <a class="next-link" href="gamma_moments.html">
    <span class="label">Next</span>
    <span class="title">gamma_moments</span>
  </a>
</div>
<!-- apiref:nav:end -->
