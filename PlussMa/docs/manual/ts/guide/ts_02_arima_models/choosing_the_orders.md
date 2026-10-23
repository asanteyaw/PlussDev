<!-- generated from choosing_the_orders.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Choosing the orders

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
```

Output

```text
p     q     loglik      AIC       BIC
0     0     -87.431     178.86    185.46
0     1     -45.337     96.67     106.57
0     2     -40.114     88.23     101.42
1     0     -32.347     70.69     80.59
1     1     -32.040     72.08     85.27
1     2     -28.368     66.74     83.23
2     0     -32.214     72.43     85.62
2     1     -31.330     72.66     89.15
2     2     -28.039     68.08     87.87

lowest AIC: (1, 2), lowest BIC: (1, 0)
```
The two criteria need not agree. Here Akaike's picks ARMA(1, 2) and Schwarz's picks AR(1), the model that generated the series. Akaike's criterion penalises each parameter by 2 and tends to choose larger models; Schwarz's penalty of log n is heavier and favours the true order as the sample grows. When they disagree, prefer the smaller model unless the residual checks of Program 4 say otherwise.
