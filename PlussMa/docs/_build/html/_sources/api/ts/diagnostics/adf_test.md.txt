<!-- generated from adf_test.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::adf_test`

The augmented Dickey–Fuller test for a unit root.

## Signature

```cpp
ts::AdfResult ts::adf_test(const pluss::Series& y, const ts::AdfOptions& options = {}, pluss::codem::Config cfg = {});
```

## What It Does

`adf_test` regresses the change of the series on its lagged level (and on a constant, or a constant and a trend, and on `lags` lagged changes) and tests whether the coefficient of the lagged level is zero, which is the null hypothesis of a unit root. The statistic is the t-ratio of that coefficient; its distribution is not Student's but a function of the sample size and of the deterministic terms, so the p-value and the critical value come from response-surface approximations, not from a table of a few sizes. **Rejecting the null means the series is stationary** (around the chosen deterministic part).

Three specifications are offered: `AR` (no deterministic term), `ARD` (a constant, the default) and `TS` (a constant and a linear trend). Add lagged changes (`lags`) when the series has short-run dynamics that would otherwise autocorrelate the regression's errors. Because the null is a unit root, ADF is best read together with [`kpss_test`](kpss_test.md), whose null is stationarity.

## Parameters

`const pluss::Series& y`
: The series to test, in time order.

`const ts::AdfOptions& options`
: `lags` (augmentation lags, default 0), `model` (`AR`, `ARD` default, `TS`) and `alpha` (level of the critical value, default 0.05); see [`AdfOptions`](AdfOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); the computation itself runs in double precision.

## Returns

An [`AdfResult`](AdfResult.md): the statistic, the p-value, the critical value at `alpha`, `reject`, the augmentation lags used and the observations used.

## Errors

- `lags` must leave enough observations for the regression.

## Practical Examples

### Example 1

A stationary autoregression rejects the unit root; a random walk does not.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  for (const auto& [name, series] : {std::pair{"AR(1)", ar1(200, 0.7)}, std::pair{"random walk", walk(200)}}) {
    const auto r = ts::adf_test(series);
    std::cout << std::fixed << std::setprecision(3) << std::left << std::setw(12) << name << "statistic " << r.stat << "  p = " << r.pvalue << "  critical (5%) " << r.critical
              << "  unit root rejected: " << std::boolalpha << r.reject << "\n";
  }
}
```

Output

```text
AR(1)       statistic -6.415  p = 0.000  critical (5%) -2.876  unit root rejected: true
random walk statistic -1.914  p = 0.326  critical (5%) -2.876  unit root rejected: false
```

### Example 2

The three specifications differ in their critical values: more deterministic terms make the critical value more negative.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  const Series y = ar1(200, 0.7);
  for (auto model : {ts::AdfModel::AR, ts::AdfModel::ARD, ts::AdfModel::TS}) {
    ts::AdfOptions options;
    options.model = model;
    const auto r = ts::adf_test(y, options);
    std::cout << std::fixed << std::setprecision(4) << "model " << static_cast<int>(model) << "  statistic " << r.stat << "  critical " << r.critical << "\n";
  }
}
```

Output

```text
model 0  statistic -6.3792  critical -1.9424
model 1  statistic -6.4149  critical -2.8762
model 2  statistic -6.4114  critical -3.4328
```

### Example 3

Augmentation lags absorb short-run dynamics; they use up observations.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  const Series y = ar1(200, 0.7);
  for (int lags : {0, 2, 4}) {
    ts::AdfOptions options;
    options.lags = lags;
    const auto r = ts::adf_test(y, options);
    std::cout << "lags " << lags << "  statistic " << std::fixed << std::setprecision(3) << r.stat << "  observations used " << r.effective_n << "\n";
  }
}
```

Output

```text
lags 0  statistic -6.415  observations used 199
lags 2  statistic -4.205  observations used 197
lags 4  statistic -3.477  observations used 195
```


## See Also

- [`AdfOptions`](AdfOptions.md)
- [`AdfResult`](AdfResult.md)
- [`kpss_test`](kpss_test.md)
