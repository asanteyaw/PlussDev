<!-- generated from VasicekParams.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::VasicekParams`

Parameters of the Vasicek short-rate model `dr = κ(θ − r)dt + σ dW`.

The Gaussian mean-reverting rate: it reverts to the level `θ` at speed `κ`, with a constant absolute volatility `σ`. It can go negative (as rates have), has the closed-form bond price [`vasicek_zcb`](vasicek_zcb.md), and `E r_T = θ + (r₀ − θ)e^{−κT}`, `Var r_T = σ²(1 − e^{−2κT})/(2κ)`.

## Fields

### `kappa`

`double`, default `0.7`.

Speed of mean reversion, positive.

### `theta`

`double`, default `0.03`.

Long-run level, any finite number.

### `sigma`

`double`, default `0.01`.

Absolute volatility, positive.

### `r0`

`double`, default `0.03`.

Initial short rate, finite.

## Guidance

Passed to the `vasicek` path and bond functions.

## Practical Examples

### Example 1

Mean reversion speed: after two years the mean is `θ + (r₀ − θ)e^{−2κ}`; a faster reversion is closer to `θ`.

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
    rt::VasicekParams p{.kappa = kappa, .theta = 0.03, .sigma = 0.01, .r0 = 0.08};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("kappa", kappa, ": mean r_2 =", rt::vasicek_paths(2, 100000, 1.0, p, options).rate.select(0, 2).mean().item<double>(), ", exact =", 0.03 + 0.05 * std::exp(-2.0 * kappa));
  }
}
```

Output

```text
kappa 0.20000 : mean r_2 = 0.06352 , exact = 0.06352
kappa 0.70000 : mean r_2 = 0.04232 , exact = 0.04233
kappa 2.00000 : mean r_2 = 0.03091 , exact = 0.03092
```

### Example 2

The long-run level.

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
  for (const double theta : {0.01, 0.03, 0.06}) {
    rt::VasicekParams p{.kappa = 1.0, .theta = theta, .sigma = 0.01, .r0 = 0.03};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("theta", theta, ": mean r_5 =", rt::vasicek_paths(5, 50000, 1.0, p, options).rate.select(0, 5).mean().item<double>(), ", exact =", theta + (0.03 - theta) * std::exp(-5.0));
  }
}
```

Output

```text
theta 0.01000 : mean r_5 = 0.01016 , exact = 0.01013
theta 0.03000 : mean r_5 = 0.03002 , exact = 0.03000
theta 0.06000 : mean r_5 = 0.05982 , exact = 0.05980
```

### Example 3

The volatility: the standard deviation of `r_T` is `σ√((1 − e^{−2κT})/(2κ))`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  for (const double sigma : {0.005, 0.01, 0.02}) {
    rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = sigma, .r0 = 0.03};
    rt::SimOptions options;
    options.scheme = rt::Scheme::Transition;
    options.seed = 3;
    out.writeln("sigma", sigma, ": sd r_3 =", rt::vasicek_paths(3, 100000, 1.0, p, options).rate.select(0, 3).std().item<double>(), ", exact =", sigma * std::sqrt((1.0 - std::exp(-2.0 * 0.7 * 3.0)) / (2.0 * 0.7)));
  }
}
```

Output

```text
sigma 0.005000 : sd r_3 = 0.004196 , exact = 0.004194
sigma 0.010000 : sd r_3 = 0.008392 , exact = 0.008388
sigma 0.020000 : sd r_3 = 0.016784 , exact = 0.016776
```

### Example 4

The initial rate: the start of every path. A negative start is allowed.

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
  for (const double r0 : {-0.01, 0.03, 0.08}) {
    rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = r0};
    out.writeln("r0", r0, ": first row of the paths =", rt::vasicek_paths(2, 3, 0.5, p).rate.select(0, 0).items<double>());
  }
}
```

Output

```text
r0 -0.0100 : first row of the paths = [-0.0100, -0.0100, -0.0100]
r0 0.0300 : first row of the paths = [0.0300, 0.0300, 0.0300]
r0 0.0800 : first row of the paths = [0.0800, 0.0800, 0.0800]
```

### Example 5

Invalid parameters are refused: a non-positive `kappa`.

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
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::VasicekParams p{.kappa = -1.0};
  try {
    (void)(rt::vasicek_paths(2, 3, 0.5, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::vasicek_paths: kappa must be finite and > 0
```

## See Also

- [`SimOptions`](SimOptions.md): the simulation settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="SimOptions.html">
    <span class="label">Previous</span>
    <span class="title">SimOptions</span>
  </a>
  <a class="next-link" href="cir_paths.html">
    <span class="label">Next</span>
    <span class="title">cir_paths</span>
  </a>
</div>
<!-- apiref:nav:end -->
