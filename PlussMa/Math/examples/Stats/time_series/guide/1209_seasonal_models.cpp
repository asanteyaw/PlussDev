// Seasonal models. `ts::Sarima::fit_mle(y, options)` takes `SarimaOptions` with the orders written as in the usual notation: non-seasonal p, d, q,
// seasonal P, D, Q, and the period s. The seasonal and non-seasonal polynomials multiply, so a model with p = 1, P = 1, s = 12 has an autoregressive term
// at lag 1, at lag 12 and, as their product, at lag 13 with coefficient -phi Phi. `fit.core` holds the coefficients (`ar`, `sar`, `ma`, `sma`) and
// `loglik(y)` / `forecast(h, y)` work as for ARMA models. Here: a monthly-like series (period 12) built from phi = 0.5, Phi = 0.6, 240 values.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
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
// AR(1): x_t = phi x_{t-1} + e_t
std::vector<double> ar1(int n, double phi) {
  auto e = noise(n);
  std::vector<double> x(static_cast<std::size_t>(n));
  double carry = 0.0;
  for (int i = 0; i < n; ++i) { carry = phi * carry + e[static_cast<std::size_t>(i)]; x[static_cast<std::size_t>(i)] = carry; }
  return x;
}
std::vector<double> running_sum(const std::vector<double>& e) {
  std::vector<double> w(e.size());
  double s = 0.0;
  for (std::size_t i = 0; i < e.size(); ++i) { s += e[i]; w[i] = s; }
  return w;
}
void print_row(const char* label, const Series& s, int precision = 4) {
  std::cout << std::left << std::setw(16) << label << std::fixed << std::setprecision(precision);
  for (double v : s.host<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const auto e = noise(240);
  std::vector<double> data(240, 0.0);
  for (int i = 0; i < 240; ++i) {
    data[static_cast<std::size_t>(i)] = e[static_cast<std::size_t>(i)] + (i >= 1 ? 0.5 * data[static_cast<std::size_t>(i - 1)] : 0.0) + (i >= 12 ? 0.6 * data[static_cast<std::size_t>(i - 12)] : 0.0) -
                                        (i >= 13 ? 0.3 * data[static_cast<std::size_t>(i - 13)] : 0.0);
  }
  const Series y = vec(data);
  std::cout << std::fixed << std::setprecision(4);

  ts::SarimaOptions seasonal;
  seasonal.p = 1;
  seasonal.P = 1;
  seasonal.s = 12;
  const auto fit = ts::Sarima::fit_mle(y, seasonal);
  std::cout << "seasonal model (1,0,0)(1,0,0)12: phi = " << fit.core.ar[0] << ", Phi = " << fit.core.sar[0] << ", sigma = " << fit.core.sigma << ", loglik = " << fit.loglik(y) << "\n";

  ts::SarimaOptions plain;
  plain.p = 2;
  const auto fit2 = ts::Sarima::fit_mle(y, plain);
  std::cout << "non-seasonal AR(2)             : loglik = " << fit2.loglik(y) << "  (the seasonal model is better by " << fit.loglik(y) - fit2.loglik(y) << " with the same number of coefficients)\n\n";

  const auto fc = fit.forecast(12, y).host<double>();
  std::cout << "next 12 forecasts:";
  for (double v : fc) std::cout << " " << v;
  std::cout << "\n";
}
