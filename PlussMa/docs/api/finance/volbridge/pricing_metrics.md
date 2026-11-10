<!-- generated from pricing_metrics.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pricing::pricing_metrics`

The likelihood, variances and innovations of returns under NGARCH or GARCH pricing dynamics.

## Signature

```cpp
pricing::OptionVolMetrics pricing::pricing_metrics(const pluss::Series &returns, const volatility::Ngarch11Params &params, pluss::Config cfg = {});
pricing::OptionVolMetrics pricing::pricing_metrics(const pluss::Series &returns, const volatility::Garch11Params &params, pluss::Config cfg = {});
```

## What It Does

Runs the recursion `h_{t+1} = ω + β h_t + α h_t (z_t + γ)²` over the returns with the shock recovered from each return (`z_t = (R_t − mean_t)/√h_t`) and returns the Gaussian likelihood under the pricing dynamics, which include the risk premium. Because the premium appears in the mean, the likelihood of returns identifies it only weakly; it is used to keep the pricing parameters consistent with the data, jointly with option prices ([`weighted_joint_loglik`](weighted_joint_loglik.md)).

## Parameters

`const pluss::Series &returns`
: Returns, one per period.

`const volatility::Ngarch11Params &params / Garch11Params`
: The pricing dynamics.

`pluss::Config cfg`
: Device.

## Returns

An [`OptionVolMetrics`](OptionVolMetrics.md).

## Errors

- Non-stationary parameters or invalid returns throw (`pluss::Error`).

## Practical Examples

### Example 1

The likelihood under a parameter set, and a profile over the leverage `γ` on data with a real asymmetry.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const Series daily = returns * 0.01;   // log returns in decimals, the unit the pricing model assumes
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  for (const double gamma : {-1.0, 0.0, 0.5, 1.0})
    out.writeln("gamma", gamma, ": loglik", fin::pricing::pricing_metrics(daily, fin::volatility::Ngarch11Params{.omega = 3e-6, .alpha = 0.04, .beta = 0.88, .gamma = gamma}, cfg).loglik);
}
```

Output

```text
gamma -1.00 : loglik 5288.99
gamma 0.00 : loglik 5272.98
gamma 0.50 : loglik 5264.97
gamma 1.00 : loglik 5227.33
```

### Example 2

A plain GARCH set of parameters.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const Series daily = returns * 0.01;   // log returns in decimals, the unit the pricing model assumes
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const auto m = fin::pricing::pricing_metrics(daily, fin::volatility::Garch11Params{.omega = 3e-6, .alpha = 0.08, .beta = 0.88}, cfg);
  out.writeln("loglik", m.loglik, ", mean conditional variance", m.variance.mean().item<double>());
}
```

Output

```text
loglik 5288.601 , mean conditional variance 0.000
```

### Example 3

The risk premium shifts the mean and so the likelihood.

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
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const Series daily = returns * 0.01;   // log returns in decimals, the unit the pricing model assumes
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  for (const double lambda : {0.0, 0.02, 0.1})
    out.writeln("lambda", lambda, ": loglik", fin::pricing::pricing_metrics(daily, fin::volatility::Ngarch11Params{.omega = 3e-6, .alpha = 0.06, .beta = 0.88, .lambda = lambda}, cfg).loglik);
}
```

Output

```text
lambda 0.0000 : loglik 5293.1957
lambda 0.0200 : loglik 5294.7505
lambda 0.1000 : loglik 5294.7543
```

### Example 4

Non-stationary parameters are refused.

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
  const fin::volatility::Ngarch11Params p{.omega = 0.02, .alpha = 0.3, .beta = 0.8, .gamma = 0.5};
  try {
    (void)(fin::pricing::pricing_metrics(pluss::make_series({0.1, 0.2, -0.1}), p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::pricing_metrics: requires beta + alpha*(1+gamma^2) < 1 - stationarity_margin
```

## See Also

- [`OptionVolMetrics`](OptionVolMetrics.md): the result
- [`to_pricing_params`](to_pricing_params.md): from a fit

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ReturnsMetrics.html">
    <span class="label">Previous</span>
    <span class="title">ReturnsMetrics</span>
  </a>
  <a class="next-link" href="pricing_variance_forecast.html">
    <span class="label">Next</span>
    <span class="title">pricing_variance_forecast</span>
  </a>
</div>
<!-- apiref:nav:end -->
