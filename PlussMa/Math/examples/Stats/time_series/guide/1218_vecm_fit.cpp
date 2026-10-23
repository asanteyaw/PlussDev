// A vector error-correction model. `Vecm::fit_mle(y, options)` fits  Delta y_t = c + y_{t-1} beta alpha' + Gamma_1 Delta y_{t-1} + ... + e_t  for a chosen `rank` (negative
// = pick it by the trace test at level `alpha`). `beta` (k x r) holds the cointegrating vectors, normalised so that its leading r x r block is the identity; `alpha`
// (k x r) the adjustment loadings: how strongly each series reacts to last period's deviation y_{t-1} beta from the long-run relation. `impact = beta alpha'`, `gamma`
// the short-run matrices (p - 1 of them), `constant`, `covariance`. The estimate is Johansen's maximum-likelihood one. `forecast(h, y)` and `simulate(n)` work as for
// the other models.
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
// Three series (n x 3): x is a random walk, y = x + u with u a stationary AR(1) (so y is tied to x), and w is an unrelated random walk.
Series three_series(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(static_cast<std::size_t>(3 * n));
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[static_cast<std::size_t>(t)];
    u = 0.6 * u + e2[static_cast<std::size_t>(t)];
    w += e3[static_cast<std::size_t>(t)];
    y[static_cast<std::size_t>(3 * t)] = x;
    y[static_cast<std::size_t>(3 * t + 1)] = x + u;
    y[static_cast<std::size_t>(3 * t + 2)] = w;
  }
  return pluss::make_series(y, {n, 3}).to(f64);
}
void row(const char* label, const Series& x) {
  std::cout << label;
  for (double v : x.reshape({x.numel()}).to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
int main() {
  const Series y = three_series(400);
  ts::VecmOptions o;
  o.lag_order = 1;
  o.rank = -1;  // choose the rank by the trace test
  const auto fit = ts::Vecm::fit_mle(y, o);
  std::cout << std::fixed << std::setprecision(4) << "rank chosen " << fit.rank << "   sample size " << fit.sample_size << "\n";
  row("beta (long run)   ", fit.beta.select(1, 0));
  row("alpha (loadings)  ", fit.alpha.select(1, 0));
  row("constant          ", fit.constant);
  std::cout << "log-likelihood " << std::setprecision(2) << fit.loglik(y) << "\n\nforecast of the levels\n" << std::setprecision(4);
  const auto f = fit.forecast(3, y);
  for (std::int64_t h = 0; h < 3; ++h) row("  ", f.select(0, h));
  std::cout << "last observation\n";
  row("  ", y.select(0, 399));
}
