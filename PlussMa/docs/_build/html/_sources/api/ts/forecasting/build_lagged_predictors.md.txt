<!-- generated from build_lagged_predictors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::build_lagged_predictors`

Builds the feature matrix of a lag plan from the predictors and the response.

## Signature

```cpp
pluss::Series ts::build_lagged_predictors(const pluss::Series& x, const pluss::Series& y, const ts::LagPlan& plan, pluss::codem::Config cfg = {});
```

## What It Does

`build_lagged_predictors` shifts the response and the predictors by the lags of the plan and stacks them as columns, non-leading predictor lags first, then response lags, then leading predictor lags. The matrix has `n + max_lag + max_lead` rows, with `NaN` wherever a lagged value does not exist; row `j` of the block for a horizon holds the features known at the origin `j − h` for the target `y_j`. The direct forecaster fits and predicts on slices of this matrix.

## Parameters

`const pluss::Series& x`
: The predictors, `n × p` (undefined when the plan uses none).

`const pluss::Series& y`
: The response, length `n` (undefined when the plan has no response lags).

`const ts::LagPlan& plan`
: The plan from `build_lag_plan`.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned arrays).

## Returns

A `Series` with `plan.feature_count` columns.

## Errors

- `x` and `y` must have equal lengths.

## Practical Examples

### Example 1

The feature matrix for one predictor and one response lag: the columns are the predictor lag and the response lag, and the first rows are `NaN`.

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
  const Data data = make_data(6);
  ts::LagPlanOptions options;
  options.response_lags = {1};
  options.predictor_lags = {{1}};
  const auto plan = ts::build_lag_plan(1, options);
  const Series features = ts::build_lagged_predictors(data.x, data.y, plan);
  std::cout << "rows " << features.sizes()[0] << ", columns " << features.sizes()[1] << "\n";
  const auto v = features.to_vec<double>();
  for (int r = 0; r < 4; ++r) std::cout << "  row " << r << ": " << std::fixed << std::setprecision(3) << v[2 * r] << "  " << v[2 * r + 1] << "\n";
}
```

Output

```text
rows 7, columns 2
  row 0: nan  nan
  row 1: -0.360  -0.288
  row 2: -0.139  -0.371
  row 3: 0.069  -0.172
```


## See Also

- [`build_lag_plan`](build_lag_plan.md)
- [`DirectForecaster`](DirectForecaster.md)
