// Testing for volatility clustering. Engle's ARCH-LM test regresses the squared series on its own `lags` lagged values; n R^2 of that regression is
// chi-squared with `lags` degrees of freedom under the null of no ARCH effects. `demean` (default true) subtracts the sample mean first, as is right for
// a raw series; pass false when the input is already a vector of model residuals. Here: returns whose size alternates between calm and turbulent
// stretches (50 values each), against steady noise of the same overall size.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
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
  const auto e = noise(300);
  std::vector<double> clustered(300), steady(300);
  for (int i = 0; i < 300; ++i) {
    const double scale = ((i / 50) % 2 == 0) ? 0.3 : 2.0;
    clustered[static_cast<std::size_t>(i)] = scale * e[static_cast<std::size_t>(i)];
    steady[static_cast<std::size_t>(i)] = 1.2 * e[static_cast<std::size_t>(i)];
  }
  std::cout << std::left << std::setw(12) << "series" << std::setw(8) << "lags" << std::setw(12) << "LM" << std::setw(14) << "p-value" << "decision\n";
  for (int which = 0; which < 2; ++which) {
    for (std::int64_t lags : {1, 4}) {
      ts::ArchLmOptions o;
      o.lags = lags;
      const auto r = ts::arch_lm(vec(which == 0 ? clustered : steady), o);
      std::cout << std::left << std::setw(12) << (which == 0 ? "clustered" : "steady") << std::setw(8) << lags << std::fixed << std::setprecision(3) << std::setw(12) << r.stat << std::scientific
                << std::setprecision(2) << std::setw(14) << r.pvalue << (r.reject ? "ARCH effects" : "no ARCH effects") << "\n";
    }
  }
}
