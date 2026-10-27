// The autocorrelation structure. `ts::acf(x)` returns the sample autocorrelations at lags 0..max_lag (lag 0 is 1); `ts::pacf(x)` the partial
// autocorrelations, the correlation at each lag after removing the effect of the shorter lags. `max_lag` defaults to 20, `demean` removes the sample
// mean first, and `norm` chooses the divisor of the autocovariances: Biased (the usual one, always a valid autocorrelation sequence) or Unbiased
// (n - k, which inflates the high lags). Here: an AR(1) series with coefficient 0.7 (200 values): its theoretical autocorrelations are 0.7^k, and
// its partial autocorrelations are 0.7 at lag 1 and zero beyond, the signature of an AR(1) process.
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
  const Series x = vec(ar1(200, 0.7));
  ts::AcfOptions a;
  a.max_lag = 6;
  print_row("acf biased", ts::acf(x, a));
  a.norm = ts::AcfNorm::Unbiased;
  print_row("acf unbiased", ts::acf(x, a));
  std::cout << std::left << std::setw(16) << "0.7^k";
  for (int k = 0; k <= 6; ++k) std::cout << " " << std::setw(8) << std::pow(0.7, k);
  std::cout << "\n";
  ts::PacfOptions p;
  p.max_lag = 6;
  p.norm = ts::AcfNorm::Unbiased;
  print_row("pacf", ts::pacf(x, p));
  std::cout << "\n95% band for a white-noise series of this length: +-" << std::setprecision(4) << 1.96 / std::sqrt(200.0) << "\n";
}
