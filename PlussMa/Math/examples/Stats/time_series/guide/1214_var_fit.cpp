// Fitting a vector autoregression. `Var::fit_mle(y, options)` takes an n x k matrix (one column per series) and fits y_t = c + A_1 y_{t-1} + ... + A_p y_{t-p} + e_t
// by least squares, which is also maximum likelihood for Gaussian errors. The fit holds `constant` (k), `ar` (p x k x k), `covariance` (k x k, the maximum-likelihood
// estimate, dividing by the number of fitted rows) and `sample_size`. Entry ar[l][j][i] is the effect of series j at lag l+1 on series i, so a row of each matrix
// lists how one series feeds all the others. The number of lags is chosen by comparing the information criteria 2m - 2 loglik (Akaike) and m log(T) - 2 loglik
// (Schwarz), with m = k (k p + 1) parameters.
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
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t  (n x 2, row t = (a_t, b_t)).
Series linked_pair(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(static_cast<std::size_t>(2 * n));
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[static_cast<std::size_t>(t)];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[static_cast<std::size_t>(t)] + e2[static_cast<std::size_t>(t)];
    a = na;
    b = nb;
    y[static_cast<std::size_t>(2 * t)] = a;
    y[static_cast<std::size_t>(2 * t + 1)] = b;
  }
  return pluss::make_series(y, {n, 2}).to(f64);
}
int main() {
  const Series y = linked_pair(300);
  std::cout << std::fixed << std::setprecision(1);
  std::cout << std::setw(6) << "lags" << std::setw(14) << "loglik" << std::setw(12) << "Akaike" << std::setw(12) << "Schwarz" << "\n";
  for (std::int64_t p = 1; p <= 4; ++p) {
    ts::VarOptions o;
    o.lag_order = p;
    const auto fit = ts::Var::fit_mle(y, o);
    const double ll = fit.loglik(y);
    const double m = 2.0 * (2.0 * p + 1.0);
    std::cout << std::setw(6) << p << std::setw(14) << ll << std::setw(12) << 2 * m - 2 * ll << std::setw(12) << m * std::log(static_cast<double>(fit.sample_size)) - 2 * ll << "\n";
  }
  ts::VarOptions o;
  o.lag_order = 1;
  const auto fit = ts::Var::fit_mle(y, o);
  std::cout << std::setprecision(4);
  const auto row = [](const char* label, const Series& x) {
    std::cout << label;
    for (double v : x.reshape({x.numel()}).host<double>()) std::cout << std::setw(9) << v;
    std::cout << "\n";
  };
  std::cout << "\n";
  row("constant            ", fit.constant);
  row("A_1, series 0 feeds ", fit.ar.select(0, 0).select(0, 0));
  row("A_1, series 1 feeds ", fit.ar.select(0, 0).select(0, 1));
  row("covariance row 0    ", fit.covariance.select(0, 0));
  row("covariance row 1    ", fit.covariance.select(0, 1));
  std::cout << "sample size " << fit.sample_size << "\n\nforecast, three steps ahead\n";
  const auto f = fit.forecast(3, y);
  for (std::int64_t h = 0; h < 3; ++h) row("  ", f.select(0, h));
}
