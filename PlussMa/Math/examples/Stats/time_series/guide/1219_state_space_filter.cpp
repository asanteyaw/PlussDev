// The Kalman filter. A linear Gaussian state-space model has a hidden state x_t and an observation y_t,
//   x_{t+1} = A x_t + B w_t,   y_t = C x_t + D v_t,   w_t, v_t standard normal,
// described by `SsmModel`: `transition` A (m x m), `state_loading` B (m x k), `observation` C (n x m), `innovation_loading` D (n x h), and the distribution of the state
// *before* the first observation, `mean0` and `cov0`. `Ssm::filter(y, model)` takes the observations as a T x n matrix and returns, for every t, the filtered state
// and its covariance, the one-step forecasts of state and observation, the forecast error and its covariance, the Kalman gain, and the log-likelihood (per observation and
// in total). The log-likelihood makes the unknown noise levels estimable: below, the observation noise of a local linear trend model is scanned and the likelihood peaks
// near the true value of 1/sqrt(12) = 0.289 used to generate the data.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series matrix(std::vector<double> v, std::vector<std::int64_t> shape) { return pluss::make_series(v, shape).to(f64); }
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// A noisy upward trend: y_t = 1 + 0.3 t + 1.0 e_t, t = 0 ... n - 1.
std::vector<double> trending(int n) {
  const auto e = noise(n);
  std::vector<double> y(static_cast<std::size_t>(n));
  for (int t = 0; t < n; ++t) y[static_cast<std::size_t>(t)] = 1.0 + 0.3 * t + e[static_cast<std::size_t>(t)];
  return y;
}
// Local linear trend: the state is (level, slope); level_{t+1} = level_t + slope_t + level_sd w1, slope_{t+1} = slope_t + slope_sd w2, and y_t = level_t + obs_sd v.
ts::SsmModel local_linear_trend(double level_sd, double slope_sd, double obs_sd) {
  ts::SsmModel m;
  m.transition = matrix({1, 1, 0, 1}, {2, 2});
  m.state_loading = matrix({level_sd, 0, 0, slope_sd}, {2, 2});
  m.observation = matrix({1, 0}, {1, 2});
  m.innovation_loading = matrix({obs_sd}, {1, 1});
  m.mean0 = matrix({0, 0}, {2});
  m.cov0 = matrix({25, 0, 0, 1}, {2, 2});
  return m;
}
int main() {
  const int n = 60;
  const Series y = matrix(trending(n), {n, 1});
  std::cout << std::fixed << std::setprecision(3);
  const auto fit = ts::Ssm::filter(y, local_linear_trend(0.2, 0.05, 0.3));
  std::cout << "filtered level and slope, and one-step forecast error\n";
  for (int t : {0, 1, 2, 10, 30, 59}) {
    const auto x = fit.filtered_states.select(0, t).host<double>();
    std::cout << "  t = " << std::setw(2) << t << "   level " << std::setw(7) << x[0] << "   slope " << std::setw(6) << x[1] << "   error " << std::setw(7)
              << fit.forecast_obs_error.select(0, t).item<double>() << "\n";
  }
  std::cout << "log-likelihood " << fit.loglik << "\n\nobservation noise scan (level sd 0.2, slope sd 0.05)\n";
  for (double sd : {0.05, 0.1, 0.2, 0.3, 0.4, 0.6, 1.0})
    std::cout << "  obs sd " << std::setw(5) << sd << "   log-likelihood " << std::setw(8) << ts::Ssm::filter(y, local_linear_trend(0.2, 0.05, sd)).loglik << "\n";
}
