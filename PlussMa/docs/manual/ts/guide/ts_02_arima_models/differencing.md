<!-- generated from differencing.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Differencing

[Back to ARIMA Models](../ts_02_arima_models.md)


```cpp
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
  const auto e = noise(200);
  const auto walk = running_sum(e);
  const Series y = vec(walk);
  ts::AdfOptions adf;
  adf.lags = 2;
  adf.model = ts::AdfModel::ARD;
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "ADF p-value, level      : " << ts::adf_test(y, adf).pvalue << "\n";
  std::cout << "ADF p-value, first diff : " << std::scientific << std::setprecision(2) << ts::adf_test(ts::diff(y).slice(0, 1, 200, 1), adf).pvalue << std::fixed << std::setprecision(4) << "\n\n";
  ts::ArimaOptions o;
  o.ar_order = 0;
  o.diff_order = 1;
  o.ma_order = 1;
  const auto fit = ts::Arima::fit_mle(y, o);
  std::cout << "ARIMA(0,1,1): ma = " << fit.ma[0] << ", sigma = " << fit.sigma << " (the generating noise has standard deviation " << std::sqrt(1.0 / 12.0) << ")\n";
  const auto fc = fit.forecast(5, y).to_vec<double>();
  std::cout << "last value " << walk.back() << "; forecasts of the level:";
  for (double v : fc) std::cout << " " << v;
  std::cout << "\n";
}
```

Output

```text
ADF p-value, level      : 0.4550
ADF p-value, first diff : 4.24e-13

ARIMA(0,1,1): ma = -0.0932, sigma = 0.2842 (the generating noise has standard deviation 0.2887)
last value 2.8627; forecasts of the level: 2.8656 2.8787 2.8918 2.9049 2.9181
```
A random walk fails the unit-root test in levels (p = 0.46) and passes it after one difference (p ≈ 4×10⁻¹³), so `diff_order = 1` is right. The fitted MA coefficient is small (−0.09), so the changes are close to white noise and the walk is close to ARIMA(0, 1, 0). The forecasts of the level continue from the last value at the estimated drift, the mean of the changes.
