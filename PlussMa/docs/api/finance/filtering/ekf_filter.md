<!-- generated from ekf_filter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::ekf_filter`

The extended Kalman filter: the Kalman equations linearised around the current estimate.

## Signature

```cpp
filtering::KalmanResult filtering::ekf_filter(const pluss::Series &observations, const filtering::EkfModel &model, pluss::Config cfg = {});
```

## What It Does

At each date it predicts the mean through `f` and the covariance through the Jacobian of `f`, then corrects with the observation through `h` and the Jacobian of `h`. It returns the same [`KalmanResult`](KalmanResult.md) as the linear filter. It needs the Jacobians; it is accurate where the model is nearly linear over the width of the posterior and degrades, sometimes badly, where it is not (strong curvature, multimodal posteriors).

## Parameters

`const pluss::Series &observations`
: `[T]` or `[T, m]`; NaN marks a missing value.

`const filtering::EkfModel &model`
: The nonlinear model with its Jacobians.

`pluss::Config cfg`
: Device.

## Returns

A [`KalmanResult`](KalmanResult.md).

## Errors

- A missing function or Jacobian, or mis-shaped covariances, throw (`pluss::Error`).

## Practical Examples

### Example 1

A mild nonlinearity: the filter tracks the level much better than inverting the observation date by date.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::EkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  m.observation_jacobian = [&](const Series &s, std::int64_t) { return (s * 0.4 + 1.0).reshape({1, 1}); };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  const auto res = flt::ekf_filter(y, m, cfg);
  out.writeln("rms error of the filtered level:", (res.filtered_mean.reshape({T}) - x).square().mean().sqrt().item<double>());
  out.writeln("rms error of the raw observation as a level estimate:", (y - x).square().mean().sqrt().item<double>());
}
```

Output

```text
rms error of the filtered level: 0.0705
rms error of the raw observation as a level estimate: 0.4701
```

### Example 2

On a linear model it equals the Kalman filter: the same `KalmanResult` to rounding.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

static double bs(fin::pricing::OptionType type, double S, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price.items<double>()[0];
}
static pluss::Series bsv(fin::pricing::OptionType type, const pluss::Series &spots, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static pluss::Series bst(fin::pricing::OptionType type, const pluss::Series &spots, const pluss::Series &tau, double K, double r, double q, double vol) {
  fin::module::Field f{.instrument = {.type = type, .strike = pluss::make_series({K}), .maturity = tau}, .market = {.spot = spots, .rate = pluss::make_series({r}), .dividend_yield = pluss::make_series({q})},
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.price().price;
}
static fin::hedging::DeltaHedgeResult hedge(const pluss::Series &path, fin::pricing::OptionType type, double K, double r, double q, double T, double vol) {
  fin::module::Field f{.instrument = fin::instrument::make(type, K, T), .market = fin::market::make(100.0, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({vol})},
                       .controls = {.engine = fin::module::Engine::Analytic}};
  return fin::hedging::delta_hedge(f, path);
}
static double iv(double price, double S, double K, double r, double q, double T, const fin::pricing::ImpliedVolOptions &o = {}) {
  fin::module::Field f{.instrument = fin::instrument::make(fin::pricing::Call, K, T), .market = fin::market::make(S, r, q),
                       .model = fin::model::BlackScholes{.volatility = pluss::make_series({0.2})},
                       .controls = {.engine = fin::module::Engine::Analytic, .risk = fin::module::Risk::PriceOnly}};
  return f.implied_vol(pluss::make_series({price}), o).items<double>()[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 100;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  flt::EkfModel e;
  e.transition = [&](const Series &s, std::int64_t) { return s; };
  e.observation = [&](const Series &s, std::int64_t) { return s; };
  e.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  e.observation_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  e.process_cov = km.process_cov; e.observation_cov = km.observation_cov; e.initial_mean = km.initial_mean; e.initial_cov = km.initial_cov;
  out.writeln("largest difference in the filtered mean:", (flt::ekf_filter(y, e, cfg).filtered_mean - flt::kalman_filter(y, km, cfg).filtered_mean).abs().max().item<double>());
}
```

Output

```text
largest difference in the filtered mean: 0.000000000000
```

### Example 3

Missing observations (NaN) are predicted through.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>
#include <vector>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::EkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  m.observation_jacobian = [&](const Series &s, std::int64_t) { return (s * 0.4 + 1.0).reshape({1, 1}); };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  std::vector<double> values = y.items<double>();
  for (int i = 50; i < 60; ++i) values[i] = std::numeric_limits<double>::quiet_NaN();
  const auto res = flt::ekf_filter(pluss::make_series(values), m, cfg);
  out.writeln("observations used:", res.data_used.sum().item<double>(), "of", T, ", filtered variance at dates 49, 55, 60:", res.filtered_cov.select(0, 49).sum().item<double>(), res.filtered_cov.select(0, 55).sum().item<double>(), res.filtered_cov.select(0, 60).sum().item<double>());
}
```

Output

```text
observations used: 190.0000 of 200 , filtered variance at dates 49, 55, 60: 0.0096 0.0696 0.0156
```

### Example 4

The log-likelihood: a likelihood profile over the observation noise peaks near the true 0.01.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::EkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  m.observation_jacobian = [&](const Series &s, std::int64_t) { return (s * 0.4 + 1.0).reshape({1, 1}); };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

  for (const double noise : {0.0025, 0.005, 0.01, 0.02, 0.04}) {
    m.observation_cov = pluss::series::full({1, 1}, noise, cfg);
    out.writeln("R =", noise, ": loglik =", flt::ekf_filter(y, m, cfg).loglik);
  }
}
```

Output

```text
R = 0.003 : loglik = 2.143
R = 0.005 : loglik = 20.739
R = 0.010 : loglik = 32.607
R = 0.020 : loglik = 31.597
R = 0.040 : loglik = 14.911
```

### Example 5

A missing function is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const double q = 0.01, r = 0.01;
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + x.square() * 0.2 + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);
  flt::EkfModel m;
  m.transition = [&](const Series &s, std::int64_t) { return s; };
  m.observation = [&](const Series &s, std::int64_t) { return s + s.square() * 0.2; };
  m.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, 1.0, cfg); };
  m.observation_jacobian = [&](const Series &s, std::int64_t) { return (s * 0.4 + 1.0).reshape({1, 1}); };
  m.process_cov = pluss::series::full({1, 1}, q, cfg);
  m.observation_cov = pluss::series::full({1, 1}, r, cfg);
  m.initial_mean = pluss::series::zeros({1}, cfg);
  m.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  m.observation = nullptr;
  try {
    (void)(flt::ekf_filter(y, m, cfg));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::filtering::ekf_filter: observation function missing
```

## See Also

- [`EkfModel`](EkfModel.md): the model
- [`ukf_filter`](ukf_filter.md): without Jacobians

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="UkfOptions.html">
    <span class="label">Previous</span>
    <span class="title">UkfOptions</span>
  </a>
  <a class="next-link" href="kalman_filter.html">
    <span class="label">Next</span>
    <span class="title">kalman_filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
