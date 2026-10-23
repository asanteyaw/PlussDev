// Comparing forecasting models out of sample. `DirectCVForecaster::fit(x, y, partition, options)` fits one direct forecaster per training window of a partition;
// `cvpredict` returns, for every row that lies in some test set, the prediction made by the model that did not train on it (NaN elsewhere), and `cvloss` the loss: averaged
// over the test rows (`CVLossMode::Average`) or per fold (`CVLossMode::Individual`), for each horizon. Two models are then compared with the Diebold-Mariano test,
// `dm_test(errors_a, errors_b, options)`: its null hypothesis is that both have the same expected loss. `DmOptions`: `horizon` h (the errors are correlated up to lag h - 1,
// which the test accounts for), `loss` (Squared, Absolute or Power), `hln_adjust` (the Harvey-Leybourne-Newbold small-sample correction and Student-t reference, default
// on), `tail` and `alpha`. The result holds `stat`, `pvalue`, `critical`, `reject`, the mean loss difference `mean_diff` and the long-run variance `lrv`. A negative statistic
// favours the first model.
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
int main() {
  const auto data = make_data(400);
  const auto partition = ts::TsPartition::expanding_window(400, 4);
  std::cout << std::fixed << std::setprecision(4);

  // Model A uses only the past of y; model B adds the lags of x.
  ts::DirectForecasterOptions a, b;
  a.lag.horizon = b.lag.horizon = {1};
  a.lag.response_lags = b.lag.response_lags = {1, 2};
  b.lag.predictor_lags = {{1, 2}};
  const auto cv_a = ts::DirectCVForecaster::fit(Series{}, data.y, partition, a);
  const auto cv_b = ts::DirectCVForecaster::fit(data.x, data.y, partition, b);

  std::cout << "mean squared error per fold      fold 1    fold 2    fold 3    fold 4\n";
  for (const auto* item : {&cv_a, &cv_b}) {
    const auto& cv = *item;
    const auto per_fold = cv.cvloss(item == &cv_a ? Series{} : data.x, data.y, ts::CVLossMode::Individual).to_vec<double>();
    std::cout << (item == &cv_a ? "  A: y lags only            " : "  B: y lags and x lags      ");
    for (double v : per_fold) std::cout << std::setw(10) << v;
    std::cout << "\n";
  }

  // Errors on the rows that every test set covers once.
  const auto pa = cv_a.cvpredict(Series{}, data.y).reshape({400}).to_vec<double>();
  const auto pb = cv_b.cvpredict(data.x, data.y).reshape({400}).to_vec<double>();
  const auto y = data.y.to_vec<double>();
  std::vector<double> ea, eb;
  for (std::size_t t = 0; t < y.size(); ++t)
    if (std::isfinite(pa[t]) && std::isfinite(pb[t])) {
      ea.push_back(y[t] - pa[t]);
      eb.push_back(y[t] - pb[t]);
    }
  const auto r = ts::dm_test(pluss::make_series(ea, {static_cast<std::int64_t>(ea.size())}).to(f64), pluss::make_series(eb, {static_cast<std::int64_t>(eb.size())}).to(f64));
  std::cout << "\nDiebold-Mariano test on " << r.effective_n << " out-of-sample errors\n";
  std::cout << "  mean loss difference (A - B) " << r.mean_diff << "   statistic " << r.stat << "   p-value " << std::scientific << std::setprecision(2) << r.pvalue << "   "
            << (r.reject ? "the models differ" : "no difference") << "\n";
}
