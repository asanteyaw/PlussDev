<!-- generated from checking_the_residuals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Checking the residuals

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
  std::cout << std::left << std::setw(26) << "model" << std::setw(14) << "Q(10)" << std::setw(8) << "df" << std::setw(14) << "p-value" << "residual acf lag 1\n";
  for (int p = 0; p <= 1; ++p) {
    ts::ArimaOptions o;
    o.ar_order = p;
    const auto fit = ts::Arima::fit_mle(y, o);
    const Series resid = fit.infer(y);
    ts::LjungBoxOptions lb;
    lb.lags = {10};
    lb.model_df = p;
    const auto q = ts::ljung_box(resid, lb);
    ts::AcfOptions a;
    a.max_lag = 1;
    std::cout << std::left << std::setw(26) << (p == 0 ? "mean only (no AR term)" : "AR(1)") << std::fixed << std::setprecision(3) << std::setw(14) << q.stat.to_vec<double>()[0] << std::setw(8) << std::setprecision(0)
              << q.dof.to_vec<double>()[0] << std::scientific << std::setprecision(2) << std::setw(14) << q.pvalue.to_vec<double>()[0] << std::fixed << std::setprecision(3) << ts::acf(resid, a).to_vec<double>()[1] << "\n";
  }
}
```

Output

```text
model                     Q(10)         df      p-value       residual acf lag 1
mean only (no AR term)    244.449       10      7.97e-47      0.650
AR(1)                     11.381        9       2.50e-01      -0.021
```
The mean-only model leaves the autocorrelation in the residuals (lag-1 autocorrelation 0.65, Ljung–Box p ≈ 10⁻⁴⁷). The AR(1) residuals are uncorrelated (lag-1 autocorrelation −0.02, p = 0.25 on 9 degrees of freedom). Always pass `model_df` equal to the number of ARMA coefficients; without it the test is too lenient.
