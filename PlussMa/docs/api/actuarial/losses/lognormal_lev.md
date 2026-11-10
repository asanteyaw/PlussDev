<!-- generated from lognormal_lev.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `actuarial::losses::lognormal_lev`

Limited expected value of a lognormal severity.

## Signature

```cpp
actuarial::losses::pluss::Series lognormal_lev(double mean_log, double stddev_log, double cap, std::int64_t quadrature_steps = 2048, pluss::Platform platform = pluss::Platform::CPU);
```

## What It Does

The limited expected value is {math}`E[\min(X,c)]=\int_0^{c}S(x)\,dx` with {math}`S` the survival function, evaluated by the midpoint rule on `quadrature_steps` equal subintervals; the error falls as the square of the step. It is the expected payment per claim of a policy with limit `c`, the figure a limit rate is built from, and the building block of [`lognormal_layer`](lognormal_layer.md). It rises with the cap and tends to the mean as the cap grows, more slowly the heavier the tail. A spread of zero is the constant {math}`e^{\mu}`, so the value is {math}`\min(e^{\mu},c)`.

## Parameters

`mean_log`, `stddev_log`
: The mean and standard deviation of the log-claim: finite, and `stddev_log` 0 or more.

`cap`
: The limit, finite and 0 or more.

`quadrature_steps`
: Number of equal subintervals of `[0, cap]` in the midpoint rule, greater than 0. Default 2048. More steps give a more accurate value.

`platform`
: Where the result is placed. Default `pluss::Platform::CPU`.

## Returns

A rank-zero series.

## Errors

Throws `lognormal_lev: cap must be >= 0`, `lognormal_lev: quadrature_steps must be > 0`, `lognormal_lev: mean_log must be finite` and `lognormal_lev: stddev_log must be >= 0`.

## Practical Examples

### Example 1: a limit of 2

The standard lognormal capped at 2.

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
  out.writeln(actuarial::losses::lognormal_lev(0.0, 1.0, 2.0).item<double>());
}
```

Output

```text
1.113870
```

### Example 2: against the closed form

The lognormal limited expected value has a closed form in the normal distribution function.

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
  const double mu = 0.0, sigma = 1.0, c = 2.0;
  const auto Phi = [](double z) { return 0.5 * std::erfc(-z / std::sqrt(2.0)); };
  const double closed = std::exp(mu + sigma * sigma / 2.0) * Phi((std::log(c) - mu - sigma * sigma) / sigma) + c * (1.0 - Phi((std::log(c) - mu) / sigma));
  out.writeln(closed, actuarial::losses::lognormal_lev(mu, sigma, c).item<double>());
}
```

Output

```text
1.11387015 1.11387016
```

### Example 3: the effect of the quadrature steps

The value converges as the number of steps grows.

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
  for (const int n : {4, 16, 64, 256, 2048, 16384})
    out.writeln("steps", n, " value =", actuarial::losses::lognormal_lev(0.0, 1.0, 2.0, n).item<double>());
}
```

Output

```text
steps 4  value = 1.11497954
steps 16  value = 1.11401749
steps 64  value = 1.11387653
steps 256  value = 1.11387055
steps 2048  value = 1.11387016
steps 16384  value = 1.11387015
```

### Example 4: rising with the cap

The value rises toward the mean, which is exp(0.5) = 1.6487.

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
  for (const double cap : {0.5, 1.0, 2.0, 5.0, 20.0, 100.0})
    out.writeln("cap", cap, " lev =", actuarial::losses::lognormal_lev(0.0, 1.0, cap).item<double>());
}
```

Output

```text
cap 0.500000  lev = 0.452491
cap 1.000000  lev = 0.761578
cap 2.000000  lev = 1.113870
cap 5.000000  lev = 1.470526
cap 20.000000  lev = 1.638210
cap 100.000000  lev = 1.648671
```

### Example 5: no spread and no cap

A constant claim is limited by the cap; a cap of zero pays nothing.

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
  out.writeln("constant e^1 = 2.718 capped at 2:", actuarial::losses::lognormal_lev(1.0, 0.0, 2.0).item<double>());
  out.writeln("constant e^1 capped at 5:", actuarial::losses::lognormal_lev(1.0, 0.0, 5.0).item<double>());
  out.writeln("zero cap:", actuarial::losses::lognormal_lev(0.0, 1.0, 0.0).item<double>());
}
```

Output

```text
constant e^1 = 2.718 capped at 2: 2.000000
constant e^1 capped at 5: 2.718282
zero cap: 0.000000
```

### Example 6: the errors

A negative cap, no steps, a negative spread.

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
  report([&] { (void)actuarial::losses::lognormal_lev(0.0, 1.0, -1.0); });
  report([&] { (void)actuarial::losses::lognormal_lev(0.0, 1.0, 2.0, 0); });
  report([&] { (void)actuarial::losses::lognormal_lev(0.0, -1.0, 2.0); });
  report([&] { (void)actuarial::losses::lognormal_lev(std::nan(""), 1.0, 2.0); });
}
```

Output

```text
lognormal_lev: cap must be >= 0
lognormal_lev: quadrature_steps must be > 0
lognormal_lev: stddev_log must be >= 0
lognormal_lev: mean_log must be finite
```

## See Also

- [`lognormal_layer`](lognormal_layer.md): a layer from the same law
- [`lognormal_moments`](lognormal_moments.md): the unlimited mean

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stoploss.html">
    <span class="label">Previous</span>
    <span class="title">stoploss</span>
  </a>
  <a class="next-link" href="gamma_lev.html">
    <span class="label">Next</span>
    <span class="title">gamma_lev</span>
  </a>
</div>
<!-- apiref:nav:end -->
