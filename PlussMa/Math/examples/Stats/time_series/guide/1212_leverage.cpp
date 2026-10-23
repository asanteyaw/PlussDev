// Asymmetric volatility. In many markets a fall raises volatility more than a rise of the same size. `Gjr` adds a term that switches on for negative shocks,
// h_t = omega + (alpha + gamma 1[e_{t-1} < 0]) e_{t-1}^2 + beta h_{t-1}; `Egarch` models the logarithm of the variance, so it needs no positivity constraints and
// responds to the sign of the standardised shock through `asym`. Both fits expose `arch`, `asym`, `garch` as vectors (one entry per lag), `loglik(y)` and `forecast`.
// The returns below have a stronger response to negative shocks (alpha 0.05, gamma 0.20); the three models are compared on the same data.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
std::vector<double> shocks(int n) {
  std::vector<double> z(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    z[static_cast<std::size_t>(i)] = (u - 0.5) * std::sqrt(12.0);
  }
  return z;
}
// h_t = 0.05 + (0.05 + 0.20 * 1[r_{t-1} < 0]) r_{t-1}^2 + 0.75 h_{t-1}
std::vector<double> leverage_returns(int n) {
  const auto z = shocks(n);
  std::vector<double> r(static_cast<std::size_t>(n));
  double h = 0.5, prev = 0.0;
  for (int t = 0; t < n; ++t) {
    h = 0.05 + (0.05 + (prev < 0.0 ? 0.20 : 0.0)) * prev * prev + 0.75 * h;
    r[static_cast<std::size_t>(t)] = std::sqrt(h) * z[static_cast<std::size_t>(t)];
    prev = r[static_cast<std::size_t>(t)];
  }
  return r;
}
int main() {
  const Series y = vec(leverage_returns(2000));
  const auto g = ts::Garch11::fit_mle(y);
  const auto j = ts::Gjr::fit_mle(y);
  const auto e = ts::Egarch::fit_mle(y);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "GARCH(1,1)  omega " << g.omega << "  alpha " << g.alpha << "  beta " << g.beta << "\n";
  std::cout << "GJR         omega " << j.omega << "  alpha " << j.arch[0] << "  gamma " << j.asym[0] << "  beta " << j.garch[0] << "   (true 0.05, 0.05, 0.20, 0.75)\n";
  std::cout << "EGARCH      omega " << e.omega << "  alpha " << e.arch[0] << "  asym " << e.asym[0] << "  beta " << e.garch[0] << "\n\n";
  // Two extra parameters in each asymmetric model: compare by the Akaike criterion, 2k - 2 loglik, smaller is better.
  const double ll_g = g.loglik(y), ll_j = j.loglik(y), ll_e = e.loglik(y);
  std::cout << std::setprecision(2) << std::setw(12) << "model" << std::setw(14) << "loglik" << std::setw(10) << "AIC" << "\n";
  std::cout << std::setw(12) << "GARCH" << std::setw(14) << ll_g << std::setw(10) << 2 * 4 - 2 * ll_g << "\n";
  std::cout << std::setw(12) << "GJR" << std::setw(14) << ll_j << std::setw(10) << 2 * 5 - 2 * ll_j << "\n";
  std::cout << std::setw(12) << "EGARCH" << std::setw(14) << ll_e << std::setw(10) << 2 * 5 - 2 * ll_e << "\n";
}
