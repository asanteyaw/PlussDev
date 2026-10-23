<!-- generated from Arima.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::Arima`

Fits an ARIMA model by maximum likelihood.

## Signature

```cpp
ts::ArimaFit ts::Arima::fit_mle(const pluss::Series& y, const ts::ArimaOptions& options = {}, pluss::codem::Config cfg = {});
ts::ArimaFit ts::Arima::fit_mle(const pluss::Series& y, const pluss::Series& x, const ts::ArimaOptions& options = {}, pluss::codem::Config cfg = {});
```

## Construction

`Arima` has no instances: it is a namespace for the static `fit_mle`, which returns an [`ArimaFit`](ArimaFit.md).

## Methods

### `fit_mle`

```cpp
ts::ArimaFit ts::Arima::fit_mle(const pluss::Series& y, const ts::ArimaOptions& options = {}, pluss::codem::Config cfg = {});
ts::ArimaFit ts::Arima::fit_mle(const pluss::Series& y, const pluss::Series& x, const ts::ArimaOptions& options = {}, pluss::codem::Config cfg = {});
```

The first form fits the model to `y`. The second adds exogenous regressors `x` (one row per observation, `k` columns): they enter the conditional mean beside the lagged observations, `y_t = c + β'x_t + a_1 y_{t−1} + … + e_t + b_1 e_{t−1} + …` (an ARMAX model), and the fit uses the conditional likelihood. After differencing, the regressors are aligned with the differenced series. The fit maximises the Gaussian likelihood over the intercept, the coefficients of the four polynomials and (for the second form) `β`; the innovation variance is concentrated out. The search runs over the partial autocorrelations of each polynomial, so every candidate is stationary and invertible and the whole such region is reachable; the default start is the zero polynomial.

`const pluss::Series& y`
: The observed series in time order (a vector or an `n × 1` matrix).

`const pluss::Series& x`
: Exogenous regressors: an `n × k` matrix (or a vector) with one row per observation of `y`.

`const ts::ArimaOptions& options`
: Orders, starting values and search settings; see [`ArimaOptions`](ArimaOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); the fit itself runs in double precision.

**Returns.** An [`ArimaFit`](ArimaFit.md) holding the estimates and the methods that evaluate the fitted model.

**Errors.** The effective sample (after differencing) must be longer than the number of parameters: a very short series is rejected. Seasonal orders require `seasonal_period > 1`. `x` must have one row per observation of `y`.

## Practical Examples

### Example 1

Fit an AR(1) model and read the estimates and the log-likelihood.

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
  ts::ArimaOptions options;
  options.ar_order = 1;
  const auto fit = ts::Arima::fit_mle(y, options);
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "ar = " << fit.ar[0] << ", sigma = " << fit.sigma << ", mean = " << fit.mean << "\n";
  std::cout << "log-likelihood = " << fit.loglik(y) << "\n";
}
```

Output

```text
ar = 0.6506, sigma = 0.2841, mean = 0.0432
log-likelihood = -32.3468
```

### Example 2

Compare candidate orders by Akaike's criterion, `2k − 2 loglik` with `k` the number of estimated parameters (coefficients, the mean and the variance).

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
  for (int p = 0; p <= 2; ++p) {
    for (int q = 0; q <= 1; ++q) {
      if (p == 0 && q == 0) continue;
      ts::ArimaOptions options;
      options.ar_order = p;
      options.ma_order = q;
      const auto fit = ts::Arima::fit_mle(y, options);
      const double ll = fit.loglik(y);
      std::cout << "ARMA(" << p << "," << q << ")  log-likelihood " << std::fixed << std::setprecision(3) << ll << "  Akaike " << 2 * (p + q + 2) - 2 * ll << "\n";
    }
  }
}
```

Output

```text
ARMA(0,1)  log-likelihood -45.337  Akaike 96.675
ARMA(1,0)  log-likelihood -32.347  Akaike 70.694
ARMA(1,1)  log-likelihood -32.040  Akaike 72.080
ARMA(2,0)  log-likelihood -32.214  Akaike 72.427
ARMA(2,1)  log-likelihood -31.330  Akaike 72.660
```

### Example 3

A model with one regular difference, for a series that wanders.

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
  const Series y = walk(200);
  ts::ArimaOptions options;
  options.ar_order = 0;
  options.diff_order = 1;
  options.ma_order = 1;
  const auto fit = ts::Arima::fit_mle(y, options);
  std::cout << std::fixed << std::setprecision(4) << "ma = " << fit.ma[0] << ", innovation sd = " << fit.sigma << "\n";
  show("forecast", fit.forecast(3, y));
}
```

Output

```text
ma = -0.0932, innovation sd = 0.2842
forecast         2.8656   2.8787   2.8918
```

### Example 4

With exogenous regressors the regression coefficient is estimated beside the autoregression (here a series driven by a sine wave).

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
  std::vector<double> xv(200), yv(200);
  const auto e = noise(200);
  double carry = 0.0;
  for (int i = 0; i < 200; ++i) {
    xv[i] = std::sin(0.3 * i);
    carry = 0.5 * carry + 0.3 * e[i];
    yv[i] = 2.0 * xv[i] + carry;
  }
  const Series x = pluss::make_series(xv, {200, 1}, f64);
  const Series y = pluss::make_series(yv, {200}, f64);
  ts::ArimaOptions options;
  options.ar_order = 1;
  const auto fit = ts::Arima::fit_mle(y, x, options);
  std::cout << std::fixed << std::setprecision(4) << "beta = " << fit.beta[0] << ", ar = " << fit.ar[0] << "\n";
}
```

Output

```text
beta = 2.0418, ar = -0.0143
```

### Example 5

An over-short series is refused before any search.

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
  try {
    ts::Arima::fit_mle(pluss::make_series({1.0, 2.0}, f64));
  } catch (const std::exception&) {
    std::cout << "a two-observation series is rejected\n";
  }
}
```

Output

```text
a two-observation series is rejected
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`ArimaOptions`](ArimaOptions.md)
- [`ArimaFit`](ArimaFit.md)
- [`Sarima`](Sarima.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
