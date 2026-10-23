<!-- generated from fitting_a_var_and_choosing_the_lag_order.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fitting a VAR and choosing the lag order

[Back to Vector Autoregressions and Cointegration](../ts_04_vector_models.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
// A reproducible pseudo-random sequence in (-0.5, 0.5): e_t = frac(sin(12.9898 (t + offset) + 78.233) * 43758.5453) - 0.5, t = 1, 2, ...
std::vector<double> noise(int n, int offset = 0) {
  std::vector<double> e(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[static_cast<std::size_t>(i)] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t  (n x 2, row t = (a_t, b_t)).
Series linked_pair(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(static_cast<std::size_t>(2 * n));
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[static_cast<std::size_t>(t)];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[static_cast<std::size_t>(t)] + e2[static_cast<std::size_t>(t)];
    a = na;
    b = nb;
    y[static_cast<std::size_t>(2 * t)] = a;
    y[static_cast<std::size_t>(2 * t + 1)] = b;
  }
  return pluss::make_series(y, {n, 2}).to(f64);
}
int main() {
  const Series y = linked_pair(300);
  std::cout << std::fixed << std::setprecision(1);
  std::cout << std::setw(6) << "lags" << std::setw(14) << "loglik" << std::setw(12) << "Akaike" << std::setw(12) << "Schwarz" << "\n";
  for (std::int64_t p = 1; p <= 4; ++p) {
    ts::VarOptions o;
    o.lag_order = p;
    const auto fit = ts::Var::fit_mle(y, o);
    const double ll = fit.loglik(y);
    const double m = 2.0 * (2.0 * p + 1.0);
    std::cout << std::setw(6) << p << std::setw(14) << ll << std::setw(12) << 2 * m - 2 * ll << std::setw(12) << m * std::log(static_cast<double>(fit.sample_size)) - 2 * ll << "\n";
  }
  ts::VarOptions o;
  o.lag_order = 1;
  const auto fit = ts::Var::fit_mle(y, o);
  std::cout << std::setprecision(4);
  const auto row = [](const char* label, const Series& x) {
    std::cout << label;
    for (double v : x.reshape({x.numel()}).to_vec<double>()) std::cout << std::setw(9) << v;
    std::cout << "\n";
  };
  std::cout << "\n";
  row("constant            ", fit.constant);
  row("A_1, series 0 feeds ", fit.ar.select(0, 0).select(0, 0));
  row("A_1, series 1 feeds ", fit.ar.select(0, 0).select(0, 1));
  row("covariance row 0    ", fit.covariance.select(0, 0));
  row("covariance row 1    ", fit.covariance.select(0, 1));
  std::cout << "sample size " << fit.sample_size << "\n\nforecast, three steps ahead\n";
  const auto f = fit.forecast(3, y);
  for (std::int64_t h = 0; h < 3; ++h) row("  ", f.select(0, h));
}
```

Output

```text
  lags        loglik      Akaike     Schwarz
     1         -93.7       199.4       221.6
     2         -93.2       206.3       243.3
     3         -89.2       206.5       258.2
     4         -88.7       213.3       279.7

constant               0.0025   0.0011
A_1, series 0 feeds    0.5210   0.1654
A_1, series 1 feeds    0.1466   0.5387
covariance row 0       0.0792   0.0365
covariance row 1       0.0365   0.0978
sample size 299

forecast, three steps ahead
     0.0149   0.0190
     0.0131   0.0138
     0.0113   0.0107
```
The data come from a VAR(1) with coefficients 0.5, 0.2, 0.1, 0.6. Both criteria prefer one lag (Akaike's 199.4 against 206.3 for two lags; Schwarz's even more so), and the first-lag estimates (rows 0.52, 0.17 and 0.15, 0.54 in the layout described above) are close to the generating values; the larger-than-generating 0.17 reflects the correlation introduced by the first shock entering both equations. The covariance entries are the maximum-likelihood ones.
