<!-- generated from build_lag_plan.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::build_lag_plan`

Normalises a lag specification into a plan for a number of predictor columns.

## Signature

```cpp
ts::LagPlan ts::build_lag_plan(std::int64_t num_predictors, const ts::LagPlanOptions& options);
```

## What It Does

`build_lag_plan` validates a [`LagPlanOptions`](LagPlanOptions.md) against the number of predictor columns, broadcasts a single list of predictor lags to every non-leading predictor, and records which feature columns exist for each horizon. It is what the direct forecaster does first; call it directly to inspect how many features a specification produces.

## Parameters

`std::int64_t num_predictors`
: The number of exogenous predictor columns (0 for none).

`const ts::LagPlanOptions& options`
: The lag specification.

## Returns

A [`LagPlan`](LagPlan.md).

## Errors

- Lists that do not match the number of predictors are rejected.
- Horizons must be positive.

## Practical Examples

### Example 1

The number of features for three predictors under the defaults (lag 1 of each and of the response), with a shared lag list, and with one list per predictor.

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
  ts::LagPlanOptions defaults;
  std::cout << "defaults: " << ts::build_lag_plan(3, defaults).feature_count << " features\n";
  ts::LagPlanOptions shared;
  shared.predictor_lags = {{1, 2}};
  std::cout << "lags {1,2} for every predictor: " << ts::build_lag_plan(3, shared).feature_count << " features\n";
  ts::LagPlanOptions each;
  each.predictor_lags = {{1}, {2}, {3}};
  std::cout << "one list per predictor: " << ts::build_lag_plan(3, each).feature_count << " features\n";
}
```

Output

```text
defaults: 4 features
lags {1,2} for every predictor: 7 features
one list per predictor: 4 features
```

### Example 2

A leading predictor with lags 0 and 1 for horizons 1 and 2.

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
  ts::LagPlanOptions options;
  options.horizon = {1, 2};
  options.response_lags = {1};
  options.leading_predictors = {0};
  options.leading_predictor_lags = {{0, 1}};
  const auto plan = ts::build_lag_plan(2, options);
  std::cout << "features " << plan.feature_count << ", longest lag " << plan.max_lag << ", steps looked ahead " << plan.max_lead << "\n";
}
```

Output

```text
features 5, longest lag 1, steps looked ahead 1
```


## See Also

- [`LagPlanOptions`](LagPlanOptions.md)
- [`build_lagged_predictors`](build_lagged_predictors.md)
