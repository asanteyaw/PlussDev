<!-- generated from forecasting.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# ts forecasting

`forecasting` evaluates and builds forecasts without a model of the series' own dynamics: `TsPartition` splits a series into training and test blocks that respect time order; `DirectForecaster` fits one regression per forecast horizon from lagged values of the series and of exogenous predictors; `DirectCVForecaster` fits one such forecaster per training window and measures out-of-sample loss; `dm_test` compares two sets of forecast errors with the Diebold–Mariano test.

A direct forecaster predicts `y_{t+h}` straight from information available at time `t`, so it needs no model of how its predictors evolve and can use exogenous series whose future values are known.

## The Surface

| Entity | What it gives you |
|---|---|
| [`build_lag_plan`](forecasting/build_lag_plan.md) | Normalises a lag specification into a plan for a number of predictor columns |
| [`build_lagged_predictors`](forecasting/build_lagged_predictors.md) | Builds the feature matrix of a lag plan from the predictors and the response |
| [`DirectCVForecaster`](forecasting/DirectCVForecaster.md) | One direct forecaster per training window of a partition, for out-of-sample evaluation |
| [`DirectForecaster`](forecasting/DirectForecaster.md) | A set of regressions, one per forecast horizon, that predict `y_{t+h}` from lagged information at time `t` |
| [`DirectForecasterOptions`](forecasting/DirectForecasterOptions.md) | Options of `DirectForecaster::fit` and `DirectCVForecaster::fit` |
| [`DirectStepModel`](forecasting/DirectStepModel.md) | The model fitted for one horizon of a direct forecaster |
| [`dm_test`](forecasting/dm_test.md) | The Diebold–Mariano test that two forecasting models have equal expected loss |
| [`DmOptions`](forecasting/DmOptions.md) | Options of `ts::dm_test` |
| [`DmResult`](forecasting/DmResult.md) | Result of `ts::dm_test` |
| [`LagPlan`](forecasting/LagPlan.md) | A normalised lag plan: the result of `build_lag_plan` |
| [`LagPlanOptions`](forecasting/LagPlanOptions.md) | Options describing which lagged features a direct forecaster uses |
| [`PartitionOptions`](forecasting/PartitionOptions.md) | Options of the `TsPartition` constructors |
| [`PartitionWindow`](forecasting/PartitionWindow.md) | One training/test window of a `TsPartition` |
| [`TsPartition`](forecasting/TsPartition.md) | Splits a series into consecutive training and test windows for honest forecast evaluation |

## How To Call It

Split 100 observations into three expanding training windows, each followed by a test window, and print the windows.

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
  const auto partition = ts::TsPartition::expanding_window(100, 3);
  for (const auto& w : partition.windows())
    std::cout << "train " << w.train_start + 1 << "-" << w.train_end + 1 << "   test " << w.test_start + 1 << "-" << w.test_end + 1 << "\n";
}
```

Output

```text
train 1-25   test 26-50
train 1-50   test 51-75
train 1-75   test 76-100
```

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

forecasting/build_lag_plan
forecasting/build_lagged_predictors
forecasting/DirectCVForecaster
forecasting/DirectForecaster
forecasting/DirectForecasterOptions
forecasting/DirectStepModel
forecasting/dm_test
forecasting/DmOptions
forecasting/DmResult
forecasting/LagPlan
forecasting/LagPlanOptions
forecasting/PartitionOptions
forecasting/PartitionWindow
forecasting/TsPartition
```
<!-- apiref:toctree:end -->
