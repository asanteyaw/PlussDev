<!-- generated from Var.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::Var`

Fits a vector autoregression by least squares (Gaussian maximum likelihood).

## Signature

```cpp
ts::VarFit ts::Var::fit_mle(const pluss::Series& y, const ts::VarOptions& options = {}, pluss::codem::Config cfg = {});
ts::VarFit ts::Var::fit_mle(const pluss::Series& y, const pluss::Series& x, const ts::VarOptions& options = {}, pluss::codem::Config cfg = {});
```

## Construction

`Var` has no instances: it is a namespace for the static `fit_mle`, which returns a [`VarFit`](VarFit.md).

## Methods

### `fit_mle`

```cpp
ts::VarFit ts::Var::fit_mle(const pluss::Series& y, const ts::VarOptions& options = {}, pluss::codem::Config cfg = {});
ts::VarFit ts::Var::fit_mle(const pluss::Series& y, const pluss::Series& x, const ts::VarOptions& options = {}, pluss::codem::Config cfg = {});
```

Fits `y_t = c + A_1 y_{t−1} + … + A_p y_{t−p} + e_t` by least squares, equation by equation; for Gaussian errors this is the maximum-likelihood estimate. The covariance of the errors is the maximum-likelihood one: the cross-products of the residuals divided by the number of fitted rows `T = n − p`. The second form adds exogenous regressors `x`, whose coefficients are returned in `beta`.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

`const ts::VarOptions& options`
: Lag order and deterministic terms; see [`VarOptions`](VarOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A [`VarFit`](VarFit.md).

**Errors.** `y` must have more rows than the number of lagged regressors.

## Practical Examples

### Example 1

Fit VARs of order one to three to two linked series and compare them by the information criteria: the true order is one, and both criteria choose it.

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
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t   (n x 2, row t = (a_t, b_t)).
Series linked(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(2 * n);
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[t];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[t] + e2[t];
    a = na;
    b = nb;
    y[2 * t] = a;
    y[2 * t + 1] = b;
  }
  return pluss::make_series(y, {n, 2}, f64);
}

int main() {
  const Series y = linked(300);
  std::cout << std::fixed << std::setprecision(1) << std::setw(6) << "lags" << std::setw(12) << "loglik" << std::setw(12) << "Akaike" << std::setw(12) << "Schwarz" << "\n";
  for (int p = 1; p <= 3; ++p) {
    ts::VarOptions options;
    options.lag_order = p;
    const auto fit = ts::Var::fit_mle(y, options);
    const double ll = fit.loglik(y), m = 2.0 * (2.0 * p + 1.0);
    std::cout << std::setw(6) << p << std::setw(12) << ll << std::setw(12) << 2 * m - 2 * ll << std::setw(12) << m * std::log(static_cast<double>(fit.sample_size)) - 2 * ll << "\n";
  }
}
```

Output

```text
  lags      loglik      Akaike     Schwarz
     1       -93.7       199.4       221.6
     2       -93.2       206.3       243.3
     3       -89.2       206.5       258.2
```

### Example 2

A time trend can be added to every equation; here it is estimated close to zero because the series has none.

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
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t   (n x 2, row t = (a_t, b_t)).
Series linked(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(2 * n);
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[t];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[t] + e2[t];
    a = na;
    b = nb;
    y[2 * t] = a;
    y[2 * t + 1] = b;
  }
  return pluss::make_series(y, {n, 2}, f64);
}

int main() {
  const Series y = linked(300);
  ts::VarOptions options;
  options.include_trend = true;
  const auto fit = ts::Var::fit_mle(y, options);
  show("trend", fit.trend);
}
```

Output

```text
trend           -0.0002  -0.0001
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`VarOptions`](VarOptions.md)
- [`VarFit`](VarFit.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
