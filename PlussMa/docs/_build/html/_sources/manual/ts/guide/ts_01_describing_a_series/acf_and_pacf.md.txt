<!-- generated from acf_and_pacf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# ACF and PACF

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
```

Output

```text
acf biased       1.0000   0.6502   0.4426   0.4037   0.3413   0.2859   0.2152  
acf unbiased     1.0000   0.6534   0.4471   0.4099   0.3482   0.2933   0.2218  
0.7^k            1.0000   0.7000   0.4900   0.3430   0.2401   0.1681   0.1176  
pacf             1.0000   0.6534   0.0351   0.1836   0.0154   0.0391   -0.0441 

95% band for a white-noise series of this length: +-0.1386
```
For this AR(1) series the autocorrelations decay roughly geometrically (0.65, 0.44, 0.40, 0.34, ...) but not exactly as 0.7ᵏ: with 200 observations the estimates carry a sampling error of about 0.07 at each lag, and the first estimate (0.65) sits below the true 0.7. The partial autocorrelations show the cut-off clearly: 0.65 at lag 1, then values around zero (0.04, 0.18, 0.02, ...). The 95% band for white noise of this length is ±0.139, so only the lags that exceed it are "real"; lag 3 (0.18) just does. The unbiased normalisation inflates the high lags slightly; the biased one is the usual choice because it guarantees a valid autocorrelation sequence.
