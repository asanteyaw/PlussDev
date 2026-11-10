<!-- generated from variance_forecast.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `bridge::variance_forecast`

The multi-step variance forecast of a fitted GARCH, EGARCH or GJR model, starting from the end of a return sample.

## Signature

```cpp
pluss::Series bridge::variance_forecast(std::int64_t horizon, const pluss::Series &returns, const stats::ts::GarchFit &fit, pluss::Config cfg = {});
pluss::Series bridge::variance_forecast(std::int64_t horizon, const pluss::Series &returns, const stats::ts::EgarchFit &fit, pluss::Config cfg = {});
pluss::Series bridge::variance_forecast(std::int64_t horizon, const pluss::Series &returns, const stats::ts::GjrFit &fit, pluss::Config cfg = {});
```

## What It Does

Runs the model over `returns` to get the conditional variance at the end of the sample, and projects it `horizon` periods ahead; the result has `horizon` entries, the forecast variance of the first, second, … period after the sample. For a stationary model the forecast converges to the unconditional variance: the horizon at which it gets there is set by the persistence. The sum of the entries is the forecast variance of the cumulative return, the input an option price needs.

## Parameters

`std::int64_t horizon`
: Number of periods, positive.

`const pluss::Series &returns`
: The sample up to now.

`const stats::ts::GarchFit &fit / EgarchFit / GjrFit`
: A fitted model.

`pluss::Config cfg`
: Device.

## Returns

A `[horizon]` `Series` of forecast variances.

## Errors

- A non-positive horizon or invalid returns throw (`pluss::Error`).

## Practical Examples

### Example 1

The forecast reverts to the long-run variance. From a high-variance state it declines, from a low one it rises.

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
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const Series f = fin::bridge::variance_forecast(60, returns, garch, cfg);
  out.writeln("forecast at steps 1, 10, 30, 60:", f.select(0, 0).item<double>(), f.select(0, 9).item<double>(), f.select(0, 29).item<double>(), f.select(0, 59).item<double>(), "; long-run", garch.unconditional_var());
}
```

Output

```text
forecast at steps 1, 10, 30, 60: 0.4607 0.4951 0.5293 0.5419 ; long-run 0.5445
```

### Example 2

The cumulative variance over a horizon: the sum of the forecast, as used for an option's total variance.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

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
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const Series f = fin::bridge::variance_forecast(21, returns, gjr, cfg);
  out.writeln("variance of the next 21 periods' cumulative return:", f.sum().item<double>(), ", equivalent period volatility", std::sqrt(f.sum().item<double>() / 21.0));
}
```

Output

```text
variance of the next 21 periods' cumulative return: 8.9773 , equivalent period volatility 0.6538
```

### Example 3

EGARCH forecasts too.

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
  ts::EgarchFit egarch;
  egarch.mean = 0.04; egarch.omega = -0.1; egarch.arch = {0.15}; egarch.asym = {-0.05}; egarch.garch = {0.95};
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const Series f = fin::bridge::variance_forecast(10, returns, egarch, cfg);
  out.writeln(f.items<double>());
}
```

Output

```text
[0.2249, 0.2205, 0.2163, 0.2124, 0.2086, 0.2051, 0.2018, 0.1987, 0.1957, 0.1929]
```

### Example 4

Starting after a shock. Appending one extreme return raises the whole forecast, which then decays.

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
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const Series shocked = returns.unite({pluss::make_series({-8.0})}, 0);
  out.writeln("first forecast without / with the shock:", fin::bridge::variance_forecast(1, returns, garch, cfg).select(0, 0).item<double>(), fin::bridge::variance_forecast(1, shocked, garch, cfg).select(0, 0).item<double>());
}
```

Output

```text
first forecast without / with the shock: 0.4607 7.4337
```

### Example 5

A non-positive horizon is refused.

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
  ts::GjrFit truth;
  truth.mean = 0.04; truth.omega = 0.03; truth.arch = {0.03}; truth.asym = {0.12}; truth.garch = {0.88};
  const Series returns = truth.simulate(1500, pluss::Config{}.with_dtype(pluss::ScalarType::Float64), 11);
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));

  try {
    (void)(fin::bridge::variance_forecast(0, returns, garch));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::stats::ts::GarchFit::forecast: horizon must be > 0
```

### Example 6

GARCH against GJR: the GJR forecast starts from its own (asymmetric) state and reverts to its own long-run variance.

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
  const auto garch = ts::Garch::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto gjr = ts::Gjr::fit_mle(returns, {}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);

  const Series a = fin::bridge::variance_forecast(40, returns, garch, cfg), b = fin::bridge::variance_forecast(40, returns, gjr, cfg);
  out.writeln("GARCH steps 1 and 40:", a.select(0, 0).item<double>(), a.select(0, 39).item<double>(), "; GJR:", b.select(0, 0).item<double>(), b.select(0, 39).item<double>());
}
```

Output

```text
GARCH steps 1 and 40: 0.4607 0.5360 ; GJR: 0.3672 0.4998
```

## See Also

- [`returns_metrics`](returns_metrics.md): the likelihood and variances
- [`pricing_variance_forecast`](pricing_variance_forecast.md): under pricing dynamics

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unconditional_variance.html">
    <span class="label">Previous</span>
    <span class="title">unconditional_variance</span>
  </a>
  <a class="next-link" href="weighted_joint_loglik.html">
    <span class="label">Next</span>
    <span class="title">weighted_joint_loglik</span>
  </a>
</div>
<!-- apiref:nav:end -->
