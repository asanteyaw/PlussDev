<!-- generated from impulse_responses_and_variance_decompositions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Impulse responses and variance decompositions

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
void show(const char* title, const Series& r) {
  std::cout << title << "\n";
  const auto v = r.to_vec<double>();
  for (std::int64_t s = 0; s < r.sizes()[0]; ++s)
    std::cout << "  step " << s << "   shock 0: " << std::setw(8) << v[static_cast<std::size_t>(4 * s)] << std::setw(8) << v[static_cast<std::size_t>(4 * s + 1)] << "   shock 1: " << std::setw(8)
              << v[static_cast<std::size_t>(4 * s + 2)] << std::setw(8) << v[static_cast<std::size_t>(4 * s + 3)] << "\n";
}
int main() {
  const Series y = linked_pair(300);
  ts::VarOptions o;
  o.lag_order = 1;
  const auto fit = ts::Var::fit_mle(y, o);
  std::cout << std::fixed << std::setprecision(4);
  ts::IrfOptions io;
  io.horizon = 5;
  show("orthogonalised responses (each shock: response of series 0, series 1)", fit.irf(io));
  io.method = ts::IrfMethod::Generalized;
  show("generalised responses", fit.irf(io));
  ts::FevdOptions fo;
  fo.horizon = 5;
  show("variance shares (each shock: share of series 0, series 1)", fit.fevd(fo));
  fo.method = ts::IrfMethod::Generalized;
  show("generalised variance shares (normalised to add up to one)", fit.fevd(fo));
}
```

Output

```text
orthogonalised responses (each shock: response of series 0, series 1)
  step 0   shock 0:   0.2815  0.1298   shock 1:   0.0000  0.2845
  step 1   shock 0:   0.1657  0.1165   shock 1:   0.0417  0.1533
  step 2   shock 0:   0.1034  0.0901   shock 1:   0.0442  0.0895
  step 3   shock 0:   0.0671  0.0657   shock 1:   0.0361  0.0555
  step 4   shock 0:   0.0446  0.0465   shock 1:   0.0270  0.0359
generalised responses
  step 0   shock 0:   0.2815  0.1298   shock 1:   0.1168  0.3127
  step 1   shock 0:   0.1657  0.1165   shock 1:   0.1067  0.1878
  step 2   shock 0:   0.1034  0.0901   shock 1:   0.0831  0.1188
  step 3   shock 0:   0.0671  0.0657   shock 1:   0.0607  0.0778
  step 4   shock 0:   0.0446  0.0465   shock 1:   0.0430  0.0519
variance shares (each shock: share of series 0, series 1)
  step 0   shock 0:   1.0000  0.1722   shock 1:   0.0000  0.8278
  step 1   shock 0:   0.9840  0.2254   shock 1:   0.0160  0.7746
  step 2   shock 0:   0.9695  0.2552   shock 1:   0.0305  0.7448
  step 3   shock 0:   0.9606  0.2705   shock 1:   0.0394  0.7295
  step 4   shock 0:   0.9558  0.2781   shock 1:   0.0442  0.7219
generalised variance shares (normalised to add up to one)
  step 0   shock 0:   0.8531  0.1469   shock 1:   0.1469  0.8531
  step 1   shock 0:   0.8100  0.1860   shock 1:   0.1900  0.8140
  step 2   shock 0:   0.7861  0.2075   shock 1:   0.2139  0.7925
  step 3   shock 0:   0.7738  0.2185   shock 1:   0.2262  0.7815
  step 4   shock 0:   0.7677  0.2240   shock 1:   0.2323  0.7760
```
At step 0 the orthogonalised response of series 0 to its own shock is 0.2815, the square root of its error variance, and a shock to series 1 does not move series 0 on impact (the Cholesky ordering). The generalised response to the same shock is 0.1168, the covariance divided by the standard deviation: it lets series 0 move with the correlated shock. All responses die out geometrically, as the VAR is stable. Series 0's variance is almost entirely its own shock (over 95% at five steps), while series 1's variance is shared (roughly a quarter of it caused by the shock to series 0 at five steps). The generalised shares, normalised to add up to one, are less lopsided because they do not attribute the common part of the shocks to the first series.
