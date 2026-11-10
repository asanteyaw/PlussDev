<!-- generated from kalman_filter.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::kalman_filter`

The Kalman filter: the exact posterior of a linear Gaussian state-space model, date by date.

## Signature

```cpp
filtering::KalmanResult filtering::kalman_filter(const pluss::Series &observations, const filtering::KalmanModel &model, pluss::Config cfg = {});
```

## What It Does

For each date the filter predicts, `x̂_{t|t−1} = F x̂_{t−1} + G uₜ`, `P_{t|t−1} = F P_{t−1} Fᵀ + Q`; compares with the observation, `eₜ = yₜ − H x̂_{t|t−1}`, `Sₜ = H P_{t|t−1} Hᵀ + R`; and corrects with the gain `Kₜ = P_{t|t−1} Hᵀ Sₜ⁻¹`: `x̂ₜ = x̂_{t|t−1} + Kₜ eₜ`, `Pₜ = (I − Kₜ H) P_{t|t−1}`. For a linear Gaussian model it is exact: the filtered mean is the conditional expectation of the state given the observations so far. The observations are `[T]` (one channel) or `[T, m]`; a **NaN** marks a missing value, which is skipped (the filter predicts through it). The result carries every intermediate quantity and the log-likelihood. See [`KalmanModel`](KalmanModel.md) and [`KalmanResult`](KalmanResult.md).

## Parameters

`const pluss::Series &observations`
: `[T]` or `[T, m]`; NaN marks a missing value.

`const filtering::KalmanModel &model`
: The state-space model.

`pluss::Config cfg`
: Device; computed in Float64.

## Returns

A [`KalmanResult`](KalmanResult.md).

## Errors

- Undefined or mis-shaped model members, or observations of rank above 2, throw (`pluss::Error`).

## Practical Examples

### Example 1

A local level: a random-walk state observed with noise. After a few dates the filter reaches a steady state with prediction variance `P = ½(q + √(q² + 4qr))`, gain `K = P/(P + r)`, and filtered variance `K r`.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 400;
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

  const auto res = flt::kalman_filter(y, km, cfg);
  const double Pp = 0.5 * (q + std::sqrt(q * q + 4.0 * q * r)), K = Pp / (Pp + r);
  out.writeln("predicted variance:", res.predicted_cov.select(0, T - 1).sum().item<double>(), " theory", Pp);
  out.writeln("filtered variance :", res.filtered_cov.select(0, T - 1).sum().item<double>(), " theory", K * r);
  out.writeln("rms error of the filtered level:", (res.filtered_mean.reshape({T}) - x).square().mean().sqrt().sum().item<double>(), " vs raw observation", (y - x).square().mean().sqrt().sum().item<double>());
}
```

Output

```text
predicted variance: 0.02562  theory 0.02562
filtered variance : 0.01562  theory 0.01562
rms error of the filtered level: 0.13518  vs raw observation 0.20599
```

### Example 2

Missing data: a NaN is predicted through, and the variance grows while the data are missing.

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
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 20;
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

  std::vector<double> values = y.items<double>();
  for (int i = 8; i < 14; ++i) values[i] = std::numeric_limits<double>::quiet_NaN();
  const auto res = flt::kalman_filter(pluss::make_series(values), km, cfg);
  out.writeln("filtered variance by date:", res.filtered_cov.reshape({20}).items<double>());
}
```

Output

```text
filtered variance by date: [0.0385, 0.0219, 0.0178, 0.0164, 0.0159, 0.0157, 0.0157, 0.0156, 0.0256, 0.0356, 0.0456, 0.0556, 0.0656, 0.0756, 0.0273, 0.0193, 0.0169, 0.0161, 0.0158, 0.0157]
```

### Example 3

Two observed channels of one state: both are used each date and the filtered variance is smaller than with either alone.

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
  out.print_defaults(PrintOptions{}.fixed(5));
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 200;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  const Series y2 = x + pluss::series::randn({T}, cfg, 3) * std::sqrt(r);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::ones({2, 1}, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::eye(2, cfg) * r;
  km.initial_mean = pluss::series::zeros({1}, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const Series both = y.unsqueeze(1).unite({y2.unsqueeze(1)}, 1);
  out.writeln("both channels used:", flt::kalman_filter(both, km, cfg).data_used.sum().sum().item<double>(), "of", 2 * T);
  out.writeln("filtered variance (two channels):", flt::kalman_filter(both, km, cfg).filtered_cov.select(0, T - 1).sum().item<double>());
}
```

Output

```text
both channels used: 400.00000 of 400
filtered variance (two channels): 0.01000
```

### Example 4

A trend: a two-state model (level and slope) observed through the level only. The filter infers the slope that is never observed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 200;
  const Series time = pluss::series::arange(0.0, 200.0, 1.0, cfg);
  const Series y = time * 0.05 + pluss::series::randn({T}, cfg, 4) * 0.5;
  flt::KalmanModel km;
  km.transition = pluss::make_series({1.0, 1.0, 0.0, 1.0}, {2, 2});
  km.observation = pluss::make_series({1.0, 0.0}, {1, 2});
  km.process_cov = pluss::make_series({1e-4, 0.0, 0.0, 1e-6}, {2, 2});
  km.observation_cov = pluss::series::full({1, 1}, 0.25, cfg);
  km.initial_mean = pluss::make_series({0.0, 0.0});
  km.initial_cov = pluss::make_series({1.0, 0.0, 0.0, 1.0}, {2, 2});
  const auto res = flt::kalman_filter(y, km, cfg);
  out.writeln("filtered level and slope at the end:", res.filtered_mean.select(0, T - 1).items<double>(), "(true 9.95, 0.05)");
}
```

Output

```text
filtered level and slope at the end: [10.0605, 0.0536] (true 9.95, 0.05)
```

### Example 5

Exogenous inputs: the model has a known input; here it carries a known drift of 0.1 per date that the filter would otherwise mistake for noise.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const std::int64_t T = 100;
  const Series y = pluss::series::arange(0.0, 100.0, 1.0, cfg) * 0.1 + pluss::series::randn({T}, cfg, 5) * 0.3;
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, 0.0001, cfg);
  km.observation_cov = pluss::series::full({1, 1}, 0.09, cfg);
  km.initial_mean = pluss::series::zeros({1}, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 0.01, cfg);
  const double plain = flt::kalman_filter(y, km, cfg).loglik;
  km.control = pluss::series::ones({T, 1}, cfg);
  km.control_matrix = pluss::series::full({1, 1}, 0.1, cfg);
  out.writeln("log-likelihood without the input:", plain, ", with the known drift:", flt::kalman_filter(y, km, cfg).loglik);
}
```

Output

```text
log-likelihood without the input: -2570.146 , with the known drift: -12.605
```

### Example 6

Invalid input: a covariance of the wrong shape is refused.

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
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 10;
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

  km.process_cov = pluss::series::eye(2, cfg);
  try {
    (void)(flt::kalman_filter(y, km, cfg));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
math::state: process_cov must have shape [..., n, n]
```

## See Also

- [`KalmanModel`](KalmanModel.md): the model
- [`KalmanResult`](KalmanResult.md): the result
- [`ekf_filter`](ekf_filter.md): for nonlinear models

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ekf_filter.html">
    <span class="label">Previous</span>
    <span class="title">ekf_filter</span>
  </a>
  <a class="next-link" href="particle_filter.html">
    <span class="label">Next</span>
    <span class="title">particle_filter</span>
  </a>
</div>
<!-- apiref:nav:end -->
