<!-- generated from dm_test.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::dm_test`

The Diebold–Mariano test that two forecasting models have equal expected loss.

## Signature

```cpp
ts::DmResult ts::dm_test(const pluss::Series& err_a, const pluss::Series& err_b, const ts::DmOptions& options = {}, pluss::codem::Config cfg = {});
```

## What It Does

`dm_test` compares the forecast errors of two models on the same observations through the loss difference `d_t = L(e_A,t) − L(e_B,t)`; under the null its mean is zero. The statistic is the mean divided by its long-run standard error, which includes the autocorrelation of `d_t` up to lag `h − 1` for `h`-step forecasts. With `hln_adjust` (the default) the statistic carries the Harvey–Leybourne–Newbold correction and is referred to a Student t distribution with `n − 1` degrees of freedom; otherwise to a normal. A negative statistic favours the first model.

## Parameters

`const pluss::Series& err_a`
: Forecast errors of the first model.

`const pluss::Series& err_b`
: Forecast errors of the second model, on the same observations.

`const ts::DmOptions& options`
: Loss, horizon and tail; see [`DmOptions`](DmOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

## Returns

A [`DmResult`](DmResult.md).

## Errors

- The two error series must have the same length.

## Practical Examples

### Example 1

Compare the out-of-sample errors of two models with the squared loss; the second is better, so the mean loss difference is positive and the first model is rejected.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// y_t = 0.5 y_{t-1} + 1.0 x_t + 0.3 x_{t-1} + 0.3 e_t with x_t = 0.8 x_{t-1} + 2 u_t: a response driven by its own past and by an exogenous series x.
struct Data {
  Series x, y;
};
Data make_data(int n) {
  const auto e = noise(n, 0), u = noise(n, 7000);
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    const double xt = 0.8 * xp + 2.0 * u[t];
    const double yt = 0.5 * yp + 1.0 * xt + 0.3 * xp + 0.3 * e[t];
    x[t] = xt;
    y[t] = yt;
    xp = xt;
    yp = yt;
  }
  return {pluss::make_series(x, {n, 1}, f64), pluss::make_series(y, {n}, f64)};
}

int main() {
  const Data data = make_data(400);
  const auto partition = ts::TsPartition::expanding_window(400, 4);
  ts::DirectForecasterOptions a, b;
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  b.lag.predictor_lags = {{1, 2}};
  const auto pa = ts::DirectCVForecaster::fit(Series{}, data.y, partition, a).cvpredict(Series{}, data.y).reshape({400}).to_vec<double>();
  const auto pb = ts::DirectCVForecaster::fit(data.x, data.y, partition, b).cvpredict(data.x, data.y).reshape({400}).to_vec<double>();
  const auto y = data.y.to_vec<double>();
  std::vector<double> ea, eb;
  for (int t = 0; t < 400; ++t)
    if (std::isfinite(pa[t]) && std::isfinite(pb[t])) {
      ea.push_back(y[t] - pa[t]);
      eb.push_back(y[t] - pb[t]);
    }
  const auto r = ts::dm_test(pluss::make_series(ea, {static_cast<std::int64_t>(ea.size())}, f64), pluss::make_series(eb, {static_cast<std::int64_t>(eb.size())}, f64));
  std::cout << std::fixed << std::setprecision(4) << "pairs " << r.effective_n << "  mean loss difference " << r.mean_diff << "  statistic " << r.stat << "  p-value " << r.pvalue << "  reject " << std::boolalpha << r.reject << "\n";
}
```

Output

```text
pairs 320  mean loss difference 0.0212  statistic 2.1820  p-value 0.0298  reject true
```

### Example 2

The same test with the absolute loss, one-sided, and with the normal reference instead of the corrected t.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// y_t = 0.5 y_{t-1} + 1.0 x_t + 0.3 x_{t-1} + 0.3 e_t with x_t = 0.8 x_{t-1} + 2 u_t: a response driven by its own past and by an exogenous series x.
struct Data {
  Series x, y;
};
Data make_data(int n) {
  const auto e = noise(n, 0), u = noise(n, 7000);
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    const double xt = 0.8 * xp + 2.0 * u[t];
    const double yt = 0.5 * yp + 1.0 * xt + 0.3 * xp + 0.3 * e[t];
    x[t] = xt;
    y[t] = yt;
    xp = xt;
    yp = yt;
  }
  return {pluss::make_series(x, {n, 1}, f64), pluss::make_series(y, {n}, f64)};
}

int main() {
  std::vector<double> ea(60), eb(60);
  const auto e = noise(60, 0), g = noise(60, 500);
  for (int i = 0; i < 60; ++i) {
    ea[i] = 1.3 * e[i] + 0.1;
    eb[i] = g[i] + 0.5 * e[i];
  }
  const Series a = pluss::make_series(ea, {60}, f64), b = pluss::make_series(eb, {60}, f64);
  ts::DmOptions options;
  options.loss = ts::DmLoss::Absolute;
  options.tail = ts::DmTail::Right;
  options.hln_adjust = false;
  const auto r = ts::dm_test(a, b, options);
  std::cout << std::fixed << std::setprecision(4) << "statistic " << r.stat << "  p-value " << r.pvalue << "  critical " << r.critical << "\n";
}
```

Output

```text
statistic 1.0482  p-value 0.1473  critical 1.6449
```

### Example 3

A multi-step comparison: with `horizon = 3` the long-run variance includes two lags of autocovariance.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// y_t = 0.5 y_{t-1} + 1.0 x_t + 0.3 x_{t-1} + 0.3 e_t with x_t = 0.8 x_{t-1} + 2 u_t: a response driven by its own past and by an exogenous series x.
struct Data {
  Series x, y;
};
Data make_data(int n) {
  const auto e = noise(n, 0), u = noise(n, 7000);
  std::vector<double> x(n), y(n);
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    const double xt = 0.8 * xp + 2.0 * u[t];
    const double yt = 0.5 * yp + 1.0 * xt + 0.3 * xp + 0.3 * e[t];
    x[t] = xt;
    y[t] = yt;
    xp = xt;
    yp = yt;
  }
  return {pluss::make_series(x, {n, 1}, f64), pluss::make_series(y, {n}, f64)};
}

int main() {
  std::vector<double> ea(60), eb(60);
  const auto e = noise(60, 0), g = noise(60, 500);
  for (int i = 0; i < 60; ++i) {
    ea[i] = 1.3 * e[i] + 0.1;
    eb[i] = g[i] + 0.5 * e[i];
  }
  ts::DmOptions options;
  options.horizon = 3;
  const auto r = ts::dm_test(pluss::make_series(ea, {60}, f64), pluss::make_series(eb, {60}, f64), options);
  std::cout << std::fixed << std::setprecision(4) << "lags used " << r.lags << ", long-run variance " << r.lrv << ", statistic " << r.stat << "\n";
}
```

Output

```text
lags used 2, long-run variance 0.0185, statistic 2.1828
```


## See Also

- [`DmOptions`](DmOptions.md)
- [`DmResult`](DmResult.md)
- [`DirectCVForecaster`](DirectCVForecaster.md)
