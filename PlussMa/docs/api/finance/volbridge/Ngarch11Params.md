<!-- generated from Ngarch11Params.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `volatility::Ngarch11Params`

Parameters of Duan's NGARCH(1,1)-in-mean model for pricing: the variance recursion with leverage, the risk premium and the risk-free rate.

`R_t = r + λ√h_t − ½h_t + √h_t z_t`, `h_{t+1} = ω + β h_t + α h_t (z_t + γ)²`, `z_t ~ N(0, 1)`, all per period. `r` is the risk-free rate per period, `λ` the premium per unit of volatility, and `−½h_t` the convexity correction that makes `e^{R_t}` a martingale when `λ = 0`. With `γ > 0` a negative shock raises the variance more (the leverage effect); with `γ = 0` the model is GARCH(1,1). The variance is stationary when `β + α(1 + γ²) < 1 − stationarity_margin`, with long-run level `ω/(1 − β − α(1 + γ²))`. Note the units: the parameters are per period and the model is written for **log returns in decimals** (the convexity term `−½h` has the size of a variance, so with returns in percent it dominates the mean and the recursion can explode); a daily model's `r` is a daily rate.

## Fields

### `omega`

`double`, default `0.02`.

Variance intercept, positive.

### `alpha`

`double`, default `0.08`.

Reaction to the shock, non-negative.

### `beta`

`double`, default `0.90`.

Variance persistence, non-negative.

### `gamma`

`double`, default `0.0`.

Leverage: shifts the shock inside the square; positive makes bad news raise the variance more.

### `lambda`

`double`, default `0.0`.

Risk premium per unit of volatility.

### `r`

`double`, default `0.0`.

Risk-free rate per period.

### `variance_floor`

`double`, default `1e-12`.

Floor under the variance.

### `stationarity_margin`

`double`, default `1e-6`.

The persistence must be below `1 − margin`.

## Guidance

Passed to `pricing_metrics`, `pricing_variance_forecast` and `unconditional_variance`.

## Practical Examples

### Example 1

`omega`, `alpha`, `beta`: the long-run variance `ω/(1 − β − α)` for `γ = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const double omega : {0.01, 0.02, 0.04}) {
    fin::volatility::Ngarch11Params p{.omega = omega, .alpha = 0.08, .beta = 0.90};
    out.writeln("omega", omega, ": unconditional variance", fin::volatility::unconditional_variance(p), ", formula", omega / (1.0 - 0.9 - 0.08));
  }
}
```

Output

```text
omega 0.0100 : unconditional variance 0.5000 , formula 0.5000
omega 0.0200 : unconditional variance 1.0000 , formula 1.0000
omega 0.0400 : unconditional variance 2.0000 , formula 2.0000
```

### Example 2

`alpha` and `beta`: persistence and long-run variance. The persistence rises and the long-run variance with it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <utility>
#include <vector>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const auto &[alpha, beta] : std::vector<std::pair<double, double>>{{0.05, 0.90}, {0.08, 0.90}, {0.08, 0.91}}) {
    fin::volatility::Ngarch11Params p{.omega = 0.02, .alpha = alpha, .beta = beta};
    out.writeln("alpha", alpha, "beta", beta, ": persistence", alpha + beta, ", unconditional variance", fin::volatility::unconditional_variance(p));
  }
}
```

Output

```text
alpha 0.0500 beta 0.9000 : persistence 0.9500 , unconditional variance 0.4000
alpha 0.0800 beta 0.9000 : persistence 0.9800 , unconditional variance 1.0000
alpha 0.0800 beta 0.9100 : persistence 0.9900 , unconditional variance 2.0000
```

### Example 3

`gamma`: the leverage raises the persistence `β + α(1 + γ²)` and so the long-run variance; the formula is `ω/(1 − β − α(1 + γ²))`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  for (const double gamma : {0.0, 0.5, 1.0}) {
    fin::volatility::Ngarch11Params p{.omega = 0.02, .alpha = 0.05, .beta = 0.85, .gamma = gamma};
    out.writeln("gamma", gamma, ": unconditional variance", fin::volatility::unconditional_variance(p), ", formula", 0.02 / (1.0 - 0.85 - 0.05 * (1.0 + gamma * gamma)));
  }
}
```

Output

```text
gamma 0.0000 : unconditional variance 0.2000 , formula 0.2000
gamma 0.5000 : unconditional variance 0.2286 , formula 0.2286
gamma 1.0000 : unconditional variance 0.4000 , formula 0.4000
```

### Example 4

`lambda` and `r`: the mean of the pricing-measure return is `r + λ√h − h/2`; with `λ = 0` the returns have mean `r − h/2` per period. They do not change the variance recursion, so the long-run variance is unchanged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const Series daily = returns * 0.01;   // log returns in decimals, the unit the pricing model assumes

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  for (const double lambda : {0.0, 0.05}) {
    const fin::volatility::Ngarch11Params p{.omega = 3e-6, .alpha = 0.06, .beta = 0.88, .lambda = lambda, .r = 1e-4};
    const auto m = fin::pricing::pricing_metrics(daily, p, cfg);
    out.writeln("lambda", lambda, ": first conditional mean", m.mean.select(0, 0).item<double>(), ", loglik", m.loglik, ", unconditional variance", fin::volatility::unconditional_variance(p));
  }
}
```

Output

```text
lambda 0.000000 : first conditional mean 0.000075 , loglik 5294.242410 , unconditional variance 0.000050
lambda 0.050000 : first conditional mean 0.000429 , loglik 5295.850124 , unconditional variance 0.000050
```

### Example 5

`variance_floor`: a floor under the variance keeps the likelihood finite for a degenerate parameter set.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const Series daily = returns * 0.01;   // log returns in decimals, the unit the pricing model assumes

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  for (const double floor : {1e-12, 1e-4}) {
    const auto m = fin::pricing::pricing_metrics(daily, fin::volatility::Ngarch11Params{.omega = 3e-6, .alpha = 0.06, .beta = 0.88, .variance_floor = floor}, cfg);
    out.writeln("floor", floor, ": smallest variance", m.variance.amin().item<double>());
  }
}
```

Output

```text
floor 0.00000000 : smallest variance 0.00002887
floor 0.00010000 : smallest variance 0.00010000
```

### Example 6

`stationarity_margin`: a parameter set that is not stationary is refused. Persistence 1.0 is not allowed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const fin::volatility::Ngarch11Params p{.omega = 0.02, .alpha = 0.1, .beta = 0.9};
  try {
    (void)(fin::volatility::unconditional_variance(p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::unconditional_variance: requires beta + alpha*(1+gamma^2) < 1 - stationarity_margin
```

## See Also

- [`pricing_metrics`](pricing_metrics.md): uses it
- [`unconditional_variance`](unconditional_variance.md): its long-run variance

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="JointLikelihoodWeights.html">
    <span class="label">Previous</span>
    <span class="title">JointLikelihoodWeights</span>
  </a>
  <a class="next-link" href="OptionVolMetrics.html">
    <span class="label">Next</span>
    <span class="title">OptionVolMetrics</span>
  </a>
</div>
<!-- apiref:nav:end -->
