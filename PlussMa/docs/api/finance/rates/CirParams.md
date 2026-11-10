<!-- generated from CirParams.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::CirParams`

Parameters of the Cox-Ingersoll-Ross short-rate model `dr = κ(θ − r)dt + σ√r dW`.

The square-root rate: mean reverting with a volatility proportional to `√r`, so the rate stays non-negative (strictly positive if the Feller condition `2κθ ≥ σ²` holds). It has the closed-form bond price [`cir_zcb`](cir_zcb.md); `E r_T = θ + (r₀ − θ)e^{−κT}`. `rate_floor` is a small positive floor used by the discretisations to keep the square root defined.

## Fields

### `kappa`

`double`, default `0.7`.

Speed of mean reversion, positive.

### `theta`

`double`, default `0.03`.

Long-run level, positive.

### `sigma`

`double`, default `0.12`.

Volatility coefficient of `√r`, positive.

### `r0`

`double`, default `0.03`.

Initial rate, at least `rate_floor`.

### `rate_floor`

`double`, default `1e-12`.

Floor under the rate in discretisations; positive.

## Guidance

Passed to the `cir` path and bond functions.

## Practical Examples

### Example 1

Mean reversion: the mean of `r_T` follows `θ + (r₀ − θ)e^{−κT}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  for (const double kappa : {0.3, 1.0, 3.0}) {
    rt::CirParams p{.kappa = kappa, .theta = 0.03, .sigma = 0.1, .r0 = 0.07, .rate_floor = 1e-12};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("kappa", kappa, ": mean r_2 =", rt::cir_paths(2, 100000, 1.0, p, options).rate.select(0, 2).mean().item<double>(), ", exact =", 0.03 + 0.04 * std::exp(-2.0 * kappa));
  }
}
```

Output

```text
kappa 0.30000 : mean r_2 = 0.05201 , exact = 0.05195
kappa 1.00000 : mean r_2 = 0.03549 , exact = 0.03541
kappa 3.00000 : mean r_2 = 0.03013 , exact = 0.03010
```

### Example 2

The level `θ`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  for (const double theta : {0.01, 0.03, 0.06}) {
    rt::CirParams p{.kappa = 1.0, .theta = theta, .sigma = 0.1, .r0 = 0.03, .rate_floor = 1e-12};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("theta", theta, ": mean r_5 =", rt::cir_paths(5, 50000, 1.0, p, options).rate.select(0, 5).mean().item<double>());
  }
}
```

Output

```text
theta 0.01000 : mean r_5 = 0.01014
theta 0.03000 : mean r_5 = 0.02996
theta 0.06000 : mean r_5 = 0.05978
```

### Example 3

The volatility coefficient: the variance of `r_T` grows with `σ²`. The rate stays non-negative throughout.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const double sigma : {0.05, 0.15, 0.3}) {
    rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = sigma, .r0 = 0.03, .rate_floor = 1e-12};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    const rt::RatePaths sim = rt::cir_paths(20, 20000, 0.1, p, options);
    out.writeln("sigma", sigma, ": sd r_T =", sim.rate.select(0, 20).std().item<double>(), ", smallest rate on any path =", sim.rate.amin().item<double>());
  }
}
```

Output

```text
sigma 0.050000 : sd r_T = 0.007068 , smallest rate on any path = 0.007797
sigma 0.150000 : sd r_T = 0.021142 , smallest rate on any path = 0.000005
sigma 0.300000 : sd r_T = 0.043306 , smallest rate on any path = 0.000000
```

### Example 4

The starting rate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const double r0 : {0.005, 0.03, 0.1}) {
    rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.1, .r0 = r0, .rate_floor = 1e-12};
    out.writeln("r0", r0, ": first row =", rt::cir_paths(2, 3, 0.5, p).rate.select(0, 0).items<double>());
  }
}
```

Output

```text
r0 0.0050 : first row = [0.0050, 0.0050, 0.0050]
r0 0.0300 : first row = [0.0300, 0.0300, 0.0300]
r0 0.1000 : first row = [0.1000, 0.1000, 0.1000]
```

### Example 5

The floor. A larger floor lifts the smallest rate to it, and a start below the floor is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  rt::CirParams p{.kappa = 0.3, .theta = 0.01, .sigma = 0.3, .r0 = 0.01, .rate_floor = 1e-12};
  rt::SimOptions options;
  options.seed = 3;
  out.writeln("floor 1e-12: smallest rate", rt::cir_paths(50, 5000, 0.1, p, options).rate.amin().item<double>());
  p.rate_floor = 0.005;
  out.writeln("floor 0.005: smallest rate", rt::cir_paths(50, 5000, 0.1, p, options).rate.amin().item<double>());
  p.r0 = 0.001;
  try {
    (void)(rt::cir_paths(5, 5, 0.1, p, options));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
floor 1e-12: smallest rate 0.000000
floor 0.005: smallest rate 0.005000
subsp::math::finance::rates::cir_paths: r0 must be finite and >= rate_floor
```

## See Also

- [`SimOptions`](SimOptions.md): the simulation settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="HjmParams.html">
    <span class="label">Next</span>
    <span class="title">HjmParams</span>
  </a>
</div>
<!-- apiref:nav:end -->
