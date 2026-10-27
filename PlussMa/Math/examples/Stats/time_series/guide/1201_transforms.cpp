// Transforming a series. All the transforms take a Series and return one of the same length, with NaN where the value is not defined (the first
// element of a difference, the start of a window with fewer than `min_periods` observations). `diff`, `returns` (x_t / x_{t-1} - 1), `log_returns`,
// `lag` and `lead` shift by `periods`; `rolling_mean`, `rolling_sum`, `rolling_min`, `rolling_max` look back over `window` observations; `rolling_var`
// and `rolling_std` use the sample divisor (count - 1; pass ddof = 0 for the population one) and are NaN for a single observation; `ema` is the
// exponential average m_t = m_{t-1} + alpha (x_t - m_{t-1}) started at x_0; `ewm_std` the matching exponentially weighted standard deviation;
// `expanding_mean` and `expanding_std` use everything so far; `zscore_rolling` standardises by the trailing window.
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
  const Series x = vec({3, 5, 4, 8, 7, 9, 12, 10});
  print_row("x", x, 0);
  print_row("diff", ts::diff(x));
  print_row("diff(2)", ts::diff(x, 2));
  print_row("returns", ts::returns(x));
  print_row("log_returns", ts::log_returns(x));
  print_row("lag", ts::lag(x), 0);
  print_row("lead", ts::lead(x), 0);
  print_row("rolling_mean(3)", ts::rolling_mean(x, 3));
  print_row("rolling_std(3)", ts::rolling_std(x, 3));
  print_row("rolling_std(3,3)", ts::rolling_std(x, 3, 3));
  print_row("rolling_std ddof0", ts::rolling_std(x, 3, 1, 0));
  print_row("rolling_max(3)", ts::rolling_max(x, 3), 0);
  print_row("ema(0.3)", ts::ema(x, 0.3));
  print_row("ewm_std(0.3)", ts::ewm_std(x, 0.3));
  print_row("expanding_mean", ts::expanding_mean(x));
  print_row("expanding_std", ts::expanding_std(x));
  print_row("zscore_rolling(3)", ts::zscore_rolling(x, 3));
}
