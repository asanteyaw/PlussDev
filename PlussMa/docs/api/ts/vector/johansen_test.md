<!-- generated from johansen_test.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::johansen_test`

Johansen's cointegration rank test.

## Signature

```cpp
ts::JohansenResult ts::johansen_test(const pluss::Series& y, const ts::JohansenOptions& options = {}, pluss::codem::Config cfg = {});
```

## What It Does

`johansen_test` finds how many independent long-run relations tie a set of series together. It removes the deterministic terms and the lagged differences from the changes and from the lagged levels, computes the squared canonical correlations between the two, and tests the sequence of hypotheses `rank ≤ 0`, `rank ≤ 1`, … by the trace or the maximum-eigenvalue statistic. Neither follows a chi-squared law: the limiting distributions are non-standard functionals of Brownian motion that depend on the deterministic terms and on the number `k − r` of series still tested, so the p-values and critical values come from simulated tables, not from a chi-squared. The rank is the first `r` that is not rejected. Pass the series in levels.

## Parameters

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const ts::JohansenOptions& options`
: Statistic, lag order, deterministic terms and level; see [`JohansenOptions`](JohansenOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

## Returns

A [`JohansenResult`](JohansenResult.md).

## Errors

- At most 12 series are supported.
- `lag_order` must be positive and the sample long enough.

## Practical Examples

### Example 1

Three series, two of them tied by a stationary difference: one cointegrating relation is found, by both statistics.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Three series: x is a random walk, y = x + a stationary AR(1) (so y is tied to x), w is an unrelated random walk.
Series three(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(3 * n);
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[t];
    u = 0.6 * u + e2[t];
    w += e3[t];
    y[3 * t] = x;
    y[3 * t + 1] = x + u;
    y[3 * t + 2] = w;
  }
  return pluss::make_series(y, {n, 3}, f64);
}

int main() {
  const Series y = three(400);
  for (auto statistic : {ts::JohansenStat::Trace, ts::JohansenStat::MaxEigen}) {
    ts::JohansenOptions options;
    options.statistic = statistic;
    const auto r = ts::johansen_test(y, options);
    const auto s = r.stat.host<double>(), c = r.critical.host<double>(), p = r.pvalue.host<double>();
    std::cout << (statistic == ts::JohansenStat::Trace ? "trace statistic\n" : "maximum-eigenvalue statistic\n");
    for (std::size_t i = 0; i < s.size(); ++i)
      std::cout << "  rank <= " << i << "  statistic " << std::fixed << std::setprecision(2) << s[i] << "  5% critical " << c[i] << "  p " << std::setprecision(4) << p[i] << "\n";
    std::cout << "  selected rank " << r.selected_rank << "\n";
  }
}
```

Output

```text
trace statistic
  rank <= 0  statistic 115.47  5% critical 29.66  p 0.0000
  rank <= 1  statistic 11.11  5% critical 15.46  p 0.2043
  rank <= 2  statistic 0.56  5% critical 3.85  p 0.4551
  selected rank 1
maximum-eigenvalue statistic
  rank <= 0  statistic 104.36  5% critical 21.06  p 0.0000
  rank <= 1  statistic 10.55  5% critical 14.23  p 0.1784
  rank <= 2  statistic 0.56  5% critical 3.85  p 0.4551
  selected rank 1
```

### Example 2

The deterministic specification changes the critical values: no constant, a constant, and a constant with a trend.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Three series: x is a random walk, y = x + a stationary AR(1) (so y is tied to x), w is an unrelated random walk.
Series three(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(3 * n);
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[t];
    u = 0.6 * u + e2[t];
    w += e3[t];
    y[3 * t] = x;
    y[3 * t + 1] = x + u;
    y[3 * t + 2] = w;
  }
  return pluss::make_series(y, {n, 3}, f64);
}

int main() {
  const Series y = three(400);
  for (int spec = 0; spec < 3; ++spec) {
    ts::JohansenOptions options;
    options.include_intercept = spec >= 1;
    options.include_trend = spec == 2;
    const auto r = ts::johansen_test(y, options);
    std::cout << "spec " << spec << "  5% critical values for rank <= 0, 1, 2: ";
    for (double c : r.critical.host<double>()) std::cout << std::fixed << std::setprecision(2) << c << " ";
    std::cout << " selected rank " << r.selected_rank << "\n";
  }
}
```

Output

```text
spec 0  5% critical values for rank <= 0, 1, 2: 24.22 12.33 4.14  selected rank 1
spec 1  5% critical values for rank <= 0, 1, 2: 29.66 15.46 3.85  selected rank 1
spec 2  5% critical values for rank <= 0, 1, 2: 34.97 18.40 3.87  selected rank 1
```


## See Also

- [`JohansenOptions`](JohansenOptions.md)
- [`JohansenResult`](JohansenResult.md)
- [`Vecm`](Vecm.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IrfOptions.html">
    <span class="label">Previous</span>
    <span class="title">IrfOptions</span>
  </a>
  <a class="next-link" href="JohansenOptions.html">
    <span class="label">Next</span>
    <span class="title">JohansenOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
