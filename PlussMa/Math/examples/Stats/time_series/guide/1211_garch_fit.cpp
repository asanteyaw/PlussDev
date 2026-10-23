// Fitting a GARCH(1,1) model. The conditional variance follows h_t = omega + alpha e_{t-1}^2 + beta h_{t-1}, where e_t = y_t - mean. `Garch11::fit_mle` maximises the
// Gaussian likelihood and returns a `Garch11Fit`: `mean`, `omega`, `alpha`, `beta`, `unconditional_var()` and the methods `loglik(y)`, `infer(y)` (the fitted conditional
// variances) and `forecast(h, y)`. The series here is built from known parameters (omega 0.05, alpha 0.15, beta 0.75), so the estimates can be read against them.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
// Reproducible shocks with unit variance: z_t = (frac(sin(12.9898 t + 78.233) * 43758.5453) - 0.5) * sqrt(12), t = 1, 2, ...
std::vector<double> shocks(int n) {
  std::vector<double> z(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    z[static_cast<std::size_t>(i)] = (u - 0.5) * std::sqrt(12.0);
  }
  return z;
}
// GARCH(1,1) returns: h_t = 0.05 + 0.15 r_{t-1}^2 + 0.75 h_{t-1}, r_t = sqrt(h_t) z_t.
std::vector<double> garch_returns(int n) {
  const auto z = shocks(n);
  std::vector<double> r(static_cast<std::size_t>(n));
  double h = 0.5, prev = 0.0;
  for (int t = 0; t < n; ++t) {
    h = 0.05 + 0.15 * prev * prev + 0.75 * h;
    r[static_cast<std::size_t>(t)] = std::sqrt(h) * z[static_cast<std::size_t>(t)];
    prev = r[static_cast<std::size_t>(t)];
  }
  return r;
}
int main() {
  const Series y = vec(garch_returns(1000));
  const auto fit = ts::Garch11::fit_mle(y);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "mean   " << fit.mean << "\nomega  " << fit.omega << "   (true 0.0500)\nalpha  " << fit.alpha << "   (true 0.1500)\nbeta   " << fit.beta
            << "   (true 0.7500)\n";
  std::cout << "alpha + beta " << fit.alpha + fit.beta << "   unconditional variance " << fit.unconditional_var() << "\nlog-likelihood " << std::setprecision(2)
            << fit.loglik(y) << "\n\n";
  std::cout << std::setprecision(4) << "forecast of the variance, by horizon\n";
  const auto f = fit.forecast(100, y).to_vec<double>();
  for (int h : {1, 2, 5, 10, 20, 50, 100}) std::cout << "  h = " << std::setw(3) << h << "   " << f[static_cast<std::size_t>(h - 1)] << "\n";
  const auto v = fit.infer(y).to_vec<double>();
  std::cout << "last fitted conditional variance " << v.back() << "\n";
}
