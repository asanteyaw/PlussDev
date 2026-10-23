<!-- generated from volatility_clustering.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Volatility clustering

[Back to Describing a Series](../ts_01_describing_a_series.md)


```cpp
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
  for (double v : s.to_vec<double>()) std::cout << " " << std::setw(8) << v;
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
```

Output

```text
series      lags    LM          p-value       decision
clustered   1       22.992      1.63e-06      ARCH effects
clustered   4       54.463      4.21e-11      ARCH effects
steady      1       0.006       9.36e-01      no ARCH effects
steady      4       4.789       3.10e-01      no ARCH effects
```
A series whose scale alternates between calm and turbulent stretches has strongly autocorrelated squares (LM = 23 on one lag, p ≈ 2×10⁻⁶), while steady noise does not (p = 0.94 and 0.31). Rejecting means a constant-variance model is wrong and a GARCH-type model for the variance (a later chapter) is appropriate. For raw returns leave `demean` on; if the input is the residual vector of a mean model, whose mean is zero by construction, set it to false.
