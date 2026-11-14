<!-- generated from Ssm.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::Ssm`

Filtering, smoothing, forecasting and simulation for a linear Gaussian state-space model.

## Signature

```cpp
ts::SsmFilterResult ts::Ssm::filter(const pluss::Series& y, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
ts::SsmSmoothResult ts::Ssm::smooth(const pluss::Series& y, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
ts::SsmForecastResult ts::Ssm::forecast(std::int64_t horizon, const ts::SsmModel& model, pluss::codem::Config cfg = {});
ts::SsmForecastResult ts::Ssm::forecast(std::int64_t horizon, const pluss::Series& y_history, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
ts::SsmSimResult ts::Ssm::simulate(std::int64_t num_observations, const ts::SsmModel& model, std::int64_t num_paths = 1, pluss::codem::Config cfg = {}, std::uint64_t seed = 0);
```

## Construction

`Ssm` has no instances: it is a namespace of static functions that act on an [`SsmModel`](SsmModel.md).

## Methods

### `filter`

```cpp
ts::SsmFilterResult ts::Ssm::filter(const pluss::Series& y, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
```

The Kalman filter: runs through the observations in time order, at each step predicting the state, comparing the prediction of the observation with the observation, and correcting the state estimate by the Kalman gain times the surprise. Missing observations (`NaN`) are skipped: the state prediction is kept and its uncertainty grows until data return. The filter also accumulates the exact Gaussian log-likelihood of the data, which is the objective for estimating unknown noise levels.

`const pluss::Series& y`
: The observations as a `T × n` matrix (a `T × 1` matrix for one series); missing values are `NaN`.

`const ts::SsmModel& model`
: The model; see [`SsmModel`](SsmModel.md).

`const ts::SsmOptions& options`
: Settings; see [`SsmOptions`](SsmOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

**Returns.** An [`SsmFilterResult`](SsmFilterResult.md).

**Errors.** The matrices must have conforming shapes.

### `smooth`

```cpp
ts::SsmSmoothResult ts::Ssm::smooth(const pluss::Series& y, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
```

The Rauch–Tung–Striebel smoother: after the forward filter, a backward pass re-estimates every state using *all* the observations, past and future. Smoothed estimates are more precise than filtered ones, and inside a gap of missing observations they interpolate; use filtered states for real-time decisions and smoothed states for looking back.

`const pluss::Series& y`
: The observations as a `T × n` matrix (a `T × 1` matrix for one series); missing values are `NaN`.

`const ts::SsmModel& model`
: The model.

`const ts::SsmOptions& options`
: Settings.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

**Returns.** An [`SsmSmoothResult`](SsmSmoothResult.md): the filter result plus the smoothed states and their covariances.

**Errors.** The matrices must have conforming shapes.

### `forecast`

```cpp
ts::SsmForecastResult ts::Ssm::forecast(std::int64_t horizon, const ts::SsmModel& model, pluss::codem::Config cfg = {});
ts::SsmForecastResult ts::Ssm::forecast(std::int64_t horizon, const pluss::Series& y_history, const ts::SsmModel& model, const ts::SsmOptions& options = {}, pluss::codem::Config cfg = {});
```

Forecasts the observations and states `horizon` steps ahead with their covariances. With a history, the filter is run over it first and the forecast continues from the last filtered state; without one, the forecast starts from the model's initial state alone (so its covariance is the model's prior).

`std::int64_t horizon`
: Number of steps (greater than 0).

`const pluss::Series& y_history`
: Observations to filter first (optional form).

`const ts::SsmModel& model`
: The model.

`const ts::SsmOptions& options`
: Settings.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

**Returns.** An [`SsmForecastResult`](SsmForecastResult.md).

**Errors.** `horizon` must be positive.

### `simulate`

```cpp
ts::SsmSimResult ts::Ssm::simulate(std::int64_t num_observations, const ts::SsmModel& model, std::int64_t num_paths = 1, pluss::codem::Config cfg = {}, std::uint64_t seed = 0);
```

Draws paths of states and observations from the model, starting from the initial state distribution. With `num_paths > 1` every output gains a leading path dimension. The same `seed` gives the same paths. The variance of simulated observations across many paths equals the model's predicted variance, which is a check on a newly written model.

`std::int64_t num_observations`
: Length of each path.

`const ts::SsmModel& model`
: The model.

`std::int64_t num_paths`
: Number of independent paths (default 1).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

`std::uint64_t seed`
: Seed (default 0).

**Returns.** An [`SsmSimResult`](SsmSimResult.md).

## Practical Examples

### Example 1

Filter a noisy rising series with several observation-noise levels: the log-likelihood is highest near the noise level that generated the data (0.29) and falls when the noise is set too small or too large, which is how unknown noise levels are chosen.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + e_t.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(n);
  for (int t = 0; t < n; ++t) y[t] = 1.0 + 0.3 * t + e[t];
  return y;
}
// Local linear trend: the state is (level, slope); level' = level + slope + level_sd w1, slope' = slope + slope_sd w2, y = level + obs_sd v.
ts::SsmModel trend_model(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = pluss::make_series({1.0, 1.0, 0.0, 1.0}, {2, 2}, f64);
  m.state_loading = pluss::make_series({level_sd, 0.0, 0.0, slope_sd}, {2, 2}, f64);
  m.observation = pluss::make_series({1.0, 0.0}, {1, 2}, f64);
  m.innovation_loading = pluss::make_series({obs_sd}, {1, 1}, f64);
  m.mean0 = pluss::make_series({0.0, 0.0}, {2}, f64);
  m.cov0 = pluss::make_series({25.0, 0.0, 0.0, 1.0}, {2, 2}, f64);
  return m;
}

int main() {
  const Series y = pluss::make_series(trending(60), {60, 1}, f64);
  for (double sd : {0.05, 0.1, 0.2, 0.3, 0.4, 0.6, 1.0})
    std::cout << "observation sd " << std::fixed << std::setprecision(2) << sd << "   log-likelihood " << std::setprecision(3) << ts::Ssm::filter(y, trend_model(0.2, 0.05, sd)).loglik << "\n";
}
```

Output

```text
observation sd 0.05   log-likelihood -79.308
observation sd 0.10   log-likelihood -55.179
observation sd 0.20   log-likelihood -33.231
observation sd 0.30   log-likelihood -32.005
observation sd 0.40   log-likelihood -36.827
observation sd 0.60   log-likelihood -49.608
observation sd 1.00   log-likelihood -71.894
```

### Example 2

Smooth across a gap of missing observations (marked NaN): the filter skips them, the smoother interpolates, and its uncertainty is largest in the middle of the gap.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + e_t.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(n);
  for (int t = 0; t < n; ++t) y[t] = 1.0 + 0.3 * t + e[t];
  return y;
}
// Local linear trend: the state is (level, slope); level' = level + slope + level_sd w1, slope' = slope + slope_sd w2, y = level + obs_sd v.
ts::SsmModel trend_model(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = pluss::make_series({1.0, 1.0, 0.0, 1.0}, {2, 2}, f64);
  m.state_loading = pluss::make_series({level_sd, 0.0, 0.0, slope_sd}, {2, 2}, f64);
  m.observation = pluss::make_series({1.0, 0.0}, {1, 2}, f64);
  m.innovation_loading = pluss::make_series({obs_sd}, {1, 1}, f64);
  m.mean0 = pluss::make_series({0.0, 0.0}, {2}, f64);
  m.cov0 = pluss::make_series({25.0, 0.0, 0.0, 1.0}, {2, 2}, f64);
  return m;
}

int main() {
  auto data = trending(40);
  for (int t = 15; t < 20; ++t) data[t] = std::nan("");
  const Series y = pluss::make_series(data, {40, 1}, f64);
  const auto s = ts::Ssm::smooth(y, trend_model(0.2, 0.05, 0.3));
  std::cout << std::fixed << std::setprecision(3) << "   t   used   filtered   smoothed   smoothed sd   truth\n";
  for (int t = 13; t < 22; ++t) {
    const double sd = std::sqrt(s.smoothed_states_cov.select(0, t).select(0, 0).select(0, 0).squeeze().item<double>());
    std::cout << std::setw(4) << t << std::setw(7) << (s.filter.data_used.select(0, t).squeeze().item<double>() > 0 ? "yes" : "no") << std::setw(11) << s.filter.filtered_states.select(0, t).select(0, 0).squeeze().item<double>() << std::setw(11)
              << s.smoothed_states.select(0, t).select(0, 0).squeeze().item<double>() << std::setw(13) << sd << std::setw(9) << 1.0 + 0.3 * t << "\n";
  }
}
```

Output

```text
   t   used   filtered   smoothed   smoothed sd   truth
  13    yes      5.078      4.980        0.180    4.900
  14    yes      5.196      5.215        0.202    5.200
  15     no      5.489      5.528        0.267    5.500
  16     no      5.783      5.842        0.302    5.800
  17     no      6.076      6.155        0.313    6.100
  18     no      6.369      6.468        0.302    6.400
  19     no      6.663      6.779        0.267    6.700
  20    yes      7.320      7.087        0.202    7.000
  21    yes      7.140      7.275        0.180    7.300
```

### Example 3

Forecast five steps with prediction intervals; the intervals widen with the horizon.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + e_t.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(n);
  for (int t = 0; t < n; ++t) y[t] = 1.0 + 0.3 * t + e[t];
  return y;
}
// Local linear trend: the state is (level, slope); level' = level + slope + level_sd w1, slope' = slope + slope_sd w2, y = level + obs_sd v.
ts::SsmModel trend_model(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = pluss::make_series({1.0, 1.0, 0.0, 1.0}, {2, 2}, f64);
  m.state_loading = pluss::make_series({level_sd, 0.0, 0.0, slope_sd}, {2, 2}, f64);
  m.observation = pluss::make_series({1.0, 0.0}, {1, 2}, f64);
  m.innovation_loading = pluss::make_series({obs_sd}, {1, 1}, f64);
  m.mean0 = pluss::make_series({0.0, 0.0}, {2}, f64);
  m.cov0 = pluss::make_series({25.0, 0.0, 0.0, 1.0}, {2, 2}, f64);
  return m;
}

int main() {
  const Series y = pluss::make_series(trending(40), {40, 1}, f64);
  const auto f = ts::Ssm::forecast(5, y, trend_model(0.2, 0.05, 0.3));
  for (int h = 0; h < 5; ++h)
    std::cout << std::fixed << std::setprecision(3) << "t = " << 40 + h << "   " << f.observations.select(0, h).squeeze().item<double>() << "  +/- " << 1.96 * std::sqrt(f.observations_cov.select(0, h).select(0, 0).select(0, 0).squeeze().item<double>()) << "   truth " << 1.0 + 0.3 * (40 + h) << "\n";
}
```

Output

```text
t = 40   13.115  +/- 0.909   truth 13.000
t = 41   13.438  +/- 1.105   truth 13.300
t = 42   13.761  +/- 1.321   truth 13.600
t = 43   14.084  +/- 1.557   truth 13.900
t = 44   14.406  +/- 1.809   truth 14.200
```

### Example 4

Simulation as a check: the variance of observations across 20000 paths agrees with the variance the model predicts from its initial state.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + e_t.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(n);
  for (int t = 0; t < n; ++t) y[t] = 1.0 + 0.3 * t + e[t];
  return y;
}
// Local linear trend: the state is (level, slope); level' = level + slope + level_sd w1, slope' = slope + slope_sd w2, y = level + obs_sd v.
ts::SsmModel trend_model(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = pluss::make_series({1.0, 1.0, 0.0, 1.0}, {2, 2}, f64);
  m.state_loading = pluss::make_series({level_sd, 0.0, 0.0, slope_sd}, {2, 2}, f64);
  m.observation = pluss::make_series({1.0, 0.0}, {1, 2}, f64);
  m.innovation_loading = pluss::make_series({obs_sd}, {1, 1}, f64);
  m.mean0 = pluss::make_series({0.0, 0.0}, {2}, f64);
  m.cov0 = pluss::make_series({25.0, 0.0, 0.0, 1.0}, {2, 2}, f64);
  return m;
}

int main() {
  ts::SsmModel m;
  m.transition = pluss::make_series({1.0}, {1, 1}, f64);
  m.state_loading = pluss::make_series({0.5}, {1, 1}, f64);
  m.observation = pluss::make_series({1.0}, {1, 1}, f64);
  m.innovation_loading = pluss::make_series({0.3}, {1, 1}, f64);
  m.mean0 = pluss::make_series({0.0}, {1}, f64);
  m.cov0 = pluss::make_series({1.0}, {1, 1}, f64);
  const auto sim = ts::Ssm::simulate(5, m, 20000, f64, 20261022ULL);
  const auto predicted = ts::Ssm::forecast(5, m);
  std::cout << std::fixed << std::setprecision(3) << "time   simulated   predicted\n";
  for (int t = 0; t < 5; ++t) {
    const Series x = sim.observations.select(1, t).reshape({20000});
    const double variance = (x * x).mean().item<double>() - x.mean().item<double>() * x.mean().item<double>();
    std::cout << std::setw(4) << t << std::setw(12) << variance << std::setw(12) << predicted.observations_cov.select(0, t).select(0, 0).select(0, 0).squeeze().item<double>() << "\n";
  }
}
```

Output

```text
time   simulated   predicted
   0       1.341       1.340
   1       1.601       1.590
   2       1.871       1.840
   3       2.116       2.090
   4       2.352       2.340
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`SsmModel`](SsmModel.md)
- [`SsmFilterResult`](SsmFilterResult.md)
- [`SsmSmoothResult`](SsmSmoothResult.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="SsmFilterResult.html">
    <span class="label">Next</span>
    <span class="title">SsmFilterResult</span>
  </a>
</div>
<!-- apiref:nav:end -->

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
