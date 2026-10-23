// Direct multi-step forecasting. Instead of iterating a one-step model, a *direct* forecaster fits a separate regression for every forecast horizon h: it predicts
// y_{t+h} from lagged values of y and of exogenous predictors, all known at time t. `DirectForecaster::fit(x, y, options)` takes the predictors `x` (n x p, or an undefined
// Series for none) and the response `y`. `DirectForecasterOptions::lag` is a `LagPlanOptions`: `horizon` (the steps to forecast), `response_lags`, `predictor_lags` (one list of
// lags per predictor that is not leading), `leading_predictors` (0-based column indices of predictors whose future values are known) and `leading_predictor_lags` (lag 0 is the
// value at the forecast time itself). `learner` is OLS or Ridge (`ridge_lambda`), `fit_intercept` adds a constant. Only rows with a complete set of lagged values enter the fit.
// `step(i).beta` holds the coefficients of horizon `step(i).horizon`: the intercept first, then one per feature in the order non-leading predictor lags, response lags,
// leading predictor lags. `forecast(x, y)` forecasts from the last observation; `predict` gives the fitted values for every row (NaN where lags are not yet available);
// `loss` the mean squared (or absolute) error over the rows that have a prediction.
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
