<!-- generated from KalmanModel.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `filtering::KalmanModel`

A linear Gaussian state-space model: the matrices of the dynamics and the observation, the noise covariances and the initial state.

`x_t = F x_{t−1} + G u_t + w_t`, `w_t ~ N(0, Q)`; `y_t = H x_t + v_t`, `v_t ~ N(0, R)`; `x_0 ~ N(m₀, P₀)` before the first observation. `n` is the state dimension and `m` the number of observed channels. The initial mean and covariance describe the state *before* the first observation, so the first step is a prediction. Every member is a `Series`; the shapes are in the field descriptions.

## Fields

### `transition`

`pluss::Series`, default `undefined`.

`F`, `[n, n]`.

### `observation`

`pluss::Series`, default `undefined`.

`H`, `[m, n]`.

### `process_cov`

`pluss::Series`, default `undefined`.

`Q`, `[n, n]`, positive semi-definite.

### `observation_cov`

`pluss::Series`, default `undefined`.

`R`, `[m, m]`, positive definite.

### `initial_mean`

`pluss::Series`, default `undefined`.

`m₀`, `[n]` or `[n, 1]`: the state before the first observation.

### `initial_cov`

`pluss::Series`, default `undefined`.

`P₀`, `[n, n]`.

### `control`

`pluss::Series`, default `undefined`.

Optional known inputs `uₜ`, `[T, k]`.

### `control_matrix`

`pluss::Series`, default `undefined`.

Optional `G`, `[n, k]`.

### `info_tol`

`double`, default `0.0`.

An observation channel whose forecast variance is at most this is ignored.

## Guidance

Passed to `kalman_filter`.

## Practical Examples

### Example 1

The transition `F`. With a coefficient of 0.9 the state is mean reverting, and with no observations at all (every value missing) the predicted mean decays like `0.9ᵗ`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <limits>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 0.9, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, 0.01, cfg);
  km.observation_cov = pluss::series::full({1, 1}, 0.04, cfg);
  km.initial_mean = pluss::series::full({1}, 1.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 0.0, cfg);
  const Series nothing = pluss::series::full({5}, std::numeric_limits<double>::quiet_NaN(), cfg);
  const auto res = flt::kalman_filter(nothing, km, cfg);
  out.writeln("predicted mean:", res.predicted_mean.reshape({5}).items<double>(), " (0.9^t)");
}
```

Output

```text
predicted mean: [0.90000, 0.81000, 0.72900, 0.65610, 0.59049]  (0.9^t)
```

### Example 2

The observation matrix `H`. With `H = 2` an observation of 4 means a state near 2: the filtered mean follows `y/H` when the observation is trusted.

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
  const double q = 0.0001, r = 0.0001;
  const std::int64_t T = 40;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 2.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, 1e-4, cfg);
  km.observation_cov = pluss::series::full({1, 1}, 1e-4, cfg);
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);
  const Series scaled = pluss::series::full({40}, 4.0, cfg);
  out.writeln("filtered mean after 40 observations of 4:", flt::kalman_filter(scaled, km, cfg).filtered_mean.select(0, 39).items<double>());
}
```

Output

```text
filtered mean after 40 observations of 4: [2.0000]
```

### Example 3

The process covariance `Q`: the more the state is expected to move, the more the filter trusts each observation, so the filtered variance rises with `Q`.

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
  const std::int64_t T = 300;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  for (const double process : {0.001, 0.01, 0.1}) {
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, process, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

    out.writeln("Q =", process, ": filtered variance at the last date =", flt::kalman_filter(y, km, cfg).filtered_cov.select(0, T - 1).sum().item<double>());
  }
}
```

Output

```text
Q = 0.00100 : filtered variance at the last date = 0.00584
Q = 0.01000 : filtered variance at the last date = 0.01562
Q = 0.10000 : filtered variance at the last date = 0.03062
```

### Example 4

The observation covariance `R`: noisier observations are believed less, so the filtered variance rises with `R`.

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
  const std::int64_t T = 300;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  for (const double noise : {0.01, 0.04, 0.16}) {
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, noise, cfg);
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 1.0, cfg);

    out.writeln("R =", noise, ": filtered variance at the last date =", flt::kalman_filter(y, km, cfg).filtered_cov.select(0, T - 1).sum().item<double>());
  }
}
```

Output

```text
R = 0.01000 : filtered variance at the last date = 0.00618
R = 0.04000 : filtered variance at the last date = 0.01562
R = 0.16000 : filtered variance at the last date = 0.03531
```

### Example 5

The initial mean and covariance: the prediction for the first date is `F m₀` with variance `F P₀ Fᵀ + Q`.

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
  const double q = 0.01, r = 0.04;
  const std::int64_t T = 300;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const Series x = (pluss::series::randn({T}, cfg, 1) * std::sqrt(q)).cumsum(0, false);
  const Series y = x + pluss::series::randn({T}, cfg, 2) * std::sqrt(r);

  for (const double p0 : {0.0, 1.0, 10.0}) {
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, q, cfg);
  km.observation_cov = pluss::series::full({1, 1}, r, cfg);
  km.initial_mean = pluss::series::full({1}, 2.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, p0, cfg);

    const auto res = flt::kalman_filter(y, km, cfg);
    out.writeln("P0 =", p0, ": first predicted mean", res.predicted_mean.select(0, 0).sum().item<double>(), ", first predicted variance", res.predicted_cov.select(0, 0).sum().item<double>(), "(= P0 + q)");
  }
}
```

Output

```text
P0 = 0.0000 : first predicted mean 2.0000 , first predicted variance 0.0100 (= P0 + q)
P0 = 1.0000 : first predicted mean 2.0000 , first predicted variance 1.0100 (= P0 + q)
P0 = 10.0000 : first predicted mean 2.0000 , first predicted variance 10.0100 (= P0 + q)
```

### Example 6

The control `u` and its matrix `G`. A known input of 1 each date through `G = 0.5` moves the state by 0.5 per date; with a very small noise the filtered mean (observed zero) follows the balance of input and observation.

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
  flt::KalmanModel km;
  km.transition = pluss::series::full({1, 1}, 1.0, cfg);
  km.observation = pluss::series::full({1, 1}, 1.0, cfg);
  km.process_cov = pluss::series::full({1, 1}, 1e-8, cfg);
  km.observation_cov = pluss::series::full({1, 1}, 1e8, cfg);   // observations carry no information
  km.initial_mean = pluss::series::full({1}, 0.0, cfg);
  km.initial_cov = pluss::series::full({1, 1}, 0.0, cfg);
  km.control = pluss::series::ones({6, 1}, cfg);
  km.control_matrix = pluss::series::full({1, 1}, 0.5, cfg);
  const auto res = flt::kalman_filter(pluss::series::zeros({6}, cfg), km, cfg);
  out.writeln("filtered mean:", res.filtered_mean.reshape({6}).items<double>(), "(0.5 per date)");
}
```

Output

```text
filtered mean: [0.5000, 1.0000, 1.5000, 2.0000, 2.5000, 3.0000] (0.5 per date)
```

### Example 7

The information tolerance. A channel whose forecast variance is at most `info_tol` is ignored (a perfectly known channel carries no news). Here a tolerance larger than every forecast variance makes the filter skip all observations: `data_used` is zero.

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
  out.print_defaults(PrintOptions{}.fixed(0));
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

  for (const double tol : {0.0, 100.0}) {
    km.info_tol = tol;
    out.writeln("info_tol", tol, ": observations used =", flt::kalman_filter(y, km, cfg).data_used.sum().sum().item<double>(), "of", T);
  }
}
```

Output

```text
info_tol 0 : observations used = 10 of 10
info_tol 100 : observations used = 0 of 10
```

## See Also

- [`kalman_filter`](kalman_filter.md): uses it
- [`KalmanResult`](KalmanResult.md): what it returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="EkfModel.html">
    <span class="label">Previous</span>
    <span class="title">EkfModel</span>
  </a>
  <a class="next-link" href="KalmanResult.html">
    <span class="label">Next</span>
    <span class="title">KalmanResult</span>
  </a>
</div>
<!-- apiref:nav:end -->
