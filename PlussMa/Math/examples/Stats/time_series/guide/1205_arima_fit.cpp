// Fitting ARMA models. `ts::Arima::fit_mle(y, options)` estimates the coefficients by maximum likelihood. `ArimaOptions` names the orders:
// `ar_order` (p), `diff_order` (d) and `ma_order` (q), and, for seasonal models, `seasonal_ar_order`, `seasonal_diff_order`, `seasonal_ma_order`
// and `seasonal_period`. The likelihood is exact (Kalman filter with the stationary start) unless `exact_likelihood = false`, which selects the
// conditional likelihood. The fit holds the AR and MA coefficients (`ar`, `ma`, with the model x_t = a_1 x_{t-1} + ... + e_t + b_1 e_{t-1} + ...),
// the innovation standard deviation `sigma`, the mean, and `loglik(y)`; Akaike's criterion is -2 loglik + 2k and Schwarz's -2 loglik + k log n,
// with k the number of estimated parameters (coefficients, mean, variance). The data are an AR(1) series with coefficient 0.7, 200 values.
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
  const Series y = vec(ar1(200, 0.7));
  const double n = 200.0;
  std::cout << std::left << std::setw(14) << "model" << std::setw(24) << "coefficients" << std::setw(10) << "sigma" << std::setw(12) << "loglik" << std::setw(10) << "AIC" << "BIC\n";
  struct Spec { const char* name; int p, q; };
  for (const Spec s : {Spec{"AR(1)", 1, 0}, Spec{"AR(2)", 2, 0}, Spec{"ARMA(1,1)", 1, 1}, Spec{"MA(1)", 0, 1}}) {
    ts::ArimaOptions o;
    o.ar_order = s.p;
    o.ma_order = s.q;
    const auto fit = ts::Arima::fit_mle(y, o);
    std::ostringstream coefs;
    coefs << std::fixed << std::setprecision(3);
    for (double a : fit.ar) coefs << "ar " << a << " ";
    for (double b : fit.ma) coefs << "ma " << b << " ";
    const double ll = fit.loglik(y);
    const double k = s.p + s.q + 2.0;
    std::cout << std::left << std::setw(14) << s.name << std::setw(24) << coefs.str() << std::fixed << std::setprecision(4) << std::setw(10) << fit.sigma << std::setw(12) << ll << std::setprecision(2)
              << std::setw(10) << (-2.0 * ll + 2.0 * k) << (-2.0 * ll + k * std::log(n)) << "\n";
  }
}
