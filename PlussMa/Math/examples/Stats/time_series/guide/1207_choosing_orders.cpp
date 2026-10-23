// Choosing orders. Fit a grid of ARMA(p, q) models and compare Akaike's and Schwarz's criteria (smaller is better; Schwarz penalises parameters more).
// The true model is AR(1), so a good criterion should prefer (1, 0): extra terms add parameters and rarely improve the likelihood enough to pay for
// them. The criteria are comparable between models only when they are fitted to the same series with the same likelihood.
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
  for (double v : s.to_vec<double>()) std::cout << " " << std::setw(8) << v;
  std::cout << "\n";
}
int main() {
  const Series y = vec(ar1(200, 0.7));
  const double n = 200.0;
  std::cout << std::left << std::setw(6) << "p" << std::setw(6) << "q" << std::setw(12) << "loglik" << std::setw(10) << "AIC" << "BIC\n";
  double best_aic = 1e300, best_bic = 1e300;
  int a_p = 0, a_q = 0, b_p = 0, b_q = 0;
  for (int p = 0; p <= 2; ++p) {
    for (int q = 0; q <= 2; ++q) {
      ts::ArimaOptions o;
      o.ar_order = p;
      o.ma_order = q;
      const auto fit = ts::Arima::fit_mle(y, o);
      const double ll = fit.loglik(y), k = p + q + 2.0;
      const double aic = -2.0 * ll + 2.0 * k, bic = -2.0 * ll + k * std::log(n);
      std::cout << std::left << std::setw(6) << p << std::setw(6) << q << std::fixed << std::setprecision(3) << std::setw(12) << ll << std::setprecision(2) << std::setw(10) << aic << bic << "\n";
      if (aic < best_aic) { best_aic = aic; a_p = p; a_q = q; }
      if (bic < best_bic) { best_bic = bic; b_p = p; b_q = q; }
    }
  }
  std::cout << "\nlowest AIC: (" << a_p << ", " << a_q << "), lowest BIC: (" << b_p << ", " << b_q << ")\n";
}
