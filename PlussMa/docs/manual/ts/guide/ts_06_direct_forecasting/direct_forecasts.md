<!-- generated from direct_forecasts.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Direct forecasts

[Back to Direct Forecasting and Model Comparison](../ts_06_direct_forecasting.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 (t + offset) + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n, int offset = 0) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// Response y driven by its own past and by an exogenous series x that is known in advance:  y_t = 0.5 y_{t-1} + 1.0 x_t + 0.3 x_{t-1} + 0.3 e_t,  x_t = 0.8 x_{t-1} + u_t.
struct Data {
  Series x, y;
};
Data make_data(int n) {
  const auto e = noise(n, 0), u = noise(n, 7000);
  std::vector<double> x(static_cast<std::size_t>(n)), y(static_cast<std::size_t>(n));
  double xp = 0.0, yp = 0.0;
  for (int t = 0; t < n; ++t) {
    const double xt = 0.8 * xp + 2.0 * u[static_cast<std::size_t>(t)];
    const double yt = 0.5 * yp + 1.0 * xt + 0.3 * xp + 0.3 * e[static_cast<std::size_t>(t)];
    x[static_cast<std::size_t>(t)] = xt;
    y[static_cast<std::size_t>(t)] = yt;
    xp = xt;
    yp = yt;
  }
  return {pluss::make_series(x, {n, 1}).to(f64), pluss::make_series(y, {n}).to(f64)};
}
void row(const char* label, const std::vector<double>& v) {
  std::cout << label;
  for (double a : v) std::cout << std::setw(9) << a;
  std::cout << "\n";
}
int main() {
  const auto data = make_data(300);
  std::cout << std::fixed << std::setprecision(4);

  ts::DirectForecasterOptions options;
  options.lag.horizon = {1, 2, 3};
  options.lag.response_lags = {1, 2};
  options.lag.predictor_lags = {{1, 2}};  // lags 1 and 2 of the single predictor
  const auto model = ts::DirectForecaster::fit(data.x, data.y, options);

  std::cout << "coefficients: intercept, x lag 1, x lag 2, y lag 1, y lag 2\n";
  for (std::int64_t i = 0; i < model.num_steps(); ++i) row(("  horizon " + std::to_string(model.step(i).horizon) + ":").c_str(), model.step(i).beta.to_vec<double>());
  row("in-sample mse by horizon  ", model.loss(data.x, data.y).to_vec<double>());
  row("forecast of y(n+1..n+3)   ", model.forecast(data.x, data.y).to_vec<double>());

  // The same with the predictor treated as leading: its current value (lag 0) is known in advance, so the forecast needs its future values.
  ts::DirectForecasterOptions lead;
  lead.lag.horizon = {1, 2, 3};
  lead.lag.response_lags = {1};
  lead.lag.leading_predictors = {0};
  lead.lag.leading_predictor_lags = {{0, 1}};
  const auto known_future = ts::DirectForecaster::fit(data.x, data.y, lead);
  row("in-sample mse, x known ahead", known_future.loss(data.x, data.y).to_vec<double>());
  const Series future = pluss::make_series(std::vector<double>{0.5, 0.2, -0.4}, {3, 1}).to(f64);
  row("forecast given the next three x", known_future.forecast(data.x, data.y, future).to_vec<double>());
}
```

Output

```text
coefficients: intercept, x lag 1, x lag 2, y lag 1, y lag 2
  horizon 1:  -0.0375   1.3136  -0.0441   0.3412   0.0828
  horizon 2:  -0.1097   1.5844   0.0906   0.0938   0.0203
  horizon 3:  -0.1804   2.0210   0.2619  -0.4782   0.2008
in-sample mse by horizon     0.2997   1.1248   1.9809
forecast of y(n+1..n+3)      0.1748   0.1451   0.0814
in-sample mse, x known ahead   0.0070   0.0116   0.0490
forecast given the next three x   0.4695   0.5768  -0.2074
```
The series is built from its own past and from an exogenous driver. The coefficients differ by horizon because each horizon is a different regression, and the in-sample error grows with the horizon (0.30, 1.12, 1.98): the further ahead, the less the present tells. When the driver is known in advance (a *leading* predictor), the error falls to 0.007, 0.012 and 0.049, close to the variance of the noise of the generating process (0.0075) at one step, and the forecast takes the three known future values of the driver as input. The first rows of the fitted values are NaN and are excluded from the loss.
