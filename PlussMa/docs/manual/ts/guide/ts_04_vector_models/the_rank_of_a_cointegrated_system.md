<!-- generated from the_rank_of_a_cointegrated_system.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The rank of a cointegrated system

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
// Three series (n x 3): x is a random walk, y = x + u with u a stationary AR(1) (so y is tied to x), and w is an unrelated random walk.
Series three_series(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(static_cast<std::size_t>(3 * n));
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[static_cast<std::size_t>(t)];
    u = 0.6 * u + e2[static_cast<std::size_t>(t)];
    w += e3[static_cast<std::size_t>(t)];
    y[static_cast<std::size_t>(3 * t)] = x;
    y[static_cast<std::size_t>(3 * t + 1)] = x + u;
    y[static_cast<std::size_t>(3 * t + 2)] = w;
  }
  return pluss::make_series(y, {n, 3}).to(f64);
}
int main() {
  const Series y = three_series(400);
  for (auto stat : {ts::JohansenStat::Trace, ts::JohansenStat::MaxEigen}) {
    ts::JohansenOptions o;
    o.lag_order = 1;
    o.statistic = stat;
    const auto r = ts::johansen_test(y, o);
    const auto lambda = r.eigenvalues.to_vec<double>(), s = r.stat.to_vec<double>(), c = r.critical.to_vec<double>(), p = r.pvalue.to_vec<double>();
    const auto rej = r.reject.to_vec<std::int64_t>();
    std::cout << (stat == ts::JohansenStat::Trace ? "trace statistic\n" : "maximum-eigenvalue statistic\n");
    std::cout << std::setw(10) << "H0: rank<=" << std::setw(12) << "eigenvalue" << std::setw(12) << "statistic" << std::setw(12) << "5% critical" << std::setw(10) << "p-value" << "\n";
    for (std::size_t i = 0; i < s.size(); ++i)
      std::cout << std::setw(10) << i << std::fixed << std::setprecision(4) << std::setw(12) << lambda[i] << std::setprecision(2) << std::setw(12) << s[i] << std::setw(12) << c[i] << std::setprecision(4)
                << std::setw(10) << p[i] << (rej[i] ? "   reject" : "   keep") << "\n";
    std::cout << "selected rank " << r.selected_rank << "\n\n";
  }
}
```

Output

```text
trace statistic
H0: rank<=  eigenvalue   statistic 5% critical   p-value
         0      0.2301      115.47       29.66    0.0000   reject
         1      0.0261       11.11       15.46    0.2043   keep
         2      0.0014        0.56        3.85    0.4551   keep
selected rank 1

maximum-eigenvalue statistic
H0: rank<=  eigenvalue   statistic 5% critical   p-value
         0      0.2301      104.36       21.06    0.0000   reject
         1      0.0261       10.55       14.23    0.1784   keep
         2      0.0014        0.56        3.85    0.4551   keep
selected rank 1
```
Three series, of which two are tied together (`y = x + stationary noise`) and one is unrelated. The hypothesis of no relation (`rank ≤ 0`) is rejected overwhelmingly by both statistics, while `rank ≤ 1` is not, so exactly one relation is found. The test sequence stops at the first non-rejection: a later rejection is ignored.
