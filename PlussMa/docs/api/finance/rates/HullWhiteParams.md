<!-- generated from HullWhiteParams.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::HullWhiteParams`

Parameters of the Hull-White (extended Vasicek) model `dr = κ(θ₀ + θ₁t − r)dt + σ dW` with a linear target.

Vasicek with a time-dependent reversion level `θ(t) = θ₀ + θ₁ t`, here linear. A time-varying level lets the model start on a sloping curve; with `theta_slope = 0` it is Vasicek with `θ = θ₀`. It is Gaussian, with the closed-form bond price [`hull_white_zcb`](hull_white_zcb.md).

## Fields

### `kappa`

`double`, default `0.7`.

Speed of mean reversion, positive.

### `sigma`

`double`, default `0.01`.

Absolute volatility, positive.

### `r0`

`double`, default `0.03`.

Initial rate.

### `theta0`

`double`, default `0.03`.

Reversion level at time 0.

### `theta_slope`

`double`, default `0.0`.

Rate of change of the reversion level per year.

## Guidance

Passed to the `hull_white` path and bond functions.

## Practical Examples

### Example 1

Speed: after two years the mean is closer to the level for larger `κ`.

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
  for (const double kappa : {0.2, 0.7, 2.0}) {
    rt::HullWhiteParams p{.kappa = kappa, .sigma = 0.01, .r0 = 0.08, .theta0 = 0.03, .theta_slope = 0.0};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("kappa", kappa, ": mean r_2 =", rt::hull_white_paths(2, 100000, 1.0, p, options).rate.select(0, 2).mean().item<double>(), ", exact =", 0.03 + 0.05 * std::exp(-2.0 * kappa));
  }
}
```

Output

```text
kappa 0.20000 : mean r_2 = 0.06350 , exact = 0.06352
kappa 0.70000 : mean r_2 = 0.04232 , exact = 0.04233
kappa 2.00000 : mean r_2 = 0.03091 , exact = 0.03092
```

### Example 2

The volatility: the spread of `r_T`.

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
  for (const double sigma : {0.005, 0.01, 0.02}) {
    rt::HullWhiteParams p{.kappa = 0.7, .sigma = sigma, .r0 = 0.03, .theta0 = 0.03, .theta_slope = 0.0};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("sigma", sigma, ": sd r_3 =", rt::hull_white_paths(3, 100000, 1.0, p, options).rate.select(0, 3).std().item<double>());
  }
}
```

Output

```text
sigma 0.005000 : sd r_3 = 0.004203
sigma 0.010000 : sd r_3 = 0.008406
sigma 0.020000 : sd r_3 = 0.016812
```

### Example 3

The initial rate.

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
  for (const double r0 : {0.0, 0.03, 0.06}) {
    rt::HullWhiteParams p{.kappa = 0.7, .sigma = 0.01, .r0 = r0, .theta0 = 0.03, .theta_slope = 0.0};
    out.writeln("r0", r0, ": first row =", rt::hull_white_paths(2, 3, 0.5, p).rate.select(0, 0).items<double>());
  }
}
```

Output

```text
r0 0.0000 : first row = [0.0000, 0.0000, 0.0000]
r0 0.0300 : first row = [0.0300, 0.0300, 0.0300]
r0 0.0600 : first row = [0.0600, 0.0600, 0.0600]
```

### Example 4

The level at time zero.

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
  for (const double theta0 : {0.01, 0.03, 0.05}) {
    rt::HullWhiteParams p{.kappa = 1.0, .sigma = 0.01, .r0 = 0.03, .theta0 = theta0, .theta_slope = 0.0};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("theta0", theta0, ": mean r_5 =", rt::hull_white_paths(5, 50000, 1.0, p, options).rate.select(0, 5).mean().item<double>());
  }
}
```

Output

```text
theta0 0.01000 : mean r_5 = 0.01010
theta0 0.03000 : mean r_5 = 0.02996
theta0 0.05000 : mean r_5 = 0.04983
```

### Example 5

The slope of the level: a rising target pulls the long-run mean up. After five years the mean sits near `θ₀ + θ₁(T − 1/κ)`.

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
  for (const double slope : {0.0, 0.004, 0.01}) {
    rt::HullWhiteParams p{.kappa = 2.0, .sigma = 0.005, .r0 = 0.03, .theta0 = 0.03, .theta_slope = slope};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("slope", slope, ": mean r_5 =", rt::hull_white_paths(5, 50000, 1.0, p, options).rate.select(0, 5).mean().item<double>(), ", about", 0.03 + slope * (5.0 - 1.0 / 2.0));
  }
}
```

Output

```text
slope 0.00000 : mean r_5 = 0.02999 , about 0.03000
slope 0.00400 : mean r_5 = 0.04799 , about 0.04800
slope 0.01000 : mean r_5 = 0.07499 , about 0.07500
```

## See Also

- [`SimOptions`](SimOptions.md): the simulation settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="HjmPaths.html">
    <span class="label">Previous</span>
    <span class="title">HjmPaths</span>
  </a>
  <a class="next-link" href="RatePaths.html">
    <span class="label">Next</span>
    <span class="title">RatePaths</span>
  </a>
</div>
<!-- apiref:nav:end -->
